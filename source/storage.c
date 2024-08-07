#include "global.h"

#define FLASH_ROM ((vu8 *)0x0e000000)
#define SCORE_ADDR (8)

static u8 read_byte(u16 addr) {
    return FLASH_ROM[addr];
}

static inline u16 read_2_bytes(u16 addr) {
    return read_byte(addr) |
           read_byte(addr + 1) << 8;
}

static inline u32 read_4_bytes(u16 addr) {
    return read_byte(addr) |
           read_byte(addr + 1) << 8 |
           read_byte(addr + 2) << 16 |
           read_byte(addr + 3) << 24;
}

bool StorageCheck(void) {
    // check if game code "2048" is present
    bool valid = read_byte(0) == '2' &&
                 read_byte(1) == '0' &&
                 read_byte(2) == '4' &&
                 read_byte(3) == '8';

    return valid;
}

void LoadState(State *state) {
    if (!StorageCheck())
        return;

    state->hiscore = read_4_bytes(SCORE_ADDR);
}


static void write_byte(u16 addr, u8 byte) {
    // checksum += byte;
    FLASH_ROM[addr] = byte;

    while(read_byte(addr) != byte);
}

static inline void write_2_bytes(u16 addr, u16 bytes) {
    write_byte(addr, bytes);
    write_byte(addr + 1, bytes >> 8);
}

static inline void write_4_bytes(u16 addr, u32 bytes) {
    write_byte(addr, bytes);
    write_byte(addr + 1, bytes >> 8);
    write_byte(addr + 2, bytes >> 16);
    write_byte(addr + 3, bytes >> 24);
}

void SaveState(State *state) {

    // game code - "2048"
    write_byte(0, '2');
    write_byte(1, '0');
    write_byte(2, '4');
    write_byte(3, '8');

    write_4_bytes(SCORE_ADDR, state->hiscore);
}
