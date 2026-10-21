// Exercise the public SIO2 registers and the real host filesystem. Run write
// and read as separate processes to verify persistence across a restart.
#include "iop/sio2.hpp"
#include "ps2/host_file.hpp"
#include "ps2/system.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
std::vector<std::uint8_t> transfer(const std::vector<std::uint8_t>& input, unsigned size) {
    using namespace ps2::iop::sio2;
    write(0x1F808268, 0xC);
    write(0x1F808200, 2u | (static_cast<unsigned>(input.size()) << 8) | (size << 18));
    for (auto value : input) write(0x1F808260, value);
    std::vector<std::uint8_t> output;
    for (unsigned i=0; i<size; ++i) output.push_back(static_cast<std::uint8_t>(read(0x1F808264)));
    return output;
}
void address(std::uint8_t command) {
    auto r=transfer({0x81,command,0x40,0x01,0,0,0x41},9); // page 320
    require(r[7]==0x2B && r[8]==0x55,"card address acknowledgement");
}
std::vector<std::uint8_t> pattern(int variant) {
    std::vector<std::uint8_t> data(528);
    for (std::size_t i=0; i<data.size(); ++i) data[i]=static_cast<std::uint8_t>((i*37+variant*0x35)^(i/7));
    return data;
}
void write_page(int variant) {
    address(0x21); transfer({0x81,0x82},4); address(0x22);
    auto data=pattern(variant);
    for (std::size_t at=0; at<data.size(); at+=128) {
        const auto count=std::min<std::size_t>(128,data.size()-at);
        std::vector<std::uint8_t> packet{0x81,0x42,static_cast<std::uint8_t>(count)};
        std::uint8_t sum=0;
        for (std::size_t i=0;i<count;++i) { packet.push_back(data[at+i]); sum^=data[at+i]; }
        packet.push_back(sum);
        auto r=transfer(packet,8);
        require(r[6]==0x2B && r[7]==0x55,"card write acknowledgement");
    }
}
void read_page(int variant) {
    address(0x23); auto data=pattern(variant);
    for (std::size_t at=0; at<data.size(); at+=128) {
        const auto count=std::min<std::size_t>(128,data.size()-at);
        auto r=transfer({0x81,0x43,static_cast<std::uint8_t>(count)},static_cast<unsigned>(count+6));
        require(r[3]==0x2B && r.back()==0x55,"card read acknowledgement");
        std::uint8_t sum=0;
        for (std::size_t i=0;i<count;++i) { require(r[4+i]==data[at+i],"persisted page mismatch"); sum^=r[4+i]; }
        require(r[4+count]==sum,"card read checksum");
    }
}
}
int main(int argc,char** argv) {
    try {
        require(argc==4,"usage: gow2_memcard_check write|read|retry|blocked|force <path> <variant>");
        const std::string mode=argv[1]; const int variant=std::stoi(argv[3]);
        ps2::g_options.memcard_path=argv[2];
        const auto file=ps2::host_path(argv[2]);
        unsigned commits=0; ps2::g_options.on_memcard_saved=[&]{++commits;};
        ps2::iop::sio2::reset();
        if (mode=="read") read_page(variant);
        else {
            std::vector<char> previous;
            if (mode=="blocked") {
                std::ifstream old(file,std::ios::binary);
                require(old.good(),"blocked test needs an existing card");
                previous.assign(std::istreambuf_iterator<char>(old),{});
            }
            write_page(variant);
            if (mode=="retry") {
                require(!std::filesystem::exists(file.parent_path()),"retry test needs an absent parent directory");
                for (int i=0;i<50;++i) ps2::iop::sio2::flush_cards();
                require(commits==0,"failed write reported success");
                std::filesystem::create_directories(file.parent_path());
            }
            if (mode=="blocked") {
                auto temporary=file; temporary+=".tmp";
                require(std::filesystem::create_directory(temporary),"cannot block temporary output");
                for (int i=0;i<50;++i) ps2::iop::sio2::flush_cards();
                require(commits==0,"blocked write reported success");
                std::ifstream old(file,std::ios::binary);
                std::vector<char> current{std::istreambuf_iterator<char>(old),{}};
                require(current==previous,"failed write changed the previous card");
                require(std::filesystem::remove(temporary),"cannot unblock temporary output");
            }
            if (mode=="force") ps2::iop::sio2::flush_cards(true);
            else for (int i=0;i<50;++i) ps2::iop::sio2::flush_cards();
            require(commits==1,"card not committed exactly once");
            require(std::filesystem::file_size(file)==528u*0x4000u,"card file size");
            require(!std::filesystem::exists(file.string()+".tmp"),"uncommitted temporary file");
            read_page(variant);
        }
        std::printf("SIO2 %s variant %d: PASS\n",mode.c_str(),variant);
        return 0;
    } catch(const std::exception& error) { std::fprintf(stderr,"FAIL: %s\n",error.what()); return 1; }
}
