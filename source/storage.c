#include "global.h"

#define FLASH_ROM ((vu8 *)0x0E000000)
#define GAME_VERSION (1)

#define SCORE_ADDR (4)
#define HISCORE_ADDR (8)
#define SAVED_ADDR (14)
#define SQUARES_ADDR (16)
#define VERSION_ADDR (64)

static u8 read_byte(u16 addr) {
    return FLASH_ROM[addr];
}

static inline u16 read_2_bytes(u16 addr) {
    return read_byte(addr) |
           read_byte(addr + 1) << 8;
}

static inline u32 read_4_bytes(u16 addr) {
    return read_2_bytes(addr) |
           read_2_bytes(addr + 2) << 16;
}

static void write_byte(u16 addr, u8 byte) {
    FLASH_ROM[addr] = byte;
}

static inline void write_2_bytes(u16 addr, u16 bytes) {
    write_byte(addr, bytes & 0xFF);
    write_byte(addr + 1, bytes >> 8);
}

static inline void write_4_bytes(u16 addr, u32 bytes) {
    write_2_bytes(addr, bytes & 0xFFFF);
    write_2_bytes(addr + 2, bytes >> 16);
}

int StorageCheck() {
    // check if game code "2048" is present
    bool valid = read_byte(0) == '2' &&
                 read_byte(1) == '0' &&
                 read_byte(2) == '4' &&
                 read_byte(3) == '8';

    int ver = (read_byte(VERSION_ADDR) == GAME_VERSION) * GAME_VERSION;

    return valid + ver;
}

void LoadState(State *state) {
    if (!StorageCheck())
        return;

    // clean
    if (StorageCheck() <= GAME_VERSION) {
        write_4_bytes(SCORE_ADDR, 0);
        write_4_bytes(SAVED_ADDR, 0);
        write_4_bytes(SQUARES_ADDR+4, 0);
        write_4_bytes(SQUARES_ADDR+8, 0);
        write_4_bytes(SQUARES_ADDR+12, 0);
    }

    state->score = read_4_bytes(SCORE_ADDR);
    state->hiscore = read_4_bytes(HISCORE_ADDR);
    state->saved = read_byte(SAVED_ADDR);

    for (u8 i = 0; i < 16; ++i) {
        state->squares[i].data = read_byte(SQUARES_ADDR+i);
    }
}

void SaveState(State *state) {
    // game code - "2048"
    write_byte(0, '2');
    write_byte(1, '0');
    write_byte(2, '4');
    write_byte(3, '8');

    write_byte(VERSION_ADDR, GAME_VERSION);

    write_4_bytes(SCORE_ADDR, state->score);
    write_4_bytes(HISCORE_ADDR, state->hiscore);

    write_byte(SAVED_ADDR, state->saved);

    for (u8 i = 0; i < 16; ++i) {
        write_byte(SQUARES_ADDR+i, state->squares[i].data);
    }
}
