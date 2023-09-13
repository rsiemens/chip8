#ifndef DISPLAY_H
#define DISPLAY_H

#include <SDL2/SDL.h>

#define SCREEN_SCALE 10
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define BUFFER_SIZE SCREEN_WIDTH * SCREEN_HEIGHT

typedef struct {
    SDL_Window* window;
    SDL_Surface* surface;
    SDL_Palette* palette;
    SDL_Surface* blit_surface;
    SDL_Surface* draw_surface;
    uint8_t buffer[BUFFER_SIZE]; 
} Display;

Display* Display_init();
void Display_free(Display* display);
void Display_render(Display* display);

#endif
