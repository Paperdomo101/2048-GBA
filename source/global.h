#pragma once

#include <tonc.h>
#include <maxmod.h>

#include "soundbank.h"

typedef enum {
    GM_FIRST = 0,
    GM_TITLE,
    GM_GAME,
    GM_SAVE,
    GM_GAMEOVER,
    GM_WIN,
} Gamemodes;

#define FIRST_FADE_DURATION (20)
#define TITLE_FADE_DURATION (15)
#define GAME_FADE_DURATION (20)

#define ANIM_SLIDE_DURATION (5)
#define ANIM_SPAWN_DURATION (9)
#define ANIM_MERGE_DURATION (9)
#define ANIM_SCORE_DURATION (32)

#define KEY_RETURN (KEY_A | KEY_B | KEY_START | KEY_SELECT)

typedef struct {
    int x : 16;
    int y : 16;
} vec2i;

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
    int mode;
    void (*update)();
    u32 hiscore;
    u32 score;
    u8 saved;
    Square squares[16];
} State;

State *GetState();
void SetMode(int mode);

void LoadState(State *state);
void SaveState(State *state);

void InitFirst();
void InitTitle();
void InitOver();
void InitSave();
void InitWin();
void InitGame();

int GetBG1Off(int hi);
int GetFirstIs1(int hi);
int GetDigitCount(int hi);
