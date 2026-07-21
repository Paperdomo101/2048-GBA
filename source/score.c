#include <global.h>

#include <graphics/score.h>

static OBJ_ATTR obj_buffer[128];

static int divisors[7] = {1, 10, 100, 1000, 10000, 100000, 1000000};
static int bg1_hofs[7] = {-13, -9, -6, -3, -1, -4, -2};

static int digit_count, hidigit_count;
static int first_is_1, hifirst_is_1;

static u16 runtime_score_tile_ids     [4] = {512+48, 512+49, 512+50, 512+51};
static u16 runtime_hiscore_tile_ids   [4] = {512+52, 512+53, 512+54, 512+55};
static u16 runtime_score_tile_ids_l   [4] = {512+64, 512+65, 512+66, 512+67};
static u16 runtime_hiscore_tile_ids_l [4] = {512+68, 512+69, 512+70, 512+71};

static struct {
    bool active;
    u32 value;
    int x;
    FIXED anim_timer;
    u8 alpha_table[ANIM_SCORE_DURATION];
} tally = {
    .alpha_table = {
        0, 0, 0, 0, 0, 1, 1, 1, 1,
        2, 2, 3, 3, 4, 5, 6, 8, 9,
        11, 12, 13, 15, 17, 19, 22,
        24, 26, 27, 29, 29, 30, 31
    },
    .x = 204
};


static
void fill_tally_gfx( u32 *tally_buffer, u32 tally, int digit_count, int draw_w, int tile_off )
{
    int offset = 0;
    const int bpp = 4;

    for (int i = 0; i < digit_count; ++i)
    {
        int val = (int)((tally) / divisors[digit_count-i-1]) % 10;

        for (int row = 0; row < 8; ++row)
        {
            u32 src_buff = 0;
            src_buff |= *((int*)(&tile_mem[4][tile_off+val]) + row);

            int index = i;
            if (offset > 0) {
                --index;
                tally_buffer[row + (i * 8)] |= (src_buff >> (bpp * (8-offset)));
            }

            tally_buffer[row + (index * 8)] |= (src_buff << (bpp * offset));
        }

        offset += draw_w;

        if (offset >= 8)
            offset -= 8;
    }
}

static
void fill_score_gfx( u32 *score_buffer, u32 *score_buffer_l, int score, int digit_count, int draw_w, int tile_off )
{
    int offset = 0;
    int rollovers = 0;
    const int bpp = 4;

    for (int i = 0; i < digit_count; ++i)
    {
        int val = (int)((score) / divisors[digit_count-i-1]) % 10;

        for (int row = 0; row < 8; ++row)
        {
            u32 src_buffer = 0;
            u32 src_buffer_len = 0;

            src_buffer |= *((int*)(&tile_mem[1][tile_off+val]) + row);

            if (digit_count < 5) // Lower half of big numbers
                src_buffer_len |= *((int*)(&tile_mem[1][tile_off+16+val]) + row);

            int index = i - rollovers;

            if (offset > 0) {
                --index;
                score_buffer[row + (i * 8)] |= (src_buffer >> (bpp * (8-offset)));

                if (digit_count < 5) // Lower half of big numbers
                    score_buffer_l[row + (i * 8)] |= (src_buffer_len >> (bpp * (8-offset)));
            }

            score_buffer[row + (index * 8)] |= (src_buffer << (bpp * offset));

            if (digit_count < 5) // Lower half of big numbers
                score_buffer_l[row + (index * 8)] |= (src_buffer_len << (bpp * offset));
        }

        offset += draw_w;

        if (offset >= 8) {
            offset -= 8;

            if (offset == 0)
                ++rollovers;
        }
    }
}

static
void hi_score_bg1_hoffset( void )
{
    REG_BG1VOFS = hidigit_count < 5 ? -3 : -4;
    REG_BG1HOFS = bg1_hofs[hidigit_count-1] + score_starts_with_1( true );
}

void init_score( void )
{
    BFN_SET( REG_DISPSTAT, 36, DSTAT_VCT );
    irq_set( II_VCOUNT, hi_score_bg1_hoffset, 1 );

    memcpy( &tile_mem[1][0], scoreTiles, scoreTilesLen );

	memcpy16( &se_mem[31][89], runtime_score_tile_ids, 4 );
	memcpy16( &se_mem[31][89+32], runtime_score_tile_ids_l, 4 );
	memcpy16( &se_mem[31][89+128], runtime_hiscore_tile_ids, 4 );
	memcpy16( &se_mem[31][89+128+32], runtime_hiscore_tile_ids_l, 4 );
}

void reset_score( void )
{
    state.score = 0;
    memset32( &tile_mem[1][48], 0, 4 * 8 );
    memset32( &tile_mem[1][64], 0, 4 * 8 );
}

void update_score( void )
{
    if (state.score > state.hiscore)
        state.hiscore = state.score;

    int tile_id = 16;
    // int draw_y = 19;
    int draw_w = 7;

    first_is_1 = state.score == 1;
    digit_count = 1;
    if (state.score > 9) { ++digit_count; first_is_1 = state.score < 20; }
    if (state.score > 99) { ++digit_count; first_is_1 = state.score < 200; }
    if (state.score > 999) { ++digit_count; first_is_1 = state.score < 2000; }
    if (state.score > 9999) { ++digit_count; first_is_1 = state.score < 20000; draw_w = 6; tile_id = 5; } //y_off = 1;
    if (state.score > 99999) { ++digit_count; first_is_1 = state.score < 200000; draw_w = 4; } //y_off = 2; tile_id = 208;

    REG_BG1VOFS = digit_count < 5 ? -3 : -4;
    REG_BG1HOFS = bg1_hofs[digit_count-1] + first_is_1;

    #define DIGITS_TILE_COUNT (4)

    u32 score_buffer[DIGITS_TILE_COUNT * 8] = {0};
    u32 score_buffer_l[DIGITS_TILE_COUNT * 8] = {0};

    fill_score_gfx( score_buffer, score_buffer_l, state.score, digit_count, draw_w, tile_id );

    memcpy32( &tile_mem[1][48], &score_buffer,   DIGITS_TILE_COUNT * 8 );
    memcpy32( &tile_mem[1][64], &score_buffer_l, DIGITS_TILE_COUNT * 8 );

    tile_id = 16;
    draw_w = 7;

    hifirst_is_1 = state.hiscore == 1;
    hidigit_count = 1;

    if (state.hiscore > 9) { ++hidigit_count; hifirst_is_1 = state.hiscore < 20; }
    if (state.hiscore > 99) { ++hidigit_count; hifirst_is_1 = state.hiscore < 200; }
    if (state.hiscore > 999) { ++hidigit_count; hifirst_is_1 = state.hiscore < 2000; }
    if (state.hiscore > 9999) { ++hidigit_count; hifirst_is_1 = state.hiscore < 20000; draw_w = 6; tile_id = 5; }
    if (state.hiscore > 99999) { ++hidigit_count; hifirst_is_1 = state.hiscore < 200000; draw_w = 4; }


    memset32( score_buffer,   0, DIGITS_TILE_COUNT * 8 );
    memset32( score_buffer_l, 0, DIGITS_TILE_COUNT * 8 );

    fill_score_gfx( score_buffer, score_buffer_l, state.hiscore, hidigit_count, draw_w, tile_id );

    memcpy32( &tile_mem[1][52], &score_buffer,   DIGITS_TILE_COUNT * 8 );
    memcpy32( &tile_mem[1][68], &score_buffer_l, DIGITS_TILE_COUNT * 8 );
}


inline
bool score_starts_with_1( bool is_hiscore )
{
    return is_hiscore ? hifirst_is_1 : first_is_1;
}

inline
int get_digit_count( void )
{
    return digit_count;
}

inline
int get_bg1_offset( void )
{
    return bg1_hofs[digit_count - 1];
}

inline
void reset_tally( void )
{
    tally.value = 0;
}

inline
void add_to_tally( u32 amount )
{
    tally.value += amount;
}

inline
void add_tally_to_score( void )
{
    state.score += tally.value;
}

inline
void hide_tally_particle( void )
{
    obj_hide( &obj_buffer[18] );
}

void spawn_tally_particle( void )
{
    memset32( &tile_mem[4][221], 0, 3 * 8 );
    u32 tally_buffer[4*8] = {0};

    int digit_count = 1;
    if (tally.value > 9) ++digit_count;
    if (tally.value > 99) ++digit_count;
    if (tally.value > 999) ++digit_count;

    fill_tally_gfx( tally_buffer, tally.value, digit_count, 6, 209 );

    memcpy32( &tile_mem[4][221], &tally_buffer, digit_count * 8 );

    tally.x = 208 - (digit_count == 2 ? 4
                   : digit_count == 3 ? 6
                   : digit_count == 4 ? 8 : 0);

    obj_set_attr( &obj_buffer[18], ATTR0_WIDE, ATTR1_SIZE_32x16, ATTR2_PALBANK(2) | 220 );
    obj_set_pos( &obj_buffer[18], tally.x, 20 );

    obj_copy( obj_mem+18, &obj_buffer[18], 1 );

    tally.anim_timer = 0;
    tally.active = true;

}

void update_tally_particle( void )
{
    if (!tally.active)
        return;

    if (tally.anim_timer >> 8 >= ANIM_SCORE_DURATION) {
        tally.active = false;
        obj_hide( &obj_buffer[18] );
    }

    obj_set_pos( &obj_buffer[18], tally.x, 20 - (tally.anim_timer >> 8) );
    obj_copy( obj_mem+18, &obj_buffer[18], 1 );

    tally.anim_timer += 280;

    clr_fade(
        &pal_obj_mem[47],
        pal_obj_mem[44],
        &pal_obj_mem[45],
        1,
        tally.alpha_table[tally.anim_timer >> 8]
    );
}
