#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include "io.h"

IO* IO_init() {
    Display* display = calloc(1, sizeof(Display));
    Audio* audio = calloc(1, sizeof(Audio));
    Keyboard* keyboard = calloc(1, sizeof(Keyboard));
    IO* io = calloc(1, sizeof(IO));

    io->display = display;
    io->audio = audio;
    io->keyboard = keyboard;


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

    audio->audio_spec.freq = 44100;
    audio->audio_spec.format = AUDIO_S16SYS;
    audio->audio_spec.channels = 1;  // mono
    audio->audio_spec.samples = 4096;  // buffer size
    audio->audio_spec.callback = NULL;

    audio->audio_device = SDL_OpenAudioDevice(NULL, 0, &audio->audio_spec, NULL, 0);
    if (audio->audio_device == 0) {
        printf("Couldn't open audio device: %s\n", SDL_GetError());
        exit(1);
    }
    SDL_PauseAudioDevice(audio->audio_device, 0);

    return io;
}

void IO_free(IO* io) {
    SDL_FreePalette(io->display->palette);
    SDL_FreeSurface(io->display->draw_surface);
    SDL_FreeSurface(io->display->blit_surface);
    SDL_DestroyWindow(io->display->window);
    free(io->display);

    SDL_CloseAudioDevice(io->audio->audio_device);
    free(io->audio);

    free(io->keyboard);

    SDL_Quit();
    free(io);
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

void Audio_play(Audio* audio) {
    int16_t sample;
    static double x = 0;
    for (int i = 0; i < audio->audio_spec.freq / 60; i++) {
        x += 0.04f;
        sample = (sin(x) > 0 ? 1 : -1) * AUDIO_GAIN;  // square wave
        SDL_QueueAudio(audio->audio_device, &sample, sizeof(int16_t));
    }
}

bool Keyboard_process_input(Keyboard* keyboard) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                return false;
                break;
            case SDL_KEYDOWN:
                switch(event.key.keysym.scancode) {
                    case SDL_SCANCODE_1: keyboard->input = 0x01; keyboard->input_active = true; break;
                    case SDL_SCANCODE_2: keyboard->input = 0x02; keyboard->input_active = true; break;
                    case SDL_SCANCODE_3: keyboard->input = 0x03; keyboard->input_active = true; break;
                    case SDL_SCANCODE_4: keyboard->input = 0x0C; keyboard->input_active = true; break;
                    case SDL_SCANCODE_Q: keyboard->input = 0x04; keyboard->input_active = true; break;
                    case SDL_SCANCODE_W: keyboard->input = 0x05; keyboard->input_active = true; break;
                    case SDL_SCANCODE_E: keyboard->input = 0x06; keyboard->input_active = true; break;
                    case SDL_SCANCODE_R: keyboard->input = 0x0D; keyboard->input_active = true; break;
                    case SDL_SCANCODE_A: keyboard->input = 0x07; keyboard->input_active = true; break;
                    case SDL_SCANCODE_S: keyboard->input = 0x08; keyboard->input_active = true; break;
                    case SDL_SCANCODE_D: keyboard->input = 0x09; keyboard->input_active = true; break;
                    case SDL_SCANCODE_F: keyboard->input = 0x0E; keyboard->input_active = true; break;
                    case SDL_SCANCODE_Z: keyboard->input = 0x0A; keyboard->input_active = true; break;
                    case SDL_SCANCODE_X: keyboard->input = 0x00; keyboard->input_active = true; break;
                    case SDL_SCANCODE_C: keyboard->input = 0x0B; keyboard->input_active = true; break;
                    case SDL_SCANCODE_V: keyboard->input = 0x0F; keyboard->input_active = true; break;
                    default: break;
                }
                break;
            case SDL_KEYUP:
                switch(event.key.keysym.scancode) {
                    case SDL_SCANCODE_1: if (keyboard->input == 0x01) { keyboard->input_active = false; } break;
                    case SDL_SCANCODE_2: if (keyboard->input == 0x02) { keyboard->input_active = false; } break;
                    case SDL_SCANCODE_3: if (keyboard->input == 0x03) { keyboard->input_active = false; } break;
                    case SDL_SCANCODE_4: if (keyboard->input == 0x0C) { keyboard->input_active = false; } break;
                    case SDL_SCANCODE_Q: if (keyboard->input == 0x04) { keyboard->input_active = false; } break;
                    case SDL_SCANCODE_W: if (keyboard->input == 0x05) { keyboard->input_active = false; } break;
                    case SDL_SCANCODE_E: if (keyboard->input == 0x06) { keyboard->input_active = false; } break;
                    case SDL_SCANCODE_R: if (keyboard->input == 0x0D) { keyboard->input_active = false; } break;
                    case SDL_SCANCODE_A: if (keyboard->input == 0x07) { keyboard->input_active = false; } break;
                    case SDL_SCANCODE_S: if (keyboard->input == 0x08) { keyboard->input_active = false; } break;
                    case SDL_SCANCODE_D: if (keyboard->input == 0x09) { keyboard->input_active = false; } break;
                    case SDL_SCANCODE_F: if (keyboard->input == 0x0E) { keyboard->input_active = false; } break;
                    case SDL_SCANCODE_Z: if (keyboard->input == 0x0A) { keyboard->input_active = false; } break;
                    case SDL_SCANCODE_X: if (keyboard->input == 0x00) { keyboard->input_active = false; } break;
                    case SDL_SCANCODE_C: if (keyboard->input == 0x0B) { keyboard->input_active = false; } break;
                    case SDL_SCANCODE_V: if (keyboard->input == 0x0F) { keyboard->input_active = false; } break;
                    default: break;
                }
                break;
            default:
                break;
        }
    }
    return true;
}
