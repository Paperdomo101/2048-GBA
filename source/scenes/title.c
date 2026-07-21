#include <global.h>

static int fade_timer = 0;
static int fade_ended = false;

void init_scene_title( void )
{
    fade_ended = false;
    fade_timer = 0;
}

void update_scene_title( void )
{
    state.seed += rand() << 1;

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
    else if (key_hit( KEY_A | KEY_B | KEY_START | KEY_SELECT ))
    {
        mmEffectEx( &sfx.start );
        fade_timer = TITLE_FADE_DURATION;
    }

    if (fade_ended)
    {
        srand( state.seed );

        memset16( pal_bg_mem,  pal_bg_mem[5], 256 );
        memset16( pal_obj_mem, pal_bg_mem[5], 256 );

        set_scene( SCENE_GAME );
    }
}
