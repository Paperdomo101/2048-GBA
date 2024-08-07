#pragma once
#include <tonc.h>
#include <tonc_memmap.h>
#include <tonc_types.h>
#include <tonc_video.h>
#include <tonc_oam.h>
#include <tonc_irq.h>
#include <maxmod.h>
#include "mm_types.h"

enum Gamemodes {
    GM_FIRST = 0,
    GM_TITLE,
    GM_GAME,
    GM_GAMEOVER,
    GM_WIN,
};

#define FIRST_FADE_DURATION (20)
#define TITLE_FADE_DURATION (15)
#define GAME_FADE_DURATION (20)

#define ANIM_SLIDE_DURATION (5)
#define ANIM_SPAWN_DURATION (9)
#define ANIM_MERGE_DURATION (9)
#define ANIM_SCORE_DURATION (32)


typedef struct State {
    int seed;
    int mode;
    u32 hiscore;
    u32 score;
} State;

typedef struct Assets {
    struct {
        mm_sound_effect start;
        mm_sound_effect spawn;
        mm_sound_effect slide;
        mm_sound_effect small_merge;
        mm_sound_effect merge;
        mm_sound_effect big_merge;
        mm_sound_effect lose;
        mm_sound_effect win;
    } sfx;
} Assets;

typedef struct vec2i {
    int x : 16;
    int y : 16;
} vec2i;

typedef union Square {
    struct {
        u8 shift : 2;
        u8 merge : 1;
        u8 fresh : 1;
        u8 value : 4;
    };
    u8 data;
} Square;

State *GetState(void);
void SetMode(int mode);

Assets *GetAssets(void);
void LoadAssets(void);

void InitFirst(void);
void UpdateFirst(void);

void InitTitle(void);
void UpdateTitle(void);

void InitOver(void);
void UpdateOver(void);

void InitWin(void);
void UpdateWin(void);

void InitGame(void);
void StartGame(void);
void UpdateGame(void);

void LoadState(State *state);
void SaveState(State *state);

int GetBG1Off(int hi);
int GetFirstIs1(int hi);
int GetDigitCount(int hi);
