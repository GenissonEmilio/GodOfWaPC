#include "control.hpp"
#include "ps2/host_file.hpp"
#include "ps2/system.hpp"
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
void release_pad() {
    ps2::g_options.presses.clear();
    ps2::g_options.axis_presses.clear();
    ps2::set_pad_buttons(0);
    for (int axis=0; axis<4; ++axis) ps2::set_pad_axis(axis,128);
}
bool capture(const std::string& path) {
    std::vector<std::uint32_t> pixels;
    std::uint32_t width=0,height=0;
    if (!ps2::display_frame(pixels,width,height)) return false;
    auto* file=ps2::host_fopen(path,"wb");
    if (!file) return false;
    bool ok=std::fprintf(file,"P6\n%u %u\n255\n",width,height)>0;
    for (auto pixel:pixels) {
        const unsigned char rgb[]={static_cast<unsigned char>(pixel),static_cast<unsigned char>(pixel>>8),static_cast<unsigned char>(pixel>>16)};
        if (std::fwrite(rgb,1,3,file)!=3) { ok=false; break; }
    }
    return std::fclose(file)==0 && ok;
}
}

void enable_step_control() {
    ps2::g_options.on_vblank=[until=std::uint64_t{0}]() mutable {
        const auto frame=ps2::frame_count();
        if (frame<until) return;
        release_pad();
        for (;;) {
            std::cout<<"[control] READY frame="<<frame<<std::endl;
            std::string line;
            if (!std::getline(std::cin,line)) { ps2::halt("control input closed"); return; }
            std::istringstream in(line);
            std::string verb; in>>verb;
            if (verb=="quit") { ps2::halt("control quit"); return; }
            if (verb=="capture" || verb=="save") {
                std::string path; std::getline(in>>std::ws,path);
                if (path.empty()) { std::cout<<"[control] ERROR missing path\n"; continue; }
                if (verb=="capture") {
                    std::cout<<"[control] CAPTURE "<<(capture(path)?"PASS ":"FAIL ")<<path<<std::endl;
                    continue;
                }
                // Save after the vblank callback has returned and the event
                // scheduler has queued the next vblank. Saving inside this
                // callback would omit that event from the checkpoint.
                ps2::g_options.save_state_frame=frame+1;
                ps2::g_options.save_state_path=path;
                until=frame+2;
                return;
            }
            if (verb=="step") {
                std::uint64_t count=0;
                std::string buttons="-";
                if (!(in>>count) || count==0 || count>1000000) {
                    std::cout<<"[control] ERROR step needs 1..1000000 frames\n"; continue;
                }
                in>>buttons;
                ps2::Press press{};
                std::vector<ps2::AxisPress> axes;
                bool valid=buttons=="-" || ps2::parse_press(std::to_string(frame+1)+":"+buttons+":"+std::to_string(count),press);
                std::string token;
                while (in>>token) {
                    const auto equal=token.find('=');
                    if (equal==std::string::npos) { valid=false; break; }
                    int value=-1; std::string trailing;
                    std::istringstream number(token.substr(equal+1));
                    if (!(number>>value) || (number>>trailing) || value<0 || value>255) { valid=false; break; }
                    ps2::AxisPress axis{};
                    if (!ps2::parse_axis(std::to_string(frame+1)+":"+token.substr(0,equal)+":"+std::to_string(value)+":"+std::to_string(count),axis)) { valid=false; break; }
                    axes.push_back(axis);
                }
                if (!valid) { std::cout<<"[control] ERROR invalid buttons or axes\n"; continue; }
                if (buttons!="-") ps2::g_options.presses.push_back(press);
                ps2::g_options.axis_presses=std::move(axes);
                until=frame+count;
                return;
            }
            std::cout<<"[control] Commands: step <frames> [buttons|-] [lx=0..255 ly=0..255 rx=0..255 ry=0..255], capture <path.ppm>, save <path.state>, quit\n";
        }
    };
}
