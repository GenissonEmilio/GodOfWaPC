#pragma once
#include <array>
#include <cstdint>
#include <span>

namespace gow2::keyboard {
// SDL scancodes. Keep movement separate from the PS2 digital pad, which
// selects magic in God of War II. C+V provides L3+R3 for Rage of the Titans.
inline constexpr std::array<int,16> default_buttons{42,6,25,40,23,11,10,9,30,31,20,26,22,27,29,4};
inline constexpr std::array<int,16> legacy_buttons{42,0,0,40,82,79,81,80,30,31,20,26,22,27,29,4};
// Up, right, down, left for the left stick, then for the right stick.
inline constexpr std::array<int,8> default_sticks{82,79,81,80,12,15,14,13};
struct State {
    std::uint16_t buttons=0;
    std::array<std::uint8_t,4> axes{128,128,128,128}; // RX, RY, LX, LY
};
inline bool migrate(std::array<int,16>& buttons,unsigned version) {
    if (version<2 && buttons==legacy_buttons) { buttons=default_buttons; return true; }
    return false;
}
inline State map(std::span<const std::uint8_t> down,const std::array<int,16>& buttons,const std::array<int,8>& sticks) {
    const auto held=[&](int code) { return code>0 && static_cast<std::size_t>(code)<down.size() && down[code]!=0; };
    const auto axis=[&](int negative,int positive)->std::uint8_t {
        const int value=static_cast<int>(held(positive))-static_cast<int>(held(negative));
        return value<0?0:value>0?255:128;
    };
    State result;
    for (int i=0;i<16;++i) if (held(buttons[i])) result.buttons|=1u<<i;
    result.axes[2]=axis(sticks[3],sticks[1]); result.axes[3]=axis(sticks[0],sticks[2]);
    result.axes[0]=axis(sticks[7],sticks[5]); result.axes[1]=axis(sticks[4],sticks[6]);
    return result;
}
}
