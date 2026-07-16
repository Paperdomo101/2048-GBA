#include <string.h>

#include "global.h"

#include "titlebg0_gfx.h"
#include "titlebg0_map.h"
#include "titlebg0_pal.h"

static State *state;

static int fade_timer = 0;
static int fade_over = 0;

static void UpdateFirst() {
    if (fade_timer > 0) {
        clr_blend(&pal_bg_mem[0], (COLOR *)titlebg0_pal, &pal_bg_mem[0], 256, (FIRST_FADE_DURATION - fade_timer) << 1);
        --fade_timer;
        if (fade_timer == 0) {
            fade_over = 1;
        }
    }

    if (fade_over) {
        memcpy(pal_bg_mem, titlebg0_pal, titlebg0_pal_size);
        SetMode(GM_TITLE);
        return;
    }
}

void InitFirst() {
    state = GetState();
    state->update = UpdateFirst;
    fade_over = 0;
    fade_timer = FIRST_FADE_DURATION;

    pal_gradient_ex(pal_bg_mem, 0, 15, 0x7bff, 0x7bff);

    memcpy(&tile_mem[0][0], titlebg0_gfx, titlebg0_gfx_size);
    memcpy(&se_mem[30][0], titlebg0_map, titlebg0_map_size);

    REG_BG0CNT = BG_CBB(0) | BG_SBB(30) | BG_8BPP | BG_REG_32x32;
    REG_DISPCNT = DCNT_BG0 | DCNT_MODE0;
}
