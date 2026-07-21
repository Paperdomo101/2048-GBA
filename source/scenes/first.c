#include <global.h>

#include <graphics/titlebg0.h>

static int fade_timer = false;
static int fade_ended  = false;

void init_scene_first( void )
{
    fade_ended  = false;
    fade_timer = FIRST_FADE_DURATION;

    pal_gradient_ex( pal_bg_mem, 0, 15, 0x7bff, 0x7bff );

    memcpy( &tile_mem[0][0], titlebg0Tiles, titlebg0TilesLen );
	memcpy( &se_mem [30][0], titlebg0Map,   titlebg0MapLen   );

    REG_BG0CNT  = BG_CBB(0) | BG_SBB(30) | BG_8BPP | BG_REG_32x32;
    REG_DISPCNT = DCNT_BG0  | DCNT_MODE0;
}

void update_scene_first( void )
{
    if (fade_timer > 0)
    {
        clr_blend( &pal_bg_mem[0],
            (COLOR *)titlebg0Pal,
            &pal_bg_mem[0],
            256,
            (FIRST_FADE_DURATION - fade_timer) << 1
        );

        --fade_timer;

        if (fade_timer == 0)
            fade_ended = true;
    }

    if (fade_ended)
    {
        memcpy( pal_bg_mem, titlebg0Pal, titlebg0PalLen );
        set_scene( SCENE_TITLE );
    }
}
