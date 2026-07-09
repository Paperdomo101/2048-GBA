#include <stdlib.h>
#include <string.h>

#include "global.h"
#include "soundbank_bin.h"

int main() {
    State *state = GetState();

    mmInitDefault((mm_addr)soundbank_bin, 8);

    irq_init(NULL);

    irq_set(II_VBLANK, mmVBlank, 0);
    irq_enable(II_VBLANK);

    LoadState(state);

    SetMode(GM_FIRST);

    // main loop
    while(1) {
        mmFrame();
        key_poll();
        state->update();
        VBlankIntrWait();
    }

    return 0;
}
