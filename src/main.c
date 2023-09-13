#include <stdint.h>
#include <stdio.h>
#include "vm.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: chip8 <rom>\n");
        return 0;
    }

    VM* vm = VM_init();
    uint16_t rom_size = VM_load_rom(vm, argv[1]);
    printf("Loaded %s (%d bytes)\n", argv[1], rom_size);
    VM_run(vm);
    VM_free(vm);

    return 0;
}
