#pragma once
#include <stdlib.h>
#include <string.h>

#include <tonc.h>
#include <tonc_memmap.h>
#include <tonc_types.h>
#include <tonc_video.h>
#include <tonc_oam.h>
#include <tonc_irq.h>

#include <maxmod.h>
#include <mm_types.h>

#define SCENE_LIST \
  x(FIRST, first)  \
  x(TITLE, title)  \
  x(GAME,  game)   \
  x(SAVE,  save)   \
  x(LOSE,  lose)   \
  x(WIN,   win)    \

#define SFX_LIST              \
  x(START,       start)       \
  x(SPAWN,       spawn)       \
  x(SLIDE,       slide)       \
  x(SMALL_MERGE, small_merge) \
  x(MERGE,       merge)       \
  x(BIG_MERGE,   big_merge)   \
  x(HUGE_MERGE,  huge_merge)  \
  x(SAVE,        save)        \
  x(LOSE,        lose)        \
  x(WIN,         win)         \

#define FIRST_FADE_DURATION (20)
#define TITLE_FADE_DURATION (15)
#define GAME_FADE_DURATION  (20)

#define ANIM_SLIDE_DURATION (5)
#define ANIM_SPAWN_DURATION (9)
#define ANIM_MERGE_DURATION (9)
#define ANIM_SCORE_DURATION (32)

#define OBJECT_IS_HIDDEN( o ) \
    (((o).attr0 & ATTR0_MODE_MASK) == ATTR0_HIDE)

typedef enum {
    SCENE_NULL = 0,
    #define x(ENUM, FUNC) SCENE_##ENUM,
    SCENE_LIST
    #undef x
    SCENE__MAX__
} Scenes;

#define x(ENUM, FUNC) void init_scene_##FUNC( void );
SCENE_LIST
#undef x

#define x(ENUM, FUNC) void update_scene_##FUNC( void );
SCENE_LIST
#undef x


typedef struct {
    int x : 16;
    int y : 16;
} Vector2_int;

typedef union {
    struct {
        u8 shift : 2;
        u8 merge : 1;
        u8 fresh : 1;
        u8 value : 4;
    };
    u8 data;
} Square;

typedef struct {
    int seed;

    Scenes scene;
    Scenes next_scene;
    Scenes last_scene;

    u32 score;
    u32 hiscore;

    u8 saved;
    Square squares[16];
} Global_State;

typedef struct {
#define x( generated, member ) mm_sound_effect member;
    SFX_LIST
#undef x
} Global_Sfx;


extern Global_State state;
extern Global_Sfx sfx;

/*------------------------------------
                             main.c */
void set_scene( Scenes scene );
void update_scene( void );

/*------------------------------------
                          storage.c */
void load_state( void );
void save_state( void );
void save_state_including_squares( void );

/*------------------------------------
                            score.c */
void init_score( void );
void reset_score( void );
void update_score( void );

bool score_starts_with_1( bool is_hiscore );
int get_digit_count( void );
int get_bg1_offset( void );

void hide_tally_particle( void );
void spawn_tally_particle( void );
void update_tally_particle( void );
void reset_tally( void );
void add_to_tally( u32 amount );
void add_tally_to_score( void );
