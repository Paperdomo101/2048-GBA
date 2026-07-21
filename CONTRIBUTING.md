
# Contributing

## Style guide

- Tab size is 4 spaces (`    `)
- Use `snake_case` for variable / function names
- Use `Title_Snake_Case` for struct / enum type names
- All includes are in angle brackets
- `functions( with, args )` are padded with spaces
- `void function_declaration( void )` has explicit void parameter
- `fn( nested_fn() )` this is fine
- `fn( nested_fn( arg ) )` this is not: pass the result of the nested function as a variable 
- Function body brace `{` should be on a new line
- Function type qualifiers should be on a separate line above [example](./source/scenes/game.c:255:1)
- Lines exceeding 100 columns should be split to separate lines eg. [1](./source/scenes/save.c:36:9), [2](./source/scenes/game.c:125:5) [3](./source/scenes/game.c:246:5)
- Code should be largely self-documenting, don't plaster the codebase with superfluous comments (see longform comments in the example)

#### Example
```c
#include <any_header.h> '<- angle brackets NOT "double quotes"'

typedef {
    int some_member;
} Some_Struct;

typedef {
    SOME_ENUM_FIRST,
    SOME_ENUM__MAX__,
} Some_Enum;

static inline         '<- type qualifiers above'
void some_fn( void )  '<- explicit void parameter'
{                     '<- { on newline'
    fn( nested_fn() );      '<- this is ok'
 // fn( nested_fn( arg ) ); '<- this is NOT - store the result and pass it'
    result = nested_fn( arg ); '<- like this'
    fn( result );
    
    multi_line_fn(     
        nested_fn( multi_line ), '<- allowed in multi-line function calls'
        another_param,
        "something"
    );

 // clr_blend_fast( &pal_obj_mem[32], &pal_obj_mem[0], &pal_obj_mem[17], 6, anim_spawn_alpha[scale_frame[i]] >> 3 ); '<- this line is too long'
    clr_blend_fast(  '<- write it this way instead'
        &pal_obj_mem[32],
        &pal_obj_mem[0],
        &pal_obj_mem[17],
        6,
        anim_spawn_alpha[scale_frame[i]] >> 3
    );
    
    /*-----------------------------------------------------|
                            this is a longform doc comment |
                            use them when the code below   |
                            warrants an explanation       */
    the_code_in_question();
}
``` 
#### 
```c
```

## Adding Scenes

Add a new entry to the `SCENE_LIST` in [global.h](./include/global.h:15:1).

The format is `x(NAME, name)`

> `NAME` is the name used to refer to the scene as an identifier, eg. `set_scene( SCENE_NAME );`

> `function_name` is the suffix of the functions to be implemented for the scene eg. `update_scene_name()`

Create a new `.c` file in [source/scenes](./source/scenes/) and implement:
```
#include <global.h>

void init_scene_name( void )
{
    ...
}

void update_scene_name( void )
{
    ...
}
```

These functions will automatically be associated with `SCENE_NAME` via the x macro in update_scene() implemented in [main.c](./source/main.c) and you can go to your new scene by calling `set_scene( SCENE_NAME )` from an existing scene

## Adding sounds

Place audio file in the [audio](./audio/) folder.

Add a new entry to the `SFX_LIST` in [global.h](./include/global.h:24:1).

The format is `x(FILENAME, name)`

> `FILENAME` is the name of the audio file in SCREAMING_SNAKE_CASE without the extension, and is generated in [soundbank.h](./build/soundbank.h) as `SFX_FILENAME` (the `SFX_` is inferred by the x macro) 

> `name` is the struct member name you will reference when playing the sound eg. `mmEffectEx( &sfx.name );`
