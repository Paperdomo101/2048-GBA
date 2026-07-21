#include <global.h>

#include <graphics/bg0.h>
#include <graphics/sprites.h>

typedef enum {
    UP    = 1,
    DOWN  = 2,
    LEFT  = 3,
    RIGHT = 4
} Directions;

static OBJ_ATTR obj_buffer[128];
static OBJ_AFFINE *obj_aff_buffer = (OBJ_AFFINE *)obj_buffer;

static Square *squares;
static int points[13] = {2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192};

static int fade_timer = 0;
static int fade_ended = 0;

static int win_freeze_timer = 0;
static int lose_freeze_timer = 0;

static u16 empty[16] = {0};
static int empty_len;

static int should_play_slide_sound = 0;
static int should_play_spawn_sound = 0;
static int merge_sound_to_play = 0;
static int spawn_random_square = 0;

static int last_slide_direction = 0;
static int slide_direction = 0;
static int input_delay = 0;

static int anim_frame = 0;
static int anim_duration = ANIM_SLIDE_DURATION;

static Vector2_int origin = {32, -6};

static u8 anim_merge_scale[ANIM_MERGE_DURATION] = {3, 9, 14, 17, 18, 19, 18, 17, 16};
static u8 anim_spawn_scale[ANIM_SPAWN_DURATION] = {1, 3, 6, 8, 11, 13, 14, 15, 16};
static u8 anim_spawn_alpha[ANIM_SPAWN_DURATION] = {15, 30, 60, 90, 130, 160, 190, 220, 255};

static u8 anim_slide[3][ANIM_SLIDE_DURATION] = {
    {6, 17, 26, 33, 36},
    {17, 33, 54, 69, 72},
    {34, 63, 90, 105, 108},
};

static enum {
    SCALE_ANIM_NULL,
    SCALE_ANIM_SPAWN,
    SCALE_ANIM_MERGE,
} scale_anim[16];

static int scale_frame[16]; // 0..8 frames

static
void calculate_used( void )
{
    int used = 0;

    for (int j = 0; j < 4; ++j)
    for (int i = 0; i < 4; ++i)
    {
        int index = i + (j << 2);

        if (squares[index].value != 0)
        {
            ++used;
            continue;
        }

        empty[index - used] = index;
    }

    empty_len = 16 - used;
}

static
void set_square( u8 index, Square square )
{
    squares[index] = square;

    if (square.value == 0)
    {
        ++empty_len;
        empty[index] = index;
    }
    else --empty_len;
}

static
void set_empty_square( u8 index )
{
    set_square( index, (Square){ 0 } );
}

static
bool add_random_square( void )
{
    if (empty_len <= 0)
        return false;

    int randex = rand() % empty_len;
    int index = empty[randex];

    set_square( index,
        (Square) {
            .value = (rand() % 100 < 10) + 1,
            .fresh = true,
            .shift = 0,
            .merge = 0
        }
    );

    scale_anim [index] = SCALE_ANIM_SPAWN;
    scale_frame[index] = 0;

    int tile_id = (squares[index].value - 1) << 4;
    OBJ_ATTR *square = &obj_buffer[index];

    obj_set_attr( square,
        ATTR0_SQUARE     | ATTR0_AFF_DBL,
        ATTR1_SIZE_32x32 | ATTR1_AFF_ID(index),
        ATTR2_PALBANK(1) | tile_id
    );

    int x = index % 4;
    int y = index >> 2;
    obj_set_pos( square, (x << 5) + (x << 2) + origin.x, (y << 5) + (y << 2) + origin.y );

    if (randex < 15)
        for (int i = 0; i < empty_len - randex; i++)
            empty[randex + i] = empty[randex + i + 1];

    empty[empty_len] = randex;
    return true;
}

static
bool is_move_possible( void )
{
    if (empty_len > 0) return true;

    for (int j = 0; j < 4; j++)
    for (int i = 0; i < 4; i++)
    {
        int index = i + (j << 2);

        if (squares[index].value == 0) return true;

        if (j > 0) {
            int up = i + ((j - 1) << 2);
            if (squares[index].value == squares[up].value)
                return true;
        }
        if (i > 0) {
            int left = (i - 1) + (j << 2);
            if (squares[index].value == squares[left].value)
                return true;
        }
        if (i < 3) {
            int right = (i + 1) + (j << 2);
            if (squares[index].value == squares[right].value)
                return true;
        }
        if (j < 3) {
            int down = i + ((j + 1) << 2);
            if (squares[index].value == squares[down].value)
                return true;
        }
    }

    return false;
}

static
bool slide_and_merge( u8 i0, u8 i1, u8 i2, u8 i3 )
{
    bool success = false;
    Square working[4] = {0};

    working[0].value = squares[i0].value;
    working[1].value = squares[i1].value;
    working[2].value = squares[i2].value;
    working[3].value = squares[i3].value;
    squares[i0].shift = 0;
    squares[i1].shift = 0;
    squares[i2].shift = 0;
    squares[i3].shift = 0;
    squares[i0].fresh = 0;
    squares[i1].fresh = 0;
    squares[i2].fresh = 0;
    squares[i3].fresh = 0;
    squares[i0].merge = 0;
    squares[i1].merge = 0;
    squares[i2].merge = 0;
    squares[i3].merge = 0;

    int d = 0;
    int p = 0;
    int p1 = 0;

    for (int i = 0; i < 4; ++i)
    {
        p  = i - d;
        p1 = d - 1;

        if (working[i].value == 0 || i <= 0)
        {
            ++d;
            continue;
        }

        if (working[p].value == 0)
        {
            working[p].value = working[i].value;
            working[i].shift = d;

            if ((i < 3 && working[i+1].value == working[i].value)
             || (i < 2 && working[i+2].value == working[i].value))
                ++d;

            working[i].value = 0;
        }
        else if (working[p].value == working[i].value)
        {
            ++working[p].value;
            working[p].merge = true;

            add_to_tally( points[working[p].value-1] );

            if (merge_sound_to_play < working[p].value)
                merge_sound_to_play = working[p].value;

            working[i].value = 0;
            working[i].shift = d;
        }
        else if (working[i - p1].value == 0)
        {
            working[i - p1].value = working[i].value;
            working[i].value = 0;
            working[i].shift = p1;
        }
    }

    if (squares[i0].value != working[0].value
     || squares[i1].value != working[1].value
     || squares[i2].value != working[2].value
     || squares[i3].value != working[3].value)
    {
        squares[i0] = working[0];
        squares[i1] = working[1];
        squares[i2] = working[2];
        squares[i3] = working[3];
        success = true;
    }

    return success;
}

static inline
bool slide( Directions direction )
{
    bool success = false;

    reset_tally();

    if (input_delay > 0)
        return success;

    switch (direction) {
        case UP: {
            for (int i = 0; i < 4; ++i)
            {
                if (!slide_and_merge( i, (1<<2) + i, (2<<2) + i, (3<<2) + i ))
                    continue;

                success = true;
            }
        } break;
        case DOWN: {
            for (int i = 3; i >= 0; --i)
            {
                if (!slide_and_merge( (3<<2) + i, (2<<2) + i, (1<<2) + i, i ))
                    continue;

                success = true;
            }
        } break;
        case LEFT: {
            for (int i = 0; i < 4; ++i)
            {
                int row = i << 2;

                if (!slide_and_merge( 0 + row, 1 + row, 2 + row, 3 + row ))
                    continue;

                success = true;
            }
        } break;
        case RIGHT: {
            for (int i = 0; i < 4; ++i)
            {
                int row = i << 2;

                if (!slide_and_merge( 3 + row, 2 + row, 1 + row, 0 + row ))
                    continue;

                success = true;
            }
        } break;
    };

    if (merge_sound_to_play)
    {
        spawn_tally_particle();
        add_tally_to_score();
    }

    calculate_used();

    if (success)
    {
        should_play_slide_sound = true;

        if (last_slide_direction != direction)
            anim_frame = 0;

        last_slide_direction = direction;
        input_delay = 5;
    }

    slide_direction = direction;

    return success;
}

static inline
void update_squares( void )
{
    for (int i = 0; i < 16; ++i)
    {
        obj_hide( &obj_buffer[i] );
        int val = squares[i].value;

        if (val <= 0)
            continue;

        OBJ_AFFINE *oaff = &obj_aff_buffer[i];

        if (squares[i].merge)
        {
            scale_anim [i]   = SCALE_ANIM_MERGE;
            scale_frame[i]   = 0;
            squares[i].merge = 0;
        }

        switch (scale_anim[i])
        {
            case SCALE_ANIM_NULL: {
                obj_aff_identity( oaff );
            } break;

            case SCALE_ANIM_SPAWN: {
                clr_blend_fast(
                    &pal_obj_mem[32],
                    &pal_obj_mem[0],
                    &pal_obj_mem[17],
                    6,
                    anim_spawn_alpha[scale_frame[i]] >> 3
                );
                FIXED scale = anim_spawn_scale[scale_frame[i]] << 4;
                obj_aff_scale_inv( oaff, scale, scale );
            } break;

            case SCALE_ANIM_MERGE: {
                FIXED scale = anim_merge_scale[scale_frame[i]] << 4;
                obj_aff_scale_inv( oaff, scale, scale );
            } break;
        }

        int tile_id = (val - 1) << 4;

        OBJ_ATTR *square = &obj_buffer[i];

        obj_set_attr( square,
            ATTR0_SQUARE | ATTR0_AFF_DBL,
            ATTR1_SIZE_32x32 | ATTR1_AFF_ID( i ),
            ATTR2_PALBANK( squares[i].fresh || val > 7 ) | tile_id
        );

        int x = i % 4;
        int y = i >> 2;

        obj_set_pos( square,
            (x << 5) + (x << 2) + origin.x,
            (y << 5) + (y << 2) + origin.y
        );

        if (val == 11)
            win_freeze_timer = 1;
    }

    state.saved = true;
}

static inline
void animate_scale( int i )
{
    switch (scale_anim[i])
    {
        case SCALE_ANIM_SPAWN: {

            if (scale_frame[i] < ANIM_SPAWN_DURATION)
            {
                clr_blend_fast(
                    &pal_obj_mem[32],
                    &pal_obj_mem[0],
                    &pal_obj_mem[17],
                    6,
                    anim_spawn_alpha[scale_frame[i]] >> 3
                );

                FIXED scale = anim_spawn_scale[scale_frame[i]] << 4;
                obj_aff_scale_inv( &obj_aff_buffer[i], scale, scale );
            }

            ++scale_frame[i];

            if (scale_frame[i] >= ANIM_SPAWN_DURATION)
            {
                scale_anim [i] = SCALE_ANIM_NULL;
                scale_frame[i] = -1;
                obj_aff_identity( &obj_aff_buffer[i] );
            }
        } break;

        case SCALE_ANIM_MERGE: {
            if (scale_frame[i] < ANIM_MERGE_DURATION)
            {
                FIXED scale = anim_merge_scale[scale_frame[i]] << 4;
                obj_aff_scale_inv( &obj_aff_buffer[i], scale, scale );
            }

            ++scale_frame[i];

            if (scale_frame[i] >= ANIM_MERGE_DURATION)
            {
                scale_anim[i] = SCALE_ANIM_NULL;
                scale_frame[i] = -1;
                obj_aff_identity( &obj_aff_buffer[i] );
            }
        } break;

        default: break;
    }
}

static inline
void animate_squares( void )
{
    for (int i = 0; i < 16; i++)
    {
        animate_scale( i );
    }

    if (anim_frame >= anim_duration)
    {
        if (anim_duration == ANIM_SLIDE_DURATION)
        {
            /*-------------------------------------------------
                                        transfer active scale |
                                        animation frames from |
                                        source slots to dest  |
                                        slots                */
            int next_scale_frame[16];
            int next_scale_type [16];

            for (int k = 0; k < 16; ++k)
            {
                next_scale_frame[k] = -1;
                next_scale_type [k] = SCALE_ANIM_NULL;
            }

            for (int k = 0; k < 16; ++k)
            {
                if (squares[k].shift)
                    continue;

                next_scale_frame[k] = scale_frame[k];
                next_scale_type [k] = scale_anim[k];
            }

            for (int k = 0; k < 16; ++k)
            {
                if (squares[k].shift)
                {
                    int shift = squares[k].shift;
                    int dest = k;

                    switch (slide_direction)
                    {
                        case UP:    dest = k - (shift << 2); break;
                        case DOWN:  dest = k + (shift << 2); break;
                        case LEFT:  dest = k - shift;        break;
                        case RIGHT: dest = k + shift;        break;
                    }

                    if (dest >= 0 && dest < 16)
                    {
                        next_scale_frame[dest] = scale_frame[k];
                        next_scale_type [dest] = scale_anim [k];
                    }
                }
            }

            for (int k = 0; k < 16; ++k)
            {
                scale_frame[k] = next_scale_frame[k];
                scale_anim [k] = next_scale_type [k];
            }
        }

        for (int i = 0; i < 16; ++i)
        {
            if (squares[i].value <= 0)
                continue;

            OBJ_AFFINE *oaff = &obj_aff_buffer[i];

            if (scale_anim[i] == SCALE_ANIM_NULL)
                obj_aff_identity( oaff );

            int tile_id = (squares[i].value - 1) << 4;

            OBJ_ATTR *square = &obj_buffer[i];

            obj_set_attr( square,
                ATTR0_SQUARE | ATTR0_AFF_DBL,
                ATTR1_SIZE_32x32 | ATTR1_AFF_ID( i ),
                ATTR2_PALBANK( squares[i].value > 7 ) | tile_id
            );

            int x = i % 4;
            int y = i >> 2;

            obj_set_pos( square,
                (x << 5) + (x << 2) + origin.x,
                (y << 5) + (y << 2) + origin.y
            );

            if (anim_duration == ANIM_SLIDE_DURATION)
                squares[i].shift = 0;
            else {
                squares[i].merge = 0;
                squares[i].fresh = 0;
            }
        }
        return;
    }

    if (anim_duration == ANIM_SLIDE_DURATION)
    {
        for (int i = 0; i < 16; ++i)
        {
            if (squares[i].shift == 0)
                continue;

            int x = i % 4;
            int y = i >> 2;
            int ix = (x << 5) + (x << 2) + origin.x;
            int iy = (y << 5) + (y << 2) + origin.y;

            int anim_x = 0;
            int anim_y = 0;

            switch (slide_direction)
            {
                case UP:    anim_y = -anim_slide[squares[i].shift-1][anim_frame]; break;
                case LEFT:  anim_x = -anim_slide[squares[i].shift-1][anim_frame]; break;
                case DOWN:  anim_y =  anim_slide[squares[i].shift-1][anim_frame]; break;
                case RIGHT: anim_x =  anim_slide[squares[i].shift-1][anim_frame]; break;
            }
            obj_set_pos( &obj_buffer[i], ix + anim_x, iy + anim_y );
        }

        ++anim_frame;
        if (anim_frame >= anim_duration)
        {
            anim_duration = ANIM_SPAWN_DURATION;
            anim_frame = 0;
            update_squares();
        }
    }
    else {
        ++anim_frame;
    }
}

static inline
void slide_in_direction( Directions direction )
{
    if (!slide( direction ))
        return;

    anim_frame = 0;
    anim_duration = ANIM_SLIDE_DURATION;
    spawn_random_square = true;
}

static inline
int get_signed_y( const OBJ_ATTR *obj )
{
    int y = obj->attr0 & ATTR0_Y_MASK;

    if (y >= 160)
        y -= 256;

    return y;
}

static inline
int get_signed_x( const OBJ_ATTR *obj )
{
    int x = obj->attr1 & ATTR1_X_MASK;

    if (x >= 240)
        x -= 512;

    return x;
}

static inline
void sort_squares( void )
{
    OBJ_ATTR sorted[16];
    int count = 0;

    for (int i = 0; i < 16; ++i)
        if (!OBJECT_IS_HIDDEN( obj_buffer[i] ))
            sorted[count++] = obj_buffer[i];

    /*--------------------------------------------------
                                  sort visible sprites |
                                  based on the slide   |
                                  direction           */
    int coord_i = 0;
    int coord_j = 0;

    for (int i = 0; i < count - 1; ++i)
    for (int j = i + 1; j < count; ++j)
    {
        if (slide_direction == DOWN || slide_direction == UP)
        {
            coord_i = get_signed_y( &sorted[i] );
            coord_j = get_signed_y( &sorted[j] );
        }
        else {
            coord_i = get_signed_x( &sorted[i] );
            coord_j = get_signed_x( &sorted[j] );
        }

        bool swap = false;

        if (slide_direction == DOWN || slide_direction == RIGHT)
            swap = (coord_i < coord_j);
        else
            swap = (coord_i > coord_j);

        if (swap)
        {
            OBJ_ATTR temp = sorted[i];
            sorted[i] = sorted[j];
            sorted[j] = temp;
        }
    }

    for (int i = 0; i < 16; ++i)
        if (OBJECT_IS_HIDDEN( obj_buffer[i] ))
            sorted[count++] = obj_buffer[i];

    obj_copy( obj_mem, sorted, 16 );
}

static inline
void play_merge_sound( void )
{
    mm_sound_effect *sound = merge_sound_to_play <= 3 ? &sfx.small_merge
                           : merge_sound_to_play <= 6 ? &sfx.merge
                           : merge_sound_to_play <= 8 ? &sfx.big_merge
                           : &sfx.huge_merge;

    mm_sfxhand handle = mmEffectEx( sound );

    if (sound == &sfx.big_merge)
    {
        int pitch_lut[3] = { 0, 128, 256+80 };
        int pitch_off = merge_sound_to_play - 7;
        mmEffectScaleRate( handle, 1024 + pitch_lut[pitch_off] );
    }
    else if (sound == &sfx.huge_merge)
    {
        int pitch_lut[3] = { 0, 128, 256+80 };
        int pitch_off = merge_sound_to_play - 9;
        mmEffectScaleRate( handle, 1024 + pitch_lut[pitch_off] );
    }

    merge_sound_to_play = 0;
}

static
void reset_game( void )
{
    win_freeze_timer  = 0;
    lose_freeze_timer = 0;

    empty_len = 0;
    srand( rand()
        + state.squares[1].value
        + state.squares[2].value
        + state.squares[4].value
        + state.squares[5].value
        + state.squares[7].value
        + state.squares[9].value
        + state.squares[12].value
        + state.squares[13].value
        + state.score );

    for (int j = 0; j < 16; j+=4)
        for (int i = 0; i < 4; ++i)
            set_empty_square( i + j );

    obj_hide_multi( obj_buffer, 16 );

    reset_score();
    update_score();

    anim_frame = 0;
    anim_duration = ANIM_SPAWN_DURATION;

    add_random_square();
    add_random_square();
    update_squares();

    should_play_spawn_sound = true;
}

void init_scene_game( void )
{
    squares = state.squares;

    win_freeze_timer  = 0;
    lose_freeze_timer = 0;

    init_score();

    fade_ended = false;
    fade_timer = GAME_FADE_DURATION;

	memcpy( &tile_mem[0][0], bg0Tiles, bg0TilesLen );
	memcpy( &se_mem[30][0], bg0Map, bg0MapLen );

	memcpy( &tile_mem[4][0], spritesTiles, spritesTilesLen );
	oam_init( obj_buffer, 128 );

	REG_BG1VOFS = -3;

    REG_BG0CNT  = BG_CBB( 0 ) | BG_SBB( 30 ) | BG_4BPP  | BG_REG_32x32 | BG_PRIO( 1 );
    REG_BG1CNT  = BG_CBB( 0 ) | BG_SBB( 31 ) | BG_4BPP  | BG_REG_32x32 | BG_PRIO( 0 );
    REG_DISPCNT = DCNT_OBJ    | DCNT_OBJ_1D  | DCNT_BG0 | DCNT_MODE0   | DCNT_BG1;

    key_repeat_limits( 20, 55555 );

    memset32( scale_anim,  0, 16 );
    memset32( scale_frame, 0, 16 );

    if (state.saved)
    {
        update_score();
        update_squares();
    }
    else reset_game();
}

void update_scene_game( void )
{
    update_score();
    update_tally_particle();

    if (fade_timer > 0)
    {
        clr_blend_fast(
            &pal_bg_mem[0],
            (COLOR *)bg0Pal,
            &pal_bg_mem[0],
            256,
            (GAME_FADE_DURATION - fade_timer) >> 1
        );

        clr_blend_fast(
            &pal_obj_mem[0],
            (COLOR *)spritesPal,
            &pal_obj_mem[0],
            256,
            (GAME_FADE_DURATION - fade_timer) >> 1
        );

        --fade_timer;

        if (fade_timer == 0 && !fade_ended)
        {
            fade_ended = true;
            memcpy( pal_bg_mem, bg0Pal, bg0PalLen );
            memcpy( pal_obj_mem, spritesPal, spritesPalLen );
        }
    }


    if (!is_move_possible())
        lose_freeze_timer++;

    if (input_delay > 0)
        --input_delay;

    if (fade_ended)
    {
        if (key_hit( KEY_SELECT ))
        {
            pal_bg_mem[22] = pal_bg_mem[24];
            pal_bg_mem[23] = pal_bg_mem[25];
            reset_game();
        }

        if (key_released( KEY_SELECT ))
        {
            pal_bg_mem[22] = pal_bg_mem[5];
            pal_bg_mem[23] = pal_bg_mem[5];
            state.saved = false;
            save_state();
        }

        if (key_hit( KEY_START ))
        {
            pal_bg_mem[38] = pal_bg_mem[40];
            pal_bg_mem[39] = pal_bg_mem[41];
            set_scene( SCENE_SAVE );
        }

        if (key_released( KEY_START ))
        {
            pal_bg_mem[38] = pal_bg_mem[5];
            pal_bg_mem[39] = pal_bg_mem[5];
        }
    }


    if (key_released( KEY_DIR ))
    {
        last_slide_direction = 0;
        calculate_used();
    }

    if (key_repeat( KEY_UP ))    slide_in_direction( UP );
    else
    if (key_repeat( KEY_DOWN ))  slide_in_direction( DOWN );

    if (key_repeat( KEY_LEFT ))  slide_in_direction( LEFT );
    else
    if (key_repeat( KEY_RIGHT )) slide_in_direction( RIGHT );


    if (spawn_random_square && anim_duration != ANIM_SLIDE_DURATION)
    {
        if (add_random_square())
            should_play_spawn_sound = true;

        spawn_random_square = 0;
        update_squares();
    }

    if (merge_sound_to_play)
        play_merge_sound();

    if (should_play_slide_sound)
    {
        mmEffectEx( &sfx.slide );
        should_play_slide_sound = false;
    }

    if (should_play_spawn_sound)
    {
        mmEffectEx( &sfx.spawn );
        should_play_spawn_sound = false;
    }

    animate_squares();

    if (lose_freeze_timer > 80)
        set_scene( SCENE_LOSE );

    if (win_freeze_timer > 0)
    {
        if (win_freeze_timer > 50)
            set_scene( SCENE_WIN );
        else
            ++win_freeze_timer;
    }

#ifdef _DEBUG
    if (key_hit( KEY_L | KEY_R ))
        set_scene( key_hit(KEY_R) ? SCENE_LOSE : SCENE_WIN );
#endif

    sort_squares();

    obj_aff_copy( obj_aff_mem, obj_aff_buffer, 16 );
}
