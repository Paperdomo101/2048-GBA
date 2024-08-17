#include "global.h"
#include "maxmod.h"
#include "tonc_input.h"
#include "tonc_memdef.h"
#include "tonc_memmap.h"
#include "tonc_oam.h"
#include "tonc_types.h"
#include "tonc_video.h"
#include "youwin.h"
#include <string.h>
#include <stdlib.h>

static State *state;
static Assets *assets;

static int fade_timer = 0;
static int fade_over = 0;

static OBJ_ATTR obj_buffer[2];

static OBJ_ATTR *obj_you = &obj_buffer[0];
static OBJ_ATTR *obj_win = &obj_buffer[1];

void InitWin(void) {
    state = GetState();
    assets = GetAssets();
    fade_over = 0;
    fade_timer = 30;
    REG_BG1VOFS = GetDigitCount(0) < 5 ? -3 : -4;
    REG_BG1HOFS = GetBG1Off(0) + GetFirstIs1(0);


    memcpy(&tile_mem[4][256], youwinTiles, youwinTilesLen);

    obj_copy(obj_mem+2, obj_mem, 16); // 16 number tiles
    obj_you = &obj_buffer[0];
    obj_win = &obj_buffer[1];

    obj_hide(obj_you);
    obj_hide(obj_win);

    mmEffectEx(&assets->sfx.win);
}

void UpdateWin(void) {
    state->seed += rand() << 1;
    vid_vsync();
    key_poll();

    REG_BG1VOFS = GetDigitCount(0) < 5 ? -3 : -4;
    REG_BG1HOFS = GetBG1Off(0) + GetFirstIs1(0);

    if (fade_timer > 0) {
        int a = (16 - fade_timer);
        if (a < 0) a = 0;
        clr_fade_fast(pal_bg_mem, 0x371C, pal_bg_mem, 5, a);
        clr_fade_fast(pal_obj_mem, 0x371C, pal_obj_mem, 32, a);
        --fade_timer;
        if (fade_timer == 0) {
            fade_over = 1;
            obj_set_attr(obj_you, ATTR0_WIDE, ATTR1_SIZE_64x32, ATTR2_PALBANK(2) | 256);
            obj_set_attr(obj_win, ATTR0_WIDE, ATTR1_SIZE_64x32, ATTR2_PALBANK(2) | (256 + 32));
            obj_set_pos(obj_you, 54, 69);
            obj_set_pos(obj_win, 54 + 64, 69);
        }
    }

    if (fade_over) {
        if (key_hit(KEY_SELECT)) {
            srand(state->seed);
            obj_hide(obj_you);
            obj_hide(obj_win);
            state->saved = 0;
            SaveState(state);
            SetMode(GM_GAME);
        }
    }

    obj_copy(obj_mem, obj_buffer, 2);
}
