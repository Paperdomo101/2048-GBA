#include <string.h>
#include <stdlib.h>

#include "global.h"

#include "gameover_gfx.h"

static State *state;

static int fade_timer = 0;
static int fade_over = 0;

static OBJ_ATTR obj_buffer[18];

static OBJ_ATTR *obj_game = &obj_buffer[0];
static OBJ_ATTR *obj_over = &obj_buffer[1];

static void UpdateOver() {
    state->seed += rand() << 1;

    REG_BG1VOFS = GetDigitCount(0) < 5 ? -3 : -4;
    REG_BG1HOFS = GetBG1Off(0) + GetFirstIs1(0);

    if (fade_timer > 0) {
        int a = (7 - fade_timer);
        if (a < 0) a = 0;
        clr_fade_fast(pal_bg_mem, pal_bg_mem[11], pal_bg_mem, 5, a);
        clr_fade_fast(pal_obj_mem, pal_bg_mem[11], pal_obj_mem, 32, a);
        --fade_timer;
        if (fade_timer == 0) {
            fade_over = 1;
            obj_set_attr(obj_game, ATTR0_WIDE, ATTR1_SIZE_64x32, ATTR2_PALBANK(2) | 256);
            obj_set_attr(obj_over, ATTR0_WIDE, ATTR1_SIZE_64x32, ATTR2_PALBANK(2) | (256 + 32));
            obj_set_pos(obj_game, 54, 69);
            obj_set_pos(obj_over, 54 + 64, 69);
        }
    }

    if (fade_over) {
        if (key_hit(KEY_RETURN)) {
            srand(state->seed);
            obj_hide(obj_game);
            obj_hide(obj_over);
            memset16(pal_bg_mem, pal_bg_mem[5], 256);
            memset16(pal_obj_mem, pal_bg_mem[5], 256);

            // alllow retry with START
            if (!key_hit(KEY_START)) {
                state->saved = 0;
                SaveState(state);
            } else {
                LoadState(state);
            }

            SetMode(GM_GAME);
            return;
        }
    }

    obj_copy(obj_mem, obj_buffer, 18);
}

void InitOver() {
    state = GetState();
    state->update = UpdateOver;
    fade_over = 0;
    fade_timer = TITLE_FADE_DURATION;
    REG_BG1VOFS = GetDigitCount(0) < 5 ? -3 : -4;
    REG_BG1HOFS = GetBG1Off(0) + GetFirstIs1(0);

    memcpy(&tile_mem[4][256], gameover_gfx, gameover_gfx_size);

    obj_copy(obj_buffer+2, obj_mem, 16); // 16 number tiles
    obj_game = &obj_buffer[0];
    obj_over = &obj_buffer[1];

    obj_hide(obj_game);
    obj_hide(obj_over);

    mmEffect(SFX_LOSE);
}
