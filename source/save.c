#include "global.h"
#include "tonc_memmap.h"
#include "tonc_video.h"
#include "savescreen.h"
#include <string.h>

static State *state;
static Assets *assets;

static int fade_timer = 0;
static int fade_over = 0;

void InitSave(void) {
    state = GetState();
    assets = GetAssets();
    fade_over = 0;
    fade_timer = TITLE_FADE_DURATION;

    memcpy(&tile_mem[0][0], savescreenTiles, savescreenTilesLen);
	memcpy(&se_mem[30][0], savescreenMap, savescreenMapLen);

    REG_DISPCNT = DCNT_BG0 | DCNT_MODE0;

    state->saved = 1;
    SaveState(state);
    mmEffectEx(&assets->sfx.save);
}

void UpdateSave(void) {
    vid_vsync();

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
    }
}
