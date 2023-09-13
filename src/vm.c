#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <SDL2/SDL.h>
#include "display.h"
#include "vm.h"

const uint8_t fonts[] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

/*
 * Test if a system is little endian. If it's little endian then convert the value to big endian.
 */
uint16_t as_big_endian(uint16_t n) {
    uint16_t test = 1;
    if (*(uint8_t*)&test == 1)
        return n << 8 | n >> 8;
    return n;
}

void debug_inst(int i, char* opcode, Inst inst) {
# ifdef DEBUG
    printf("%04X: %s (%04X)\n", i, opcode, inst.opcode);
# endif
}

// 0x00E0
void clear_screen(Inst* inst, VM* vm) {
    for (int i = 0; i < BUFFER_SIZE; i++) {
        vm->display->buffer[i] = 0;
    }
}

// 0x00EE
void subroutine_return(Inst* inst, VM* vm) {
    uint16_t return_addr = ((uint16_t*)vm->memory)[MEMORY_SIZE - vm->sp];
    vm->sp -= 2;
    vm->pc = return_addr;
}

// 0x1NNN
void jump(Inst* inst, VM* vm) {
    uint16_t addr = inst->opcode &0x0FFF;
    vm->pc = addr;
}

// 0x2NNN
void subroutine_call(Inst* inst, VM* vm) {
    uint16_t return_addr = vm->pc;
    uint16_t jump_addr = inst->opcode &0x0FFF;
    vm->pc = jump_addr;
    // Stack grows down from the top of memory. It stores 16bit addresses
    vm->sp += 2;
    ((uint16_t*)vm->memory)[MEMORY_SIZE - vm->sp] = return_addr;
}

// 0x3XNN
void skip_equal(Inst* inst, VM* vm) {
    uint8_t v_register = (inst->opcode >> 8) & 0x0F;
    uint8_t value = inst->opcode & 0x00FF;

    if (vm->v[v_register] == value)
        vm->pc += 2;
}

// 0x4XNN
void skip_not_equal(Inst* inst, VM* vm) {
    uint8_t v_register = (inst->opcode >> 8) & 0x0F;
    uint8_t value = inst->opcode & 0x00FF;

    if (vm->v[v_register] != value)
        vm->pc += 2;
}

// 0x5XY0
void skip_registers_equal(Inst* inst, VM* vm) {
    uint8_t v_register_y = (inst->opcode >> 4) & 0x0F;
    uint8_t v_register_x = (inst->opcode >> 8) & 0x0F;

    if (vm->v[v_register_y] == vm->v[v_register_x])
        vm->pc += 2;
}

// 0x6XNN
void load_v(Inst* inst, VM* vm) {
    uint8_t v_register = (inst->opcode >> 8) & 0x0F;
    uint8_t value = inst->opcode & 0x00FF;
    vm->v[v_register] = value;
}

// 0x7XNN
void add(Inst* inst, VM* vm) {
    uint8_t v_register = (inst->opcode >> 8) & 0x0F;
    uint8_t value = inst->opcode & 0x00FF;
    vm->v[v_register] += value;
}

// 0x8XYN
void math(Inst* inst, VM* vm) {
    uint8_t operation = inst->opcode & 0x0F;
    uint8_t v_register_y = (inst->opcode >> 4) & 0x0F;
    uint8_t v_register_x = (inst->opcode >> 8) & 0x0F;

    switch (operation) {
        case 0x00:
            vm->v[v_register_x] = vm->v[v_register_y];
            break;
        case 0x01:
            vm->v[v_register_x] |= vm->v[v_register_y];
            break;
        case 0x02:
            vm->v[v_register_x] &= vm->v[v_register_y];
            break;
        case 0x03:
            vm->v[v_register_x] ^= vm->v[v_register_y];
            break;
        case 0x04:
            if (vm->v[v_register_x] + vm->v[v_register_y] > 0xFF) {
                vm->v[0x0F] = 1;
            }
            vm->v[v_register_x] += vm->v[v_register_y];
            break;
        case 0x05:
            vm->v[0xF] = vm->v[v_register_x] > vm->v[v_register_y] ? 1 : 0;
            vm->v[v_register_x] -= vm->v[v_register_y];
            break;
        case 0x06:
            vm->v[0x0F] = vm->v[v_register_x] & 0b00000001;
            vm->v[v_register_x] = vm->v[v_register_x] >> 1;
            break;
        case 0x07:
            vm->v[0x0F] = vm->v[v_register_y] > vm->v[v_register_x] ? 1 : 0;
            vm->v[v_register_x] = vm->v[v_register_y] - vm->v[v_register_x];
            break;
        case 0x0E:
            vm->v[0x0F] = vm->v[v_register_x] >> 7;
            vm->v[v_register_x] = vm->v[v_register_x] << 1;
            break;
        default:
            exit(1);
    }
}

// 0x9XY0
void skip_registers_not_equal(Inst* inst, VM* vm) {
    uint8_t v_register_y = (inst->opcode >> 4) & 0x0F;
    uint8_t v_register_x = (inst->opcode >> 8) & 0x0F;

    if (vm->v[v_register_y] != vm->v[v_register_x])
        vm->pc += 2;
}

// 0xANNN
void load_i(Inst* inst, VM* vm) {
    uint16_t value = inst->opcode & 0x0FFF;
    vm->i = value;
}

// 0xBNNN
void jump_offset(Inst* inst, VM* vm) {
    uint16_t addr = inst->opcode &0x0FFF;
    vm->pc = addr + vm->v[0];
}

// 0xCXNN
void rnd(Inst* inst, VM* vm) {
    uint8_t v_register_x = (inst->opcode >> 8) & 0x0F;
    uint8_t value = inst->opcode & 0x00FF;
    vm->v[v_register_x] = rand() & value;
}

// 0xDXYN
void draw(Inst* inst, VM* vm) {
    uint8_t x = (inst->opcode >> 8) & 0x0F;
    uint8_t y = (inst->opcode >> 4) & 0x0F;
    uint8_t x_coord = vm->v[x] % 64;
    uint8_t y_coord = vm->v[y] % 32;
    uint8_t height = inst->opcode & 0x0F;
    uint8_t sprite_row;
    uint8_t new_pixel;
    uint8_t current_pixel;
    uint32_t offset;

    vm->v[0x0F] = 0;
    for (int i = 0; i < height; i++) {
        sprite_row = vm->memory[vm->i + i];

        if (y_coord + i >= SCREEN_HEIGHT)
            break;

        for (int j = 0; j < 8; j++) {
            if (x_coord + j >= SCREEN_WIDTH)
                break;

            offset = ((y_coord + i) * SCREEN_WIDTH) + x_coord + j;
            new_pixel = (sprite_row >> (7-j)) & 0b00000001;
            current_pixel = vm->display->buffer[offset]; 

            if (new_pixel && current_pixel)
                vm->v[0x0F] = 1;

            vm->display->buffer[offset] ^= new_pixel;
        }
    }
}

// 0xEX9E and 0xEXA1
void skip_key(Inst* inst, VM* vm) {
    uint8_t v_register = (inst->opcode >> 8) & 0x0F;
    uint8_t operation = inst->opcode & 0x00FF;

    switch(operation) {
        case 0x9E:
            if (vm->input_active && vm->v[v_register] == vm->input)
                vm->pc += 2;
            break;
        case 0xA1:
            if (vm->input_active && vm->v[v_register] != vm->input)
                vm->pc += 2;
            break;
        default:
            exit(1);
    }
}

// 0xFXNN
void f_family(Inst* inst, VM* vm) {
    uint8_t v_register_x = (inst->opcode >> 8) & 0x0F;
    uint8_t operation = inst->opcode & 0x00FF;
    uint8_t value;
    uint8_t remainder;

    switch(operation) {
        case 0x07:
            vm->v[v_register_x] = vm->delay;
            break;
        case 0x0A:
            if (!vm->input_active)
                vm->pc -= 2;
            else
                vm->v[v_register_x] = vm->input;
            break;
        case 0x15:
            vm->delay = vm->v[v_register_x];
            break;
        case 0x18:
            vm->sound = vm->v[v_register_x];
            break;
        case 0x1E:
            vm->i += vm->v[v_register_x];
            if (vm->i & 0xF000)
                vm->v[0x0F] = 1;
            break;
        case 0x29:
            // offset to the font character. each font sprite is 5 rows tall
            vm->i = (vm->v[v_register_x] & 0x0F) * 5;
            break;
        case 0x33:
            value = vm->v[v_register_x];
            for (int i = 2; i >= 0; i--) {
                remainder = value % 10;
                value /= 10;
                vm->memory[vm->i + i] = remainder;
            }
            break;
        case 0x55:
            for (int i=0; i <= v_register_x; i++) {
                vm->memory[vm->i + i] = vm->v[i];
            }
            break;
        case 0x65:
            for (int i=0; i <= v_register_x; i++) {
                vm->v[i] = vm->memory[vm->i + i];
            }
            break;
        default:
            exit(1);
    }
}

VM* VM_init() {
    VM* vm = calloc(1, sizeof(VM));
    memcpy(vm->memory, fonts, 5 * 16);
    vm->display = Display_init();
    return vm;
}

void VM_free(VM* vm) {
    Display_free(vm->display);
    free(vm);
}

uint16_t VM_load_rom(VM* vm, char* fpath) {
    FILE* fd = fopen(fpath, "r");
    uint16_t* data = calloc(MAX_PROGRAM_SIZE, 1); 
    uint16_t size = fread(data, 1, MAX_PROGRAM_SIZE, fd);

    memcpy(vm->memory + PROGRAM_START_ADDR, data, size);
    vm->pc = PROGRAM_START_ADDR;
    fclose(fd);
    return size;
}

void VM_tick(VM* vm) {
    Inst inst;
    inst.opcode = as_big_endian(*(uint16_t*)(vm->memory + vm->pc));
    inst.family = inst.opcode >> 12;
    int i = vm->pc;
    vm->pc += 2;

    if (inst.opcode == 0x00E0) {
        debug_inst(i, "CLS", inst);
        clear_screen(&inst, vm);
    } else if (inst.opcode == 0x00EE) {
        debug_inst(i, "RET", inst);
        subroutine_return(&inst, vm);
    }
    else {
        switch (inst.family) {
            // only for host systems like COSMAC VIP
            case 0x00:
                debug_inst(i, "SYS", inst);
                exit(1);
                break;
            case 0x01:
                debug_inst(i, "JP", inst);
                jump(&inst, vm);
                break;
            case 0x02:
                debug_inst(i, "CALL", inst);
                subroutine_call(&inst, vm);
                break;
            case 0x03:
                debug_inst(i, "SE", inst);
                skip_equal(&inst, vm);
                break;
            case 0x04:
                debug_inst(i, "SNE", inst);
                skip_not_equal(&inst, vm);
                break;
            case 0x05:
                debug_inst(i, "SVE", inst);
                skip_registers_equal(&inst, vm);
                break;
            case 0x06:
                debug_inst(i, "LD_V", inst);
                load_v(&inst, vm);
                break;
            case 0x07:
                debug_inst(i, "ADD", inst);
                add(&inst, vm);
                break;
            case 0x08:
                debug_inst(i, "MATH", inst);
                math(&inst, vm);
                break;
            case 0x09:
                debug_inst(i, "SVNE", inst);
                skip_registers_not_equal(&inst, vm);
                break;
            case 0x0A:
                debug_inst(i, "LD_I", inst);
                load_i(&inst, vm);
                break;
            case 0x0B:
                debug_inst(i, "JP", inst);
                jump_offset(&inst, vm);
                break;
            case 0x0C:
                debug_inst(i, "RND", inst);
                rnd(&inst, vm);
                break;
            case 0x0D:
                debug_inst(i, "DRW", inst);
                draw(&inst, vm);
                break;
            case 0x0E:
                debug_inst(i, "SKP", inst);
                skip_key(&inst, vm);
                break;
            case 0x0F:
                debug_inst(i, "Ffamily", inst);
                f_family(&inst, vm);
                break;
            default:
                debug_inst(i, "???", inst);
                exit(1);
        }
    }
}

void VM_run(VM* vm) {
    uint32_t ticks = SDL_GetTicks();
    uint32_t next_draw = 0;
    uint32_t next_instruction = 0;
     

    for (;;) {
        ticks = SDL_GetTicks();
        if (SDL_TICKS_PASSED(ticks, next_instruction)) {
            VM_tick(vm);
            next_instruction = ticks + 2;  // 500 instructions per second
        }

        if (SDL_TICKS_PASSED(ticks, next_draw)) {
            if (vm->delay > 0)
                vm->delay -= 1;
            if (vm->sound > 0)
                // TODO play sound
                vm->sound -= 1;

            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                switch (event.type) {
                    case SDL_QUIT:
                        return;
                        break;
                    case SDL_KEYDOWN:
                        switch(event.key.keysym.scancode) {
                            case SDL_SCANCODE_1: vm->input = 0x01; vm->input_active = true; break;
                            case SDL_SCANCODE_2: vm->input = 0x02; vm->input_active = true; break;
                            case SDL_SCANCODE_3: vm->input = 0x03; vm->input_active = true; break;
                            case SDL_SCANCODE_4: vm->input = 0x0C; vm->input_active = true; break;
                            case SDL_SCANCODE_Q: vm->input = 0x04; vm->input_active = true; break;
                            case SDL_SCANCODE_W: vm->input = 0x05; vm->input_active = true; break;
                            case SDL_SCANCODE_E: vm->input = 0x06; vm->input_active = true; break;
                            case SDL_SCANCODE_R: vm->input = 0x0D; vm->input_active = true; break;
                            case SDL_SCANCODE_A: vm->input = 0x07; vm->input_active = true; break;
                            case SDL_SCANCODE_S: vm->input = 0x08; vm->input_active = true; break;
                            case SDL_SCANCODE_D: vm->input = 0x09; vm->input_active = true; break;
                            case SDL_SCANCODE_F: vm->input = 0x0E; vm->input_active = true; break;
                            case SDL_SCANCODE_Z: vm->input = 0x0A; vm->input_active = true; break;
                            case SDL_SCANCODE_X: vm->input = 0x00; vm->input_active = true; break;
                            case SDL_SCANCODE_C: vm->input = 0x0B; vm->input_active = true; break;
                            case SDL_SCANCODE_V: vm->input = 0x0F; vm->input_active = true; break;
                            default: break;
                        }
                        break;
                    case SDL_KEYUP:
                        switch(event.key.keysym.scancode) {
                            case SDL_SCANCODE_1: if (vm->input == 0x01) { vm->input_active = false; } break;
                            case SDL_SCANCODE_2: if (vm->input == 0x02) { vm->input_active = false; } break;
                            case SDL_SCANCODE_3: if (vm->input == 0x03) { vm->input_active = false; } break;
                            case SDL_SCANCODE_4: if (vm->input == 0x0C) { vm->input_active = false; } break;
                            case SDL_SCANCODE_Q: if (vm->input == 0x04) { vm->input_active = false; } break;
                            case SDL_SCANCODE_W: if (vm->input == 0x05) { vm->input_active = false; } break;
                            case SDL_SCANCODE_E: if (vm->input == 0x06) { vm->input_active = false; } break;
                            case SDL_SCANCODE_R: if (vm->input == 0x0D) { vm->input_active = false; } break;
                            case SDL_SCANCODE_A: if (vm->input == 0x07) { vm->input_active = false; } break;
                            case SDL_SCANCODE_S: if (vm->input == 0x08) { vm->input_active = false; } break;
                            case SDL_SCANCODE_D: if (vm->input == 0x09) { vm->input_active = false; } break;
                            case SDL_SCANCODE_F: if (vm->input == 0x0E) { vm->input_active = false; } break;
                            case SDL_SCANCODE_Z: if (vm->input == 0x0A) { vm->input_active = false; } break;
                            case SDL_SCANCODE_X: if (vm->input == 0x00) { vm->input_active = false; } break;
                            case SDL_SCANCODE_C: if (vm->input == 0x0B) { vm->input_active = false; } break;
                            case SDL_SCANCODE_V: if (vm->input == 0x0F) { vm->input_active = false; } break;
                            default: break;
                        }
                        break;
                    default:
                        break;
                }
            }

            Display_render(vm->display);
            next_draw = ticks + 17;  // 60 fps;
        }
    }
}
