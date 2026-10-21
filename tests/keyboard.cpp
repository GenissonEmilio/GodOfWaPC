#include "keyboard.hpp"
#include <cstdio>

int main() {
    using namespace gow2::keyboard;
    std::array<std::uint8_t,512> down{};
    auto digital=default_buttons; auto sticks=default_sticks;
    const auto check=[](bool ok,const char* name) {
        std::printf("Keyboard %s: %s\n",name,ok?"PASS":"FAIL"); return ok;
    };
    bool ok=true;
    down[82]=1; auto input=map(down,digital,sticks); // up arrow
    ok&=check(input.buttons==0 && input.axes[3]==0,"movement does not select magic");
    down[81]=1; input=map(down,digital,sticks);
    ok&=check(input.axes[3]==128,"opposite movement keys cancel");
    down={}; down[23]=1; input=map(down,digital,sticks); // T
    ok&=check(input.buttons==0x10 && input.axes[3]==128,"digital magic selector does not move");
    down={}; down[6]=down[25]=1; input=map(down,digital,sticks); // C+V
    ok&=check(input.buttons==6,"L3 and R3 available for Rage of the Titans");
    down={}; down[12]=1; input=map(down,digital,sticks); // I
    ok&=check(input.axes[1]==0 && input.axes[3]==128 && input.buttons==0,"right stick remains independent");
    sticks[0]=96; down={}; down[96]=1; input=map(down,digital,sticks); // keypad 8
    ok&=check(input.axes[3]==0 && input.buttons==0,"analog movement can be remapped");
    digital=legacy_buttons;
    ok&=check(migrate(digital,0) && digital==default_buttons,"legacy defaults migrate");
    digital=legacy_buttons; digital[15]=5;
    ok&=check(!migrate(digital,0) && digital[15]==5,"custom bindings are preserved");
    return ok?0:1;
}
