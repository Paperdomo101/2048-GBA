#include <global.h>

#include <soundbank.h>
#include <soundbank_bin.h>

int main( void )
{
    load_state();

    mmInitDefault( (mm_addr)soundbank_bin, 8 );

    irq_init( NULL );

    irq_set( II_VBLANK, mmVBlank, 0 );
    irq_enable( II_VBLANK );

    set_scene( SCENE_FIRST );

    for (;;)
    {
        mmFrame();
        key_poll();

        update_scene();
        VBlankIntrWait();
    }

    exit( EXIT_SUCCESS );
}

inline
void set_scene( Scenes scene )
{
    /*----------------------------------
            actual scene change occurs |
            at the top of update_scene |
            next frame                */
    state.next_scene = scene;
}

inline
void update_scene( void )
{
    if (state.scene != state.next_scene) {

        state.last_scene = state.scene;
        state.scene = state.next_scene;

        switch (state.scene) {
            #define x(ENUM, FUNC) case SCENE_##ENUM: init_scene_##FUNC(); break;
            SCENE_LIST
            #undef x
            case SCENE_NULL: case SCENE__MAX__: break;
        }
    }

    switch (state.scene) {
        #define x(ENUM, FUNC) case SCENE_##ENUM: update_scene_##FUNC(); break;
        SCENE_LIST
        #undef x
        case SCENE_NULL: case SCENE__MAX__: break;
    }
}

Global_State state = { 0 };

Global_Sfx sfx = {
#define x( generated, member )             \
    .member = {                            \
        .id = SFX_##generated,             \
        .rate = (int)(1.0f * (1 << 10)),   \
        .volume = 255,                     \
        .panning = 127,                    \
    },
    SFX_LIST
#undef x
};
