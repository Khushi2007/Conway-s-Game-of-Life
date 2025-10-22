#include "audio_manager.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_thread.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static SDL_Thread *music_thread = NULL;
static bool music_thread_running = false;
static bool music_paused = false;

static SDL_AudioStream *music_stream = NULL;
static SDL_AudioStream *sfx_stream = NULL;

static Uint8 *music_buffer = NULL;
static Uint32 music_length = 0;
static const char *current_music_file = NULL;

static int music_loop_thread(void *arg) {
    while (music_thread_running) {
        if (!music_paused && music_stream && SDL_GetAudioStreamQueued(music_stream) == 0) {
            SDL_AudioSpec spec;
            Uint8 *buffer;
            Uint32 length;

            if (!SDL_LoadWAV(current_music_file, &spec, &buffer, &length)) {
                SDL_Log("Failed to reload WAV: %s\n", SDL_GetError());
            } else {
                SDL_PutAudioStreamData(music_stream, buffer, length);
                SDL_free(buffer);
            }
        }
        SDL_Delay(50);
    }
    return 0;
}

bool init_audio_system(void) {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        SDL_Log("SDL_Init AUDIO failed: %s\n", SDL_GetError());
        return false;
    }
    return true;
}

void shutdown_audio_system(void) {
    music_thread_running = false;
    if (music_thread) {
        SDL_WaitThread(music_thread, NULL);
        music_thread = NULL;
    }

    if (music_stream) {
        SDL_DestroyAudioStream(music_stream);
        music_stream = NULL;
    }

    if (sfx_stream) {
        SDL_DestroyAudioStream(sfx_stream);
        sfx_stream = NULL;
    }

    if (music_buffer) {
        SDL_free(music_buffer);
        music_buffer = NULL;
    }

    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

bool play_background_music(const char* filename) {
    if (music_stream) {
        SDL_DestroyAudioStream(music_stream);
        music_stream = NULL;
    }
    if (music_buffer) {
        SDL_free(music_buffer);
        music_buffer = NULL;
    }

    SDL_AudioSpec spec;
    Uint8 *buffer;
    Uint32 length;

    if (!SDL_LoadWAV(filename, &spec, &buffer, &length)) {
        SDL_Log("Failed to load WAV: %s\n", SDL_GetError());
        return false;
    }

    music_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (!music_stream) {
        SDL_Log("SDL_OpenAudioDeviceStream failed: %s\n", SDL_GetError());
        SDL_free(buffer);
        return false;
    }

    SDL_PutAudioStreamData(music_stream, buffer, length);
    SDL_ResumeAudioStreamDevice(music_stream);

    sfx_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (!sfx_stream) {
        SDL_Log("SDL_OpenAudioDeviceStream (SFX) failed: %s\n", SDL_GetError());
        SDL_DestroyAudioStream(music_stream);
        SDL_free(buffer);
        return false;
    }
    SDL_ResumeAudioStreamDevice(sfx_stream);

    music_buffer = buffer;
    music_length = length;
    current_music_file = filename;

    if (!music_thread_running) {
        music_thread_running = true;
        music_thread = SDL_CreateThread(music_loop_thread, "MusicThread", NULL);
    }

    music_paused = false;
    return true;
}

void stop_background_music(void) {
    music_thread_running = false;
    if (music_thread) {
        SDL_WaitThread(music_thread, NULL);
        music_thread = NULL;
    }

    if (music_stream) {
        SDL_DestroyAudioStream(music_stream);
        music_stream = NULL;
    }

    if (sfx_stream) {
        SDL_DestroyAudioStream(sfx_stream);
        sfx_stream = NULL;
    }

    if (music_buffer) {
        SDL_free(music_buffer);
        music_buffer = NULL;
    }

    music_length = 0;
    music_paused = false;
}

void pause_background_music(void) {
    music_paused = true;
    if (music_stream)
        SDL_PauseAudioStreamDevice(music_stream);
}

void resume_background_music(void) {
    music_paused = false;
    if (music_stream)
        SDL_ResumeAudioStreamDevice(music_stream);
}

bool play_sfx(const char* filename) {
    SDL_AudioSpec sfx_spec;
    Uint8 *sfx_buffer;
    Uint32 sfx_length;

    if (!SDL_LoadWAV(filename, &sfx_spec, &sfx_buffer, &sfx_length)) {
        SDL_Log("Failed to load SFX WAV: %s\n", SDL_GetError());
        return false;
    }

    if (sfx_stream) {
        SDL_PutAudioStreamData(sfx_stream, sfx_buffer, sfx_length);
        SDL_ResumeAudioStreamDevice(sfx_stream);
    }

    SDL_free(sfx_buffer);
    return true;
}
