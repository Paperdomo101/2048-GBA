#include "global.h"
#include <stdlib.h>

static State *state;
static Assets *assets;

static int fade_timer = 0;
static int fade_over = 0;

void InitTitle(void) {
    state = GetState();
    assets = GetAssets();

    fade_over = 0;
    fade_timer = 0;
}

void UpdateTitle(void) {
    state->seed += rand() << 1;
    vid_vsync();
    key_poll();


    if (fade_timer > 0) {
        clr_fade_fast(&pal_bg_mem[0], pal_bg_mem[5], &pal_bg_mem[0], 256, TITLE_FADE_DURATION - fade_timer);
        --fade_timer;
        if (fade_timer == 0) {
            fade_over = 1;
        }
    } else {
        if (key_hit(KEY_START | KEY_A | KEY_B)) {
            mmEffectEx(&assets->sfx.start);
            fade_timer = TITLE_FADE_DURATION;
        }
    }

    if (fade_over) {
        srand(state->seed);
        SetMode(GM_GAME);
    }
}
