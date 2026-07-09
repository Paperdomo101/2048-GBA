#include <stdlib.h>

#include "global.h"

static State *state;

static int fade_timer = 0;
static int fade_over = 0;
static int reset_timer = 0;

static void UpdateTitle() {
    state->seed += rand() << 1;

    if (fade_timer > 0) {
        clr_fade_fast(&pal_bg_mem[0], pal_bg_mem[5], &pal_bg_mem[0], 256, TITLE_FADE_DURATION - fade_timer);
        --fade_timer;
        if (fade_timer == 0) {
            fade_over = 1;
        }
    } else {
        if (key_hit(KEY_START | KEY_A | KEY_B)) {
            mmEffect(SFX_START);
            fade_timer = TITLE_FADE_DURATION;
        }
        if (key_held(KEY_RESET_HI) == KEY_RESET_HI) {
            reset_timer++;
            if (reset_timer == 80) {
                reset_timer = 0;
                CleanStorage(1);
                LoadState(state);
                mmEffect(SFX_LOSE);
            }
        }
    }

    if (fade_over) {
        srand(state->seed);
        memset16(pal_bg_mem, pal_bg_mem[5], 256);
        memset16(pal_obj_mem, pal_bg_mem[5], 256);
        SetMode(GM_GAME);
        return;
    }
}

void InitTitle() {
    state = GetState();
    state->update = UpdateTitle;
    fade_over = 0;
    fade_timer = 0;
    reset_timer = 0;
}
