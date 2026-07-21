#define NOB_IMPLEMENTATION
#include <nob.h>

/*-------------------------------------------------
                                   project setup */
#define GAME_TITLE   "2048"
#define GAME_CODE    "2048"
#define MAKER_CODE   "bt"
#define GAME_VERSION  2

#define BUILD       "build"
#define AUDIO       "audio"
#define SOURCE      "source"
#define INCLUDE     "include"
#define GRAPHICS    "graphics"
#define CARTNAME    BUILD"/"GAME_TITLE

#define EMULATOR "/Applications/mGBA.app/Contents/MacOS/mGBA"

/*-------------------------------------------------
                                 devkitpro setup */
#define   DKP_ROOT "/opt/devkitpro"
#define  DEVKITARM DKP_ROOT"/devkitARM"
#define ARM_PREFIX "arm-none-eabi"

#define      CC  DEVKITARM"/bin/" ARM_PREFIX "-gcc"
#define      LD  DEVKITARM"/bin/" ARM_PREFIX "-gcc"
#define OBJCOPY  DEVKITARM"/bin/" ARM_PREFIX "-objcopy"

#define LIBTONC  DKP_ROOT"/libtonc"
#define  LIBGBA  DKP_ROOT"/libgba"

#define    GRIT  DKP_ROOT"/tools/bin/grit"
#define   BIN2S  DKP_ROOT"/tools/bin/bin2s"
#define  MMUTIL  DKP_ROOT"/tools/bin/mmutil"
#define  GBAFIX  DKP_ROOT"/tools/bin/gbafix"

#define GBA_MB_SPECS  DEVKITARM"/"ARM_PREFIX"/lib/gba_mb.specs"
#define    GBA_SPECS  DEVKITARM"/"ARM_PREFIX"/lib/gba.specs"

#define GBA_FLAGS \
       "-march=armv4t", \
      "-mcpu=arm7tdmi", \
     "-mtune=arm7tdmi", \
               "-marm", \
             "-mthumb", \
        "-mlong-calls", \
   "-mthumb-interwork", \
 "-ffunction-sections", \
     "-fdata-sections", \
           "-D__GBA__", \
              "-DARM7", \

static struct {
    bool run_in_emulator;
    bool multiboot;
    bool debug;
} flags = { 0 };

static bool process_graphics( Walk_Entry entry );
static bool append_audio_files( Walk_Entry entry );
static bool compile_o_from_source_files( Walk_Entry entry );
static bool append_o_files( Walk_Entry entry );
static bool delete_o_files( Walk_Entry entry );

int main( int argc, char **argv )
{
    GO_REBUILD_URSELF( argc, argv );

    mkdir_if_not_exists( BUILD );

    Cmd cmd = { 0 };

    /*-------------------------------------------------------
                                               parse flags */
    for (int i = 1; i < argc; ++i)
    {
        if (*argv[i] == '-')
            for (char *flag = argv[i] + 1; *flag; ++flag)
                switch (*flag) {
                    case 'r': flags.run_in_emulator = true; break;
                    case 'm': flags.multiboot = true; break;
                    case 'd': flags.debug = true; break;
                }
        else if (!strcmp( argv[i], "run" ))       flags.run_in_emulator = true;
        else if (!strcmp( argv[i], "multiboot" )) flags.multiboot = true;
        else if (!strcmp( argv[i], "debug" ))     flags.debug = true;
    }

    /*-------------------------------------------------------
                                                  graphics */
    mkdir_if_not_exists( BUILD"/graphics" );

    walk_dir( GRAPHICS, process_graphics, &cmd );

    /*-------------------------------------------------------
                                                     audio */
    cmd_append( &cmd, MMUTIL );
    walk_dir( AUDIO, append_audio_files, &cmd );

    if (cmd.count > 1) {
        cmd_append( &cmd, "-o"BUILD"/soundbank.bin", "-h"BUILD"/soundbank.h" );
        if (!cmd_run( &cmd )) exit( EXIT_FAILURE );

        cmd_append( &cmd, BIN2S, "-a", "4", "-H", BUILD"/soundbank_bin.h", BUILD"/soundbank.bin" );
        if (!cmd_run( &cmd, .stdout_path = BUILD"/soundbank.s" )) exit( EXIT_FAILURE );
    }

    /*-------------------------------------------------------
                                               source code */

    walk_dir( SOURCE, compile_o_from_source_files, &cmd );

    walk_dir( BUILD,  compile_o_from_source_files, &cmd );

    cmd_append( &cmd,
        CC,
        "-std=c99",
        "-g", "-O2",
        "-Wall", "-Wextra", "-Werror",

        GBA_FLAGS

		flags.multiboot ? "-specs="GBA_MB_SPECS : "-specs="GBA_SPECS,

        "-I"BUILD,
        "-I"INCLUDE,
        "-I"LIBTONC"/include",
        "-I"LIBGBA"/include",

        "-o"CARTNAME".elf",
    );

    walk_dir( BUILD, append_o_files, &cmd );

    cmd_append( &cmd,
        "-L"DEVKITARM"/lib",
        "-L"LIBTONC"/lib",
        "-L"LIBGBA"/lib",
        "-lmm",
        "-ltonc",
    );

    if (!cmd_run( &cmd )) exit( EXIT_FAILURE );

    walk_dir( BUILD, delete_o_files );

    /*-------------------------------------------------------
                                                 make .gba */
    cmd_append( &cmd,
        OBJCOPY,
        "-O", "binary",
        CARTNAME".elf",
        CARTNAME".gba",
    );
    if (!cmd_run( &cmd )) exit( EXIT_FAILURE );


    /*-------------------------------------------------------
                               fix rom  ie. write metadata */
    #define stringify(x) #x
    #define string(x) stringify(x)

    cmd_append( &cmd,
        GBAFIX,
        CARTNAME".gba",
        "-p",
        "-t" GAME_TITLE,
        "-c" GAME_CODE,
        "-m" MAKER_CODE,
        "-r" string(GAME_VERSION),
    );

    #undef string
    #undef stringify

    if (flags.debug) cmd_append( &cmd, "-d1" );

    if (!cmd_run( &cmd )) exit( EXIT_FAILURE );


    /*-------------------------------------------------------
                                           run in emulator */
    if (flags.run_in_emulator) {
        cmd_append( &cmd, EMULATOR, CARTNAME".gba" );
        if (!cmd_run( &cmd )) exit( EXIT_FAILURE );
    }

    exit( EXIT_SUCCESS );
}


static
bool process_graphics( Walk_Entry entry )
{
    Cmd *cmd = entry.data;

    if (sv_ends_with_cstr( sv_from_cstr(entry.path), ".png" ))
    {
        String_Builder sb = { 0 };
        sb_appendf( &sb, BUILD"/%s", entry.path );
        String_View output = sb_to_sv( sb );

        cmd_append( cmd, GRIT, entry.path, "-fts", "-o", temp_sv_to_cstr(output) );

        if (!cmd_run( cmd )) exit( EXIT_FAILURE );
    }

    return true;
}

static
bool append_audio_files( Walk_Entry entry )
{
    Cmd *cmd = entry.data;

    if (sv_ends_with_cstr( sv_from_cstr(entry.path), ".wav" ))
        cmd_append( cmd, "-o", strdup(entry.path) );

    return true;
}

static
bool compile_o_from_source_files( Walk_Entry entry )
{
    Cmd *cmd = entry.data;

    String_View path = sv_from_cstr( entry.path );

    if (sv_ends_with_cstr( path, ".c" ) || sv_ends_with_cstr( path, ".s" ))
    {
        cmd_append( cmd, CC, "-c", entry.path );

        cmd_append( cmd,
            "-g", "-O2",
            "-Wall", "-Wextra", "-Werror",

            "-I"BUILD,
            "-I"INCLUDE,
            "-I"LIBTONC"/include",
            "-I"LIBGBA"/include",
            GBA_FLAGS
        );

        if (flags.debug)
            cmd_append( cmd, "-D_DEBUG" );

        char *filename = strrchr( entry.path, '/' ) + 1;

        String_View path_without_ext = sv_chop_by_delim( &path, '.' );

        /*--------------------------------------------------------
                           the graphics directory is explicitly  |
                           detected for the sake of includes eg. |
                           #include <graphics/titlebg0.h>       */
        bool is_in_graphics_dir = sv_chop_prefix( &path_without_ext, SVLIT(BUILD"/graphics") );

        const char *prefix = is_in_graphics_dir ? "graphics" : "";

        String_View file = sv_from_cstr( filename );
        String_View file_without_ext = sv_chop_by_delim( &file, '.' );

        String_Builder sb = { 0 };
        sb_appendf( &sb, BUILD"/%s"SV_Fmt".o", prefix, SV_Arg(file_without_ext) );

        String_View output = sb_to_sv( sb );
        cmd_append( cmd, "-o", temp_sv_to_cstr(output) );

        if (!cmd_run( cmd )) exit( EXIT_FAILURE );
    }
    return true;
}

static
bool append_o_files( Walk_Entry entry )
{
    Cmd *cmd = entry.data;

    if (sv_ends_with_cstr( sv_from_cstr(entry.path), ".o" ))
        cmd_append( cmd, strdup(entry.path) );

    return true;
}

static
bool delete_o_files( Walk_Entry entry )
{
    if (sv_ends_with_cstr( sv_from_cstr(entry.path), ".o" ))
        delete_file( entry.path );

    return true;
}
