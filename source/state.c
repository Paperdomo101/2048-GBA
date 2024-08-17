#include "global.h"

static State state = {0};

void SetMode(int mode) {
    state.mode = mode;

    switch (state.mode)
    {
    case GM_FIRST:
        InitFirst();
        break;
    case GM_TITLE:
        InitTitle();
        break;
    case GM_GAME:
        InitGame();
        break;
    case GM_SAVE:
        InitSave();
        break;
    case GM_GAMEOVER:
        InitOver();
        break;
    case GM_WIN:
        InitWin();
        break;
    default:
        break;
    }
}

void UpdateState(void) {
    switch (state.mode)
    {
    case GM_FIRST:
        UpdateFirst();
        break;
    case GM_TITLE:
        UpdateTitle();
        break;
    case GM_GAME:
        UpdateGame();
        break;
    case GM_SAVE:
        UpdateSave();
        break;
    case GM_GAMEOVER:
        UpdateOver();
        break;
    case GM_WIN:
        UpdateWin();
        break;
    default:
        break;
    }
}

State *GetState() {
    return &state;
}
