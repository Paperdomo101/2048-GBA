#include "global.h"
#include <stdlib.h>
#include <string.h>
#include "bg0.h"
#include "score.h"
#include "sprites.h"
#include "tonc_bios.h"
#include "tonc_core.h"
#include "tonc_input.h"
#include "tonc_irq.h"
#include "tonc_memdef.h"
#include "tonc_memmap.h"
#include "tonc_oam.h"
#include "tonc_types.h"
#include "tonc_video.h"

static State *state;
static Assets *assets;

static int fade_timer = 0;
static int fade_over = 0;
static int lose_timer = 0;

static int points[13] = {2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192};

static u16 runtimeScoreTileIDs[4] = {512+48, 512+49, 512+50, 512+51};
static u16 runtimeHiScoreTileIDs[4] = {512+52, 512+53, 512+54, 512+55};
static u16 runtimeScoreTileIDsL[4] = {512+64, 512+65, 512+66, 512+67};
static u16 runtimeHiScoreTileIDsL[4] = {512+68, 512+69, 512+70, 512+71};

OBJ_ATTR obj_buffer[128];
OBJ_AFFINE *obj_aff_buffer = (OBJ_AFFINE *)obj_buffer;

enum Directions { UP = 1, DOWN = 2, LEFT = 3, RIGHT = 4 };

static int divisors[7] = {1, 10, 100, 1000, 10000, 100000, 1000000};
static int bg1_hofs[7] = {-13, -9, -6, -3, -1, -4, -2};
static int digit_count, hidigit_count;
static int first_is_1, hifirst_is_1;

static Square squares[16] = {0};
static u16 empty[16] = {0};
static int empty_len;

static int play_slide_sound = 0;
static int play_spawn_sound = 0;
static int play_merge_sound = 0;
static int spawn_random_square = 0;

static int last_dir = 0;
static int dir = 0;
static int input_delay = 0;
static FIXED new_game_timer = 0;
static int win_timer = 0;
static int you_win = 0;

static int anim_frame = 0;
static int anim_duration = ANIM_SLIDE_DURATION;

static vec2i origin = {32, -6};

static u8 anim_merge_scale[ANIM_MERGE_DURATION] = { 3, 9, 14, 17, 18, 19, 18, 17, 16 };
static u8 anim_spawn_scale[ANIM_SPAWN_DURATION] = { 1, 3, 6, 8, 11, 13, 14, 15, 16 };
static u8 anim_spawn_alpha[ANIM_SPAWN_DURATION] = { 15, 30, 60, 90, 130, 160, 190, 220, 255 };

static u8 anim_slide[3][ANIM_SLIDE_DURATION] = {
    { 6, 17, 26, 33, 36 },
    { 17, 33, 54, 69, 72 },
    { 34, 63, 90, 105, 108 },
};

static FIXED tally_anim_timer = 0;
static u8 anim_tally_alpha[ANIM_SCORE_DURATION] = {0,0,0,0,0,1,1,1,1,2,2,3,3,4,5,6,8,9,11,12,13,15,17,19,22,24,26,27,29,29,30,31};
static int tally_x = 204;


void FillTallyGFX(u32 *tally_buffer, u32 tally, int digit_count, int draw_w, int tile_off) {
    int offset = 0;
    const int bpp = 4;

    for (int i = 0; i < digit_count; ++i) {

        int val = (int)((tally) / divisors[digit_count-i-1]) % 10;

        for (int row = 0; row < 8; ++row) {
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
        if (offset >= 8) {
            offset -= 8;
        }
    }
}

void SpawnScoreParticle(u32 tally) {
    memset32(&tile_mem[4][221], 0, 3 * 8);
    u32 tally_buffer[4*8] = {0};

    int tally_digit_count = 1;
    if (tally > 9) { ++tally_digit_count; }
    if (tally > 99) { ++tally_digit_count; }
    if (tally > 999) { ++tally_digit_count; }

    FillTallyGFX(tally_buffer, tally, tally_digit_count, 6, 209);

    memcpy32(&tile_mem[4][221], &tally_buffer, tally_digit_count*8);

    tally_x = 208 - (tally_digit_count == 2 ? 4 : tally_digit_count == 3 ? 6 : tally_digit_count == 4 ? 8 : 0);
    obj_set_attr(&obj_buffer[18], ATTR0_WIDE, ATTR1_SIZE_32x16, ATTR2_PALBANK(2) | 220);
    obj_set_pos(&obj_buffer[18], tally_x, 20);
    obj_copy(obj_mem+18, &obj_buffer[18], 1);
    tally_anim_timer = 0;

}

void UpdateScoreParticle(void) {
    if (tally_anim_timer >> 8 >= ANIM_SCORE_DURATION) {
        tally_anim_timer = 0;
        obj_hide(&obj_buffer[18]);
    }
    obj_set_pos(&obj_buffer[18], tally_x, 20 - (tally_anim_timer >> 8));
    obj_copy(obj_mem+18, &obj_buffer[18], 1);
    tally_anim_timer += 280;
    clr_fade(&pal_obj_mem[47], pal_obj_mem[44], &pal_obj_mem[45], 1, anim_tally_alpha[tally_anim_timer >> 8]);
}

void FillScoreGFX(u32 *score_buffer, u32 *score_buffer_l, int score, int digit_count, int draw_w, int tile_off) {

    int offset = 0;
    int rollovers = 0;
    const int bpp = 4;

    // int big_off = 0;


    for (int i = 0; i < digit_count; ++i) {

        int val = (int)((score) / divisors[digit_count-i-1]) % 10;

        for (int row = 0; row < 8; ++row) {
            u32 src_buff = 0; u32 src_buffl = 0;
            src_buff |= *((int*)(&tile_mem[1][tile_off+val]) + row);

            if (digit_count < 5) { // Lower half of big numbers
                src_buffl |= *((int*)(&tile_mem[1][tile_off+16+val]) + row);
            }

            int index = i - rollovers;
            if (offset > 0) {
                --index;
                score_buffer[row + (i * 8)] |= (src_buff >> (bpp * (8-offset)));

                if (digit_count < 5) { // Lower half of big numbers
                    score_buffer_l[row + (i * 8)] |= (src_buffl >> (bpp * (8-offset)));
                }
            }

            score_buffer[row + (index * 8)] |= (src_buff << (bpp * offset));

            if (digit_count < 5) { // Lower half of big numbers
                score_buffer_l[row + (index * 8)] |= (src_buffl << (bpp * offset));
            }
        }


        offset += draw_w;
        if (offset >= 8) {
            // big_off += 8;
            offset -= 8;
            if (offset == 0) {
                rollovers ++;
            }
        }
    }
}


void ResetScore(void) {
    state->score = 0;

    memset32(&tile_mem[1][48], 0, 4 * 8);
    memset32(&tile_mem[1][64], 0, 4 * 8);
}

int GetFirstIs1(int hi) {
    return hi ? hifirst_is_1 : first_is_1;
}

int GetDigitCount(int hi) {
    return hi ? hidigit_count : digit_count;
}

int GetBG1Off(int hi) {
    return bg1_hofs[(hi ? hidigit_count : digit_count) - 1];
}

void HiScoreBG1HOffset(void) {
    REG_BG1VOFS = hidigit_count < 5 ? -3 : -4;
    REG_BG1HOFS = bg1_hofs[hidigit_count-1] + GetFirstIs1(1);
}

void UpdateScore(void) {

    if (state->score > state->hiscore) {
        state->hiscore = state->score;
        SaveState(state);
    }

    int tile_id = 16;
    int draw_y = 19;
    int draw_w = 7;
    // int y_off = 0;

    first_is_1 = state->score == 1;
    digit_count = 1;
    if (state->score > 9) { ++digit_count; first_is_1 = state->score < 20; }
    if (state->score > 99) { ++digit_count; first_is_1 = state->score < 200; }
    if (state->score > 999) { ++digit_count; first_is_1 = state->score < 2000; }
    if (state->score > 9999) { ++digit_count; first_is_1 = state->score < 20000; draw_w = 6; ++draw_y; tile_id = 5; } //y_off = 1;
    if (state->score > 99999) { ++digit_count; first_is_1 = state->score < 200000; draw_w = 4; ++draw_y; } //y_off = 2; tile_id = 208;

    // digit_count = 4;

    REG_BG1VOFS = digit_count < 5 ? -3 : -4;
    REG_BG1HOFS = bg1_hofs[digit_count-1] + first_is_1;

    #define DIGITS_TILE_COUNT (4)

    u32 score_buffer[DIGITS_TILE_COUNT*8] = {0};
    u32 score_buffer_l[DIGITS_TILE_COUNT*8] = {0};

    FillScoreGFX(score_buffer, score_buffer_l, state->score, digit_count, draw_w, tile_id);

    memcpy32(&tile_mem[1][48], &score_buffer, DIGITS_TILE_COUNT*8);
    memcpy32(&tile_mem[1][64], &score_buffer_l, DIGITS_TILE_COUNT*8);

    tile_id = 16;
    draw_y = 19;
    draw_w = 7;

    hifirst_is_1 = state->hiscore == 1;
    hidigit_count = 1;
    if (state->hiscore > 9) { ++hidigit_count; hifirst_is_1 = state->hiscore < 20; }
    if (state->hiscore > 99) { ++hidigit_count; hifirst_is_1 = state->hiscore < 200; }
    if (state->hiscore > 999) { ++hidigit_count; hifirst_is_1 = state->hiscore < 2000; }
    if (state->hiscore > 9999) { ++hidigit_count; hifirst_is_1 = state->hiscore < 20000; draw_w = 6; ++draw_y; tile_id = 5; }
    if (state->hiscore > 99999) { ++hidigit_count; hifirst_is_1 = state->hiscore < 200000; draw_w = 4; ++draw_y; }


    memset32(score_buffer, 0, DIGITS_TILE_COUNT*8);
    memset32(score_buffer_l, 0, DIGITS_TILE_COUNT*8);

    FillScoreGFX(score_buffer, score_buffer_l, state->hiscore, hidigit_count, draw_w, tile_id);

    memcpy32(&tile_mem[1][52], &score_buffer, DIGITS_TILE_COUNT*8);
    memcpy32(&tile_mem[1][68], &score_buffer_l, DIGITS_TILE_COUNT*8);

}

void SetSquare(u8 index, Square square) {
    squares[index] = square;
    if (square.value == 0) {
        ++empty_len;
        empty[index] = index;
    } else {
        --empty_len;
    }
}

int AddRandomSquare(void) {
    if (empty_len <= 0) {
        return 0;
    }

    int randex = rand() % empty_len;
    int index = empty[randex];
    SetSquare(index, (Square) { .value=(rand() % 100 < 10) + 1, .fresh = 1, .shift = 0, .merge = 0});

    int tile_id = (squares[index].value - 1) << 4;
    OBJ_ATTR *square = &obj_buffer[index];
    obj_set_attr(square, ATTR0_SQUARE | ATTR0_AFF_DBL, ATTR1_SIZE_32x32 | ATTR1_AFF_ID(index), ATTR2_PALBANK(1) | tile_id);
    // anim_duration = ANIM_SPAWN_DURATION;

    int x = index % 4;
    int y = index >> 2;
    obj_set_pos(square, (x << 5) + (x << 2) + origin.x, (y << 5) + (y << 2) + origin.y);

    if (randex < 15) {
        for (int i = 0; i < empty_len - randex; i++)
        {
            empty[randex + i] = empty[randex + i + 1];
        }
    }
    empty[empty_len] = randex;
    return 1;
}

int CanMove(void) {
    if (empty_len > 0) { return 1; }

/// FIXME: Something's wrong here! (Not gaming over sometimes)
    for (int j = 0; j < 4; j++) {
    for (int i = 0; i < 4; i++)
    {
        int index = i + (j << 2);
        if (squares[index].value == 0) {
            return 1;
        }

        if (j > 0) {
            int up = i + ((j + 1) << 2);
            if (squares[index].value == squares[up].value) {
                return 1;
            }
        }
        if (i > 0) {
            int left = (i - 1) + (j << 2);
            if (squares[index].value == squares[left].value) {
                return 1;
            }
        }
        if (i < 3) {
            int right = (i + 1) + (j << 2);
            if (squares[index].value == squares[right].value) {
                return 1;
            }
        }
        if (j < 3) {
            int down = i + ((j + 1) << 2);
            if (squares[index].value == squares[down].value) {
                return 1;
            }
        }
    }}
    return 0;
};

int SlideAndMerge(u8 i0, u8 i1, u8 i2, u8 i3) {
    int success = 0;
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

    u32 tally = 0;

    int d = 0;
    int p = 0;
    int p1 = 0;
    for (int i = 0; i < 4; ++i) {
        p = i - d;
        p1 = d - 1;

        if (working[i].value == 0 || i <= 0) {
            ++d;
            continue;
        }

        if (working[p].value == 0) {
            working[p].value = working[i].value;
            working[i].shift = d;
            if ((i < 3 && working[i+1].value == working[i].value) || ((i < 2 && working[i+2].value == working[i].value))) {
                ++d;
            }
            working[i].value = 0;
        }
        else if (working[p].value == working[i].value) {
            ++working[p].value;
            working[p].merge = 1;
            tally += points[working[p].value-1];
            play_merge_sound = (working[p].value <= 3 && play_merge_sound < 2) ? 1 : (working[p].value <= 6 && play_merge_sound < 3) ? 2 : 3;
            working[i].value = 0;
            working[i].shift = d;
        }
        else if (working[i - p1].value == 0) {
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
        success = 1;
    }

    if (play_merge_sound && tally > 0) {
        SpawnScoreParticle(tally);

        state->score += tally;
    }

    return success;
}


int Slide(enum Directions direction) {
    int success = 0;

    if (input_delay > 0) {
        return success;
    }

    switch (direction) {
    case UP: {
        for (int i = 0; i < 4; ++i) {
            if (!SlideAndMerge(i, (1<<2) + i, (2<<2) + i, (3<<2) + i)) { continue; }
            success = 1;
        }
    } break;
    case DOWN: {
        for (int i = 3; i >= 0; --i) {
            if (!SlideAndMerge((3<<2) + i, (2<<2) + i, (1<<2) + i, i)) { continue; }
            success = 1;
        }
    } break;
    case LEFT: {
        for (int i = 0; i < 4; ++i) {
            int row = i << 2;
            if (!SlideAndMerge(0 + row, 1 + row, 2 + row, 3 + row)) { continue; }
            success = 1;
        }
    } break;
    case RIGHT: {
        for (int i = 0; i < 4; ++i) {
            int row = i << 2;
            if (!SlideAndMerge(3 + row, 2 + row, 1 + row, 0 + row)) { continue; }
            success = 1;
        }
    } break;
    default: {
    } break;
    };

    int used = 0;
    for (int j = 0; j < 4; j++) {
    for (int i = 0; i < 4; i++)
    {
        int index = i + (j << 2);
        if (squares[index].value != 0) {
            ++used;
            continue;
        }
        empty[index - used] = index;
    }}
    empty_len = 16 - used;

    if (success) {
        play_slide_sound = 1;

        if (last_dir != direction) {
            anim_frame = 0;
        }
        last_dir = direction;
        input_delay = 5;
    }

    dir = direction;

    return success;
}

void UpdateSquares(void) {
    for (int i = 0; i < 16; ++i)
    {
        obj_hide(&obj_buffer[i]);
        int val = squares[i].value;

        if (val <= 0) { continue; }

        OBJ_AFFINE *oaff = &obj_aff_buffer[i];
        obj_aff_identity(oaff);

        int tile_id = (val - 1) << 4;
        OBJ_ATTR *square = &obj_buffer[i];

        obj_set_attr(square, ATTR0_SQUARE | ATTR0_AFF_DBL, ATTR1_SIZE_32x32 | ATTR1_AFF_ID(i), ATTR2_PALBANK(squares[i].fresh || val > 7) | tile_id);

        int x = i % 4;
        int y = i >> 2;
        obj_set_pos(square, (x << 5) + (x << 2) + origin.x, (y << 5) + (y << 2) + origin.y);

        if (val == 11) {
            you_win = 1;
        }
    }
}

void AnimateSquares(void) {

    if (anim_frame >= anim_duration) {
        for (int i = 0; i < 16; i++)
        {
            if (squares[i].value <= 0) { continue; }
            OBJ_AFFINE *oaff = &obj_aff_buffer[i];
            obj_aff_identity(oaff);


            int tile_id = (squares[i].value - 1) << 4;
            OBJ_ATTR *square = &obj_buffer[i];
            obj_set_attr(square, ATTR0_SQUARE | ATTR0_AFF_DBL, ATTR1_SIZE_32x32 | ATTR1_AFF_ID(i), ATTR2_PALBANK(squares[i].value > 7) | tile_id);

            int x = i % 4;
            int y = i >> 2;
            obj_set_pos(square, (x << 5) + (x << 2) + origin.x, (y << 5) + (y << 2) + origin.y);
            if (anim_duration == ANIM_SLIDE_DURATION) {
                squares[i].shift = 0;
            } else {
                squares[i].merge = 0;
                squares[i].fresh = 0;
            }
        }
        return;
    }

    if (anim_duration == ANIM_SLIDE_DURATION) {
        for (int i = 0; i < 16; ++i) {
            if (squares[i].shift == 0) { continue; }

            int x = i % 4;
            int y = i >> 2;
            int ix = (x << 5) + (x << 2) + origin.x;
            int iy = (y << 5) + (y << 2) + origin.y;

            int anim_x = 0;
            int anim_y = 0;
            switch (dir) {
                case UP: {
                    anim_y = -anim_slide[squares[i].shift-1][anim_frame];
                } break;
                case LEFT: {
                    anim_x = -anim_slide[squares[i].shift-1][anim_frame];
                } break;
                case DOWN: {
                    anim_y = anim_slide[squares[i].shift-1][anim_frame];
                } break;
                case RIGHT: {
                    anim_x = anim_slide[squares[i].shift-1][anim_frame];
                } break;
            }
            obj_set_pos(&obj_buffer[i], ix + anim_x, iy + anim_y);
        }

        ++anim_frame;
        if (anim_frame >= anim_duration) {
            anim_duration = ANIM_SPAWN_DURATION;

            anim_frame = 0;
            UpdateSquares();
        }
    } else {
        for (int i = 0; i < 16; i++)
        {
            if (squares[i].value <= 0) { continue; }
            OBJ_AFFINE *oaff = &obj_aff_buffer[i];
            // obj_aff_identity(oaff);

            if (squares[i].fresh) {
                clr_blend_fast(&pal_obj_mem[32], &pal_obj_mem[0], &pal_obj_mem[17], 6, anim_spawn_alpha[anim_frame] >> 3);
                FIXED scale = anim_spawn_scale[anim_frame] << 4;
                obj_aff_scale_inv(oaff, scale , scale );
            }
            else if (squares[i].merge) {
                FIXED scale = anim_merge_scale[anim_frame] << 4;
                obj_aff_scale_inv(oaff, scale , scale );
            }
        }
        ++anim_frame;
    }
}




void InitGame(void) {
    state = GetState();
    assets = GetAssets();

    LoadState(state);


    BFN_SET(REG_DISPSTAT, 36, DSTAT_VCT);
    irq_add(II_VCOUNT, HiScoreBG1HOffset);

    fade_over = 0;
    fade_timer = GAME_FADE_DURATION;

	memcpy(&tile_mem[0][0], bg0Tiles, bg0TilesLen);
	memcpy(&se_mem[30][0], bg0Map, bg0MapLen);

	memcpy(&tile_mem[1][0], scoreTiles, scoreTilesLen);

	memcpy16(&se_mem[31][89], runtimeScoreTileIDs, 4);
	memcpy16(&se_mem[31][89+32], runtimeScoreTileIDsL, 4);
	memcpy16(&se_mem[31][89+128], runtimeHiScoreTileIDs, 4);
	memcpy16(&se_mem[31][89+128+32], runtimeHiScoreTileIDsL, 4);

	memcpy(&tile_mem[4][0], spritesTiles, spritesTilesLen);
	oam_init(obj_buffer, 128);

	REG_BG1VOFS = -3;

    REG_BG0CNT = BG_CBB(0) | BG_SBB(30) | BG_4BPP | BG_REG_32x32 | BG_PRIO(1);
    REG_BG1CNT = BG_CBB(0) | BG_SBB(31) | BG_4BPP | BG_REG_32x32 | BG_PRIO(0);
    REG_DISPCNT = DCNT_OBJ | DCNT_OBJ_1D | DCNT_BG0 | DCNT_MODE0 | DCNT_BG1;

    key_repeat_limits(20, 55555);

    StartGame();

}


void StartGame(void) {
    you_win = 0;
    win_timer = 0;
    new_game_timer = 0;

    empty_len = 0;
    srand(rand()+squares[1].value+squares[2].value+squares[4].value+squares[5].value+squares[7].value+squares[9].value+squares[12].value+squares[13].value + state->score);

    for (int i = 0; i < 16; i+=4)   // Reset arrays
    {
        SetSquare(i, (Square){0});
        obj_hide(&obj_buffer[i]);
        int next = i+1;
        SetSquare(next, (Square){0});
        obj_hide(&obj_buffer[next]);
        next = i+2;
        SetSquare(next, (Square){0});
        obj_hide(&obj_buffer[next]);
        next = i+3;
        SetSquare(next, (Square){0});
        obj_hide(&obj_buffer[next]);
    }

    ResetScore();
    UpdateScore();


    anim_frame = 0;

    AddRandomSquare();
    AddRandomSquare();
    UpdateSquares();

    play_spawn_sound = 1;
}



void UpdateGame(void) {

    vid_vsync();
    UpdateScore();
    UpdateScoreParticle();

    if (fade_timer > 0) {
        clr_blend_fast(&pal_bg_mem[0], (COLOR *)bg0Pal, &pal_bg_mem[0], 256, (GAME_FADE_DURATION - fade_timer) >> 1);
        clr_blend_fast(&pal_obj_mem[0], (COLOR *)spritesPal, &pal_obj_mem[0], 256, (GAME_FADE_DURATION - fade_timer) >> 1);
        --fade_timer;
        if (fade_timer == 0) {
            fade_over = 1;
            memcpy(pal_bg_mem, bg0Pal, bg0PalLen);
            memcpy(pal_obj_mem, spritesPal, spritesPalLen);
        }
    }
    if (!fade_over) {
        return;
    }


    if (!CanMove()) {
        lose_timer++;
    }
    if (input_delay > 0) {
        --input_delay;
    }

    if (!you_win) {
        key_poll();
    }


    if (key_hit(KEY_SELECT)) {
        pal_bg_mem[22] = pal_bg_mem[24];
        pal_bg_mem[23] = pal_bg_mem[25];
        StartGame();
    }
    if (key_is_up(KEY_SELECT)) {
        memcpy(pal_bg_mem, bg0Pal, bg0PalLen);
    }

    if (!key_repeat(KEY_DIR)) {
        last_dir = 0;
    }

    if (key_repeat(KEY_UP)) {
        if (Slide(UP)) {
            anim_frame = 0;
            anim_duration = ANIM_SLIDE_DURATION;
            spawn_random_square = 1;
        }
    }

    if (key_repeat(KEY_DOWN)) {
        if (Slide(DOWN)) {
            anim_frame = 0;
            anim_duration = ANIM_SLIDE_DURATION;
            spawn_random_square = 1;
        }
    }

    if (key_repeat(KEY_LEFT)) {
        if (Slide(LEFT)) {
            anim_frame = 0;
            anim_duration = ANIM_SLIDE_DURATION;
            spawn_random_square = 1;
        }
    }

    if (key_repeat(KEY_RIGHT)) {
        if (Slide(RIGHT)) {
            anim_frame = 0;
            anim_duration = ANIM_SLIDE_DURATION;
            spawn_random_square = 1;
        }
    }

    if (spawn_random_square && anim_duration != ANIM_SLIDE_DURATION) {
        if (AddRandomSquare()) {
            play_spawn_sound = 1;
        }
        spawn_random_square = 0;
        UpdateSquares();
    }

    if (play_merge_sound) {
        mmEffectEx(play_merge_sound == 3 ? &assets->sfx.big_merge : play_merge_sound == 2 ? &assets->sfx.merge : &assets->sfx.small_merge);
        play_merge_sound = 0;
    }
    if (play_slide_sound) {
        mmEffectEx(&assets->sfx.slide);
        play_slide_sound = 0;
    }
    if (play_spawn_sound) {
        mmEffectEx(&assets->sfx.spawn);
        play_spawn_sound = 0;
    }

    AnimateSquares();


    if (lose_timer > 80) {
        SetMode(GM_GAMEOVER);
        lose_timer = 0;
        mmEffectEx(&assets->sfx.lose);
    }

    if (you_win) {
        ++win_timer;
    }

    if (win_timer > 50) {
        SetMode(GM_WIN);
    }

    obj_copy(obj_mem, obj_buffer, 16); // 16 number tiles
    obj_aff_copy(obj_aff_mem, obj_aff_buffer, 16);


}
