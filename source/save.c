#include <string.h>

#include "global.h"

#include "savescreen_gfx.h"
#include "savescreen_map.h"

static State *state;

static int fade_timer = 0;
static int fade_over = 0;

static void UpdateSave() {
    if (fade_timer > 0) {
        clr_fade_fast(&pal_bg_mem[0], pal_bg_mem[5], &pal_bg_mem[0], 256, TITLE_FADE_DURATION - fade_timer);
        --fade_timer;
        if (fade_timer == 0) {
            fade_over = 1;
        }
    }

    if (fade_over) {
        memset16(pal_bg_mem, pal_bg_mem[5], 256);
        memset16(pal_obj_mem, pal_bg_mem[5], 256);
        SetMode(GM_GAME);
        return;
    }
}

void InitSave() {
    state = GetState();
    fade_over = 0;
    fade_timer = TITLE_FADE_DURATION;

    memcpy(&tile_mem[0][0], savescreen_gfx, savescreen_gfx_size);
    memcpy(&se_mem[30][0], savescreen_map, savescreen_map_size);

    REG_DISPCNT = DCNT_BG0 | DCNT_MODE0;

    state->saved = 1;
    SaveState(state);
    state->update = UpdateSave;
    mmEffect(SFX_SAVE);
}
