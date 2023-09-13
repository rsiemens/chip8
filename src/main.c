#include <stdint.h>
#include <stdio.h>
#include "vm.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: chip8 <rom>\n");
        return 0;
    }

    VM* vm = VM_init();
    int16_t rom_size = VM_load_rom(vm, argv[1]);

    if (rom_size == -1) {
        printf("No such file \"%s\"\n", argv[1]);
        return 0;
    }
    printf("Loaded %s (%d bytes)\n", argv[1], rom_size);

    VM_run(vm);
    VM_free(vm);

    return 0;
}
