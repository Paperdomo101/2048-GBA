#include "global.h"
#include <stdlib.h>
#include <string.h>


int main(void) {

    State *state = GetState();

    irq_init(NULL);

    irq_set(II_VBLANK, mmVBlank, 0);
    irq_enable(II_VBLANK);

    LoadAssets();

    SetMode(GM_FIRST);

    while(1) {
        mmFrame();
        switch (state->mode)
        {
        case GM_FIRST:
            UpdateFirst();
            break;
        case GM_TITLE:
            UpdateTitle();
            break;
        case GM_GAME:
            UpdateGame();
            break;
        case GM_GAMEOVER:
            UpdateOver();
            break;
        case GM_WIN:
            UpdateWin();
            break;
        default:
            break;
        }
    }

    return 0;
}
