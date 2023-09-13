#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include "display.h"

Display* Display_init() {
    Display* display = calloc(1, sizeof(Display));

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        printf("Couldn't initialize SDL: %s\n", SDL_GetError());
        exit(1);
    };

    display->window = SDL_CreateWindow(
        "CHIP-8 Emulator",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH * SCREEN_SCALE,
        SCREEN_HEIGHT * SCREEN_SCALE,
        0
    );
    if (!display->window) {
		printf("Failed to open window: %s\n", SDL_GetError());
		exit(1);
    }

    display->surface = SDL_GetWindowSurface(display->window);
    if (!display->surface) {
		printf("Failed to create surface: %s\n", SDL_GetError());
		exit(1);
    }

    display->palette = SDL_AllocPalette(2);
    // gameboy colors (dark green and light green)
    display->palette->colors[0].r = 48;
    display->palette->colors[0].g = 98;
    display->palette->colors[0].b = 48;
    display->palette->colors[1].r = 139;
    display->palette->colors[1].g = 172;
    display->palette->colors[1].b = 15;

    display->draw_surface = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 8, 0, 0, 0, 0);
    display->blit_surface = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0, 0, 0, 0);

    if (SDL_SetSurfacePalette(display->draw_surface, display->palette) != 0) {
        printf("%s\n", SDL_GetError());
        exit(1);
    }

    return display;
}

void Display_free(Display* display) {
    SDL_FreePalette(display->palette);
    SDL_FreeSurface(display->draw_surface);
    SDL_DestroyWindow(display->window);
    SDL_Quit();
    free(display);
}

void Display_render(Display* display) {
    SDL_LockSurface(display->draw_surface);
    for (int i = 0; i < BUFFER_SIZE; i++) {
        ((uint8_t*)display->draw_surface->pixels)[i] = display->buffer[i];
    }
    SDL_UnlockSurface(display->draw_surface);

    SDL_BlitSurface(display->draw_surface, NULL, display->blit_surface, NULL);
    SDL_BlitScaled(display->blit_surface, NULL, display->surface, NULL);
    SDL_UpdateWindowSurface(display->window);
}
