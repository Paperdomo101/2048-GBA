#include <global.h>

#include <graphics/savescreen.h>

static int fade_timer = 0;
static int fade_ended = false;

void init_scene_save( void )
{
    fade_ended = false;
    fade_timer = TITLE_FADE_DURATION;

    memcpy( &tile_mem[0][0], savescreenTiles, savescreenTilesLen );
	memcpy( &se_mem [30][0], savescreenMap,   savescreenMapLen   );

    REG_DISPCNT = DCNT_BG0 | DCNT_MODE0;

    save_state_including_squares();

    /*--------------------------------------------------
                score particle becomes corrupted after |
                saving for some reason: fix for now is |
                to simply hide it                      |
                                                      */
    hide_tally_particle();

    state.saved = true;

    mmEffectEx( &sfx.save );
}

void update_scene_save( void )
{
    if (fade_timer > 0)
    {
        clr_fade_fast(
            &pal_bg_mem[0],
            pal_bg_mem[5],
            &pal_bg_mem[0],
            256,
            TITLE_FADE_DURATION - fade_timer
        );

        --fade_timer;

        if (fade_timer == 0)
            fade_ended = true;
    }

    if (fade_ended)
    {
        memset16( pal_bg_mem,  pal_bg_mem[5], 256 );
        memset16( pal_obj_mem, pal_bg_mem[5], 256 );

        set_scene( SCENE_GAME );
    }
}
