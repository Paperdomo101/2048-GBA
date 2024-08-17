#include "global.h"
#include <stdlib.h>
#include <string.h>


int main(void) {

    State *state = GetState();

    irq_init(NULL);

    irq_set(II_VBLANK, mmVBlank, 0);
    irq_enable(II_VBLANK);

    LoadAssets();

    LoadState(state);

    SetMode(GM_FIRST);

    while(1) {
        mmFrame();
        UpdateState();
    }

    return 0;
}
