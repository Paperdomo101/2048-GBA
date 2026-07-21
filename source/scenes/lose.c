#include <global.h>

#include <graphics/gameover.h>

static int fade_timer = 0;
static int fade_ended = false;

static OBJ_ATTR obj_buffer[18];

static OBJ_ATTR *obj_game = &obj_buffer[0];
static OBJ_ATTR *obj_over = &obj_buffer[1];

void init_scene_lose( void )
{
    fade_ended = false;
    fade_timer = TITLE_FADE_DURATION;

    REG_BG1VOFS = get_digit_count() < 5 ? -3 : -4;
    REG_BG1HOFS = get_bg1_offset() + score_starts_with_1( false );

    memcpy( &tile_mem[4][256], gameoverTiles, gameoverTilesLen );

    /*------------------------------------------------------
                          prioritize OAM slots 0 and 1 for |
                          game over text so it's drawn in  |
                          front of the number tiles       */
    obj_copy( obj_buffer + 2, obj_mem, 16 );
    obj_game = &obj_buffer[0];
    obj_over = &obj_buffer[1];

    obj_hide( obj_game );
    obj_hide( obj_over );

    mmEffectEx( &sfx.lose );
}

void update_scene_lose( void )
{
    update_tally_particle();

    state.seed += rand() << 1;

    REG_BG1VOFS = get_digit_count() < 5 ? -3 : -4;
    REG_BG1HOFS = get_bg1_offset() + score_starts_with_1( false );

    if (fade_timer > 0)
    {
        int a = (7 - fade_timer);
        if (a < 0) a = 0;

        clr_fade_fast( pal_bg_mem, pal_bg_mem [11], pal_bg_mem,  5,  a );
        clr_fade_fast( pal_obj_mem, pal_bg_mem[11], pal_obj_mem, 32, a );
        --fade_timer;

        if (fade_timer == 0)
        {
            fade_ended = 1;

            obj_set_attr( obj_game,
                ATTR0_WIDE,
                ATTR1_SIZE_64x32,
                ATTR2_PALBANK( 2 ) | 256
            );

            obj_set_attr( obj_over,
                ATTR0_WIDE,
                ATTR1_SIZE_64x32,
                ATTR2_PALBANK( 2 ) | (256 + 32)
            );

            obj_set_pos( obj_game, 54, 69 );
            obj_set_pos( obj_over, 54 + 64, 69 );
        }
    }

    if (fade_ended)
    {
        if (key_hit( KEY_SELECT | KEY_A | KEY_B | KEY_START ))
        {
            srand( state.seed );

            obj_hide( obj_game );
            obj_hide( obj_over );
            memset16( pal_bg_mem,  pal_bg_mem[5], 256 );
            memset16( pal_obj_mem, pal_bg_mem[5], 256 );

            state.saved = 0;
            save_state();

            set_scene( SCENE_GAME );
        }
    }

    obj_copy( obj_mem, obj_buffer, 18 );
}
