#ifndef IO_H
#define IO_H

#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#define SCREEN_SCALE 10
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define BUFFER_SIZE SCREEN_WIDTH * SCREEN_HEIGHT
#define AUDIO_GAIN 2000

typedef struct {
    SDL_Window* window;
    SDL_Surface* surface;
    SDL_Palette* palette;
    SDL_Surface* blit_surface;
    SDL_Surface* draw_surface;
    uint8_t buffer[BUFFER_SIZE];
} Display;

typedef struct {
    SDL_AudioDeviceID audio_device;
    SDL_AudioSpec audio_spec;
} Audio;

typedef struct {
    bool input_active;
    uint8_t input;
} Keyboard;

typedef struct {
    Display* display;
    Audio* audio;
    Keyboard* keyboard;
} IO;

IO* IO_init();
void IO_free(IO* io);
void Display_render(Display* display);
void Audio_play(Audio* audio);
bool Keyboard_process_input(Keyboard* keyboard);

#endif
