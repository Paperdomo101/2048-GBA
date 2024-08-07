#include "global.h"
#include "soundbank.h"
#include "soundbank_bin.h"

static Assets assets = {0};

Assets *GetAssets(void) {
    return  &assets;
}

void LoadAssets(void) {

    mmInitDefault((mm_addr)soundbank_bin, 8);


    assets.sfx.start = (mm_sound_effect) { .id = SFX_START, .rate = (int)(1.0f * (1 << 10)), .volume = 255, .panning = 127};
    assets.sfx.spawn = (mm_sound_effect) { .id = SFX_SPAWN, .rate = (int)(1.0f * (1 << 10)), .volume = 255, .panning = 127};
    assets.sfx.slide = (mm_sound_effect) { .id = SFX_SLIDE, .rate = (int)(1.0f * (1 << 10)), .volume = 255, .panning = 127};
    assets.sfx.small_merge = (mm_sound_effect) { .id = SFX_SMALL_MERGE, .rate = (int)(1.0f * (1 << 10)), .volume = 255, .panning = 127};
    assets.sfx.merge = (mm_sound_effect) { .id = SFX_MERGE, .rate = (int)(1.0f * (1 << 10)), .volume = 255, .panning = 127};
    assets.sfx.big_merge = (mm_sound_effect) { .id = SFX_BIG_MERGE, .rate = (int)(1.0f * (1 << 10)), .volume = 255, .panning = 127};
    assets.sfx.lose  = (mm_sound_effect) { .id = SFX_LOSE,  .rate = (int)(1.0f * (1 << 10)), .volume = 255, .panning = 127};
    assets.sfx.win  = (mm_sound_effect) { .id = SFX_WIN,  .rate = (int)(1.0f * (1 << 10)), .volume = 255, .panning = 127};
}
