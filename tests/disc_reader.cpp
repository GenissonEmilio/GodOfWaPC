// Integration check for DVD9 reads beyond the 32-bit file offset boundary.
// Expected values are computed independently from the user's ISO by Python.
#include "ps2/disc.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>

int main(int argc, char** argv) {
    if (argc != 5 || !ps2::disc::open(argv[1])) return 2;
    auto part = ps2::disc::find("PART2.PAK");
    if (!part || !ps2::disc::layer1_start()) return 3;
    const std::uint32_t sectors[] = {part->sector + 16384u,
        part->sector + part->size / 2048u - 8u,
        static_cast<std::uint32_t>(ps2::disc::sector_count() - 1)};
    for (int i=0;i<3;++i) {
        std::array<std::uint8_t,2048> bytes{};
        if (!ps2::disc::read(sectors[i],1,bytes.data())) return 4;
        std::uint64_t hash=14695981039346656037ull;
        for (auto byte:bytes) hash=(hash^byte)*1099511628211ull;
        if (hash != std::strtoull(argv[i+2],nullptr,16)) return 5;
        std::printf("DVD9 sector %u beyond 4 GiB: PASS\n",sectors[i]);
    }
    return 0;
}
