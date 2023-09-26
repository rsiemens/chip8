#ifndef VM_H
#define VM_H

#include <stdint.h>
#include <stdbool.h>
#include "io.h"

#define MEMORY_SIZE 0xFFF
#define PROGRAM_START_ADDR 0x200
#define MAX_PROGRAM_SIZE MEMORY_SIZE - PROGRAM_START_ADDR

typedef struct {
    uint16_t opcode;
    uint8_t family;
} Inst;

typedef struct{
    uint8_t memory[MEMORY_SIZE];
    // v0-f are general purpose 8-bit registers. The vf register is somewhat special as
    // it is used as a flag by some instructions.
    uint8_t v[16];
    // 16-bit memory address register (only the lower 12 are actually used due
    // to the OPcode only having room for 12bits.
    uint16_t i;
    // Special timer registers. They count down at a rate of 1 every 60hz till they reach 0.
    uint8_t delay;
    uint8_t sound;
    uint16_t pc;   // program counter
    uint8_t sp;    // stack pointer
    IO* io;
} VM;

VM* VM_init();
void VM_free(VM* vm);
int16_t VM_load_rom(VM* vm, char* fpath);
void VM_tick(VM* vm);
void VM_run(VM* vm);

#endif
