#include "global.h"
#include <string.h>
#include "titlebg0.h"
#include "tonc_types.h"
#include "tonc_video.h"

static int fade_timer = 0;
static int fade_over = 0;


void InitFirst(void) {
    fade_over = 0;
    fade_timer = FIRST_FADE_DURATION;

    pal_gradient_ex(pal_bg_mem, 0, 15, 0x7bff, 0x7bff);

    memcpy(&tile_mem[0][0], titlebg0Tiles, titlebg0TilesLen);
	memcpy(&se_mem[30][0], titlebg0Map, titlebg0MapLen);

    REG_BG0CNT = BG_CBB(0) | BG_SBB(30) | BG_8BPP | BG_REG_32x32;
    REG_DISPCNT = DCNT_BG0 | DCNT_MODE0;
}

void UpdateFirst(void) {
    vid_vsync();

    if (fade_timer > 0) {
        clr_blend(&pal_bg_mem[0], (COLOR *)titlebg0Pal, &pal_bg_mem[0], 256, (FIRST_FADE_DURATION - fade_timer) << 1);
        --fade_timer;
        if (fade_timer == 0) {
            fade_over = 1;
        }
    }

    if (fade_over) {
        memcpy(pal_bg_mem, titlebg0Pal, titlebg0PalLen);
        SetMode(GM_TITLE);
    }
}
