// Parv and Omkumar

/**
 * @file audio_manager.c
 * @brief Handles all audio functionalities including background music and sound effects.
 * 
 * This module uses SDL3 for audio management, playing WAV files for both music and sound effects simultaneously.
 * Music threading is implemented to ensure continuous playback without blocking the main application flow.
 */

#include "audio_manager.h" // for audio manager function declarations
#include <SDL3/SDL.h> // for SDL main functionalities
#include <SDL3/SDL_thread.h> // for SDL threading
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* --------------------------------------------------------------------------------------------
 * Global State
 * --------------------------------------------------------------------------------------------
 * These static variables maintain the persistence of audio streams and thread state
 * -------------------------------------------------------------------------------------------- */

static SDL_Thread *music_thread = NULL; // Thread for looping background music
static bool music_thread_running = false; // Flag to control the music thread loop
static bool music_paused = false; // Indicates if the music is currently paused

static SDL_AudioStream *music_stream = NULL; // Stream for background music playback
static SDL_AudioStream *sfx_stream = NULL; // Separate stream for sound effects playback

static Uint8 *music_buffer = NULL; // WAV Buffer to hold the loaded background music data
static Uint32 music_length = 0; // Length of the current music buffer
static const char *current_music_file = NULL; // Path to the currently loaded background music file

/* --------------------------------------------------------------------------------------------
 * Internal Thread Function
 * --------------------------------------------------------------------------------------------
 * Continuously checks and reloads the music stream to ensure seamless looping
 * -------------------------------------------------------------------------------------------- */

static int music_loop_thread(void *arg) {
    while (music_thread_running) {
        // If music is not paused and the stream is empty, reload the music
        if (!music_paused && music_stream && SDL_GetAudioStreamQueued(music_stream) == 0) {
            SDL_AudioSpec spec; // Spec to hold WAV format
            Uint8 *buffer; // Buffer for reloading WAV data
            Uint32 length; // Length of reloaded data

            // Reload the WAV data
            if (!SDL_LoadWAV(current_music_file, &spec, &buffer, &length)) {
                SDL_Log("Failed to reload WAV: %s\n", SDL_GetError());
            }
            // Refill the music stream 
            else {
                SDL_PutAudioStreamData(music_stream, buffer, length);
                SDL_free(buffer);
            }
        }
        SDL_Delay(50); // Sleep briefly to avoid busy-waiting
    }
    return 0;
}

/* --------------------------------------------------------------------------------------------
 * Audio System Initialization and Shutdown
 * -------------------------------------------------------------------------------------------- */

/**
 * @brief Initializes the SDL audio subsystem.
 * @return true if initialization is successful, false otherwise.
 */

bool init_audio_system(void) {
    // Initialize SDL audio subsystem
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        SDL_Log("SDL_Init AUDIO failed: %s\n", SDL_GetError());
        return false;
    }
    return true;
}

/**
 * @brief Shuts down the SDL audio subsystem and frees all resources.
 */

void shutdown_audio_system(void) {
    music_thread_running = false; // Stop the music thread
    if (music_thread) {
        SDL_WaitThread(music_thread, NULL); // Wait for thread to finish
        music_thread = NULL; // Clear thread pointer
    }
    // Free music stream
    if (music_stream) {
        SDL_DestroyAudioStream(music_stream);
        music_stream = NULL;
    }
    // Free SFX stream
    if (sfx_stream) {
        SDL_DestroyAudioStream(sfx_stream);
        sfx_stream = NULL;
    }
    // Free music buffer
    if (music_buffer) {
        SDL_free(music_buffer);
        music_buffer = NULL;
    }
    // Reset music state
    current_music_file = NULL;
    music_length = 0;
    music_paused = false;
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

/* --------------------------------------------------------------------------------------------
 * Background Music Control Functions
 * -------------------------------------------------------------------------------------------- */

/**
 * @brief Loads and starts playing background music from a WAV file.
 * @param filename Path to the WAV file.
 * @return true if the music starts playing successfully, false otherwise.
 */

bool play_background_music(const char* filename) {
    if (music_stream) {
        // Free existing music stream
        SDL_DestroyAudioStream(music_stream); 
        music_stream = NULL;
    }
    if (music_buffer) {
        // Free existing music buffer
        SDL_free(music_buffer);
        music_buffer = NULL;
    }

    SDL_AudioSpec spec; // Spec to hold WAV format
    Uint8 *buffer; // Buffer for WAV data
    Uint32 length; // Length of WAV data

    // Load the WAV file
    if (!SDL_LoadWAV(filename, &spec, &buffer, &length)) {
        SDL_Log("Failed to load WAV: %s\n", SDL_GetError());
        return false;
    }
    
    // Create background music stream
    music_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (!music_stream) {
        SDL_Log("SDL_OpenAudioDeviceStream failed: %s\n", SDL_GetError());
        SDL_free(buffer);
        return false;
    }

    SDL_PutAudioStreamData(music_stream, buffer, length); // Queue the music data
    SDL_ResumeAudioStreamDevice(music_stream); // Start playback

    // Create a second stream for SFX playback
    sfx_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (!sfx_stream) {
        SDL_Log("SDL_OpenAudioDeviceStream (SFX) failed: %s\n", SDL_GetError());
        SDL_DestroyAudioStream(music_stream);
        SDL_free(buffer);
        return false;
    }
    
    SDL_ResumeAudioStreamDevice(sfx_stream); // Start SFX playback

    // Store music data for looping
    music_buffer = buffer;
    music_length = length;
    current_music_file = filename;

    // Stop background music thread if already running
    if (!music_thread_running) {
        music_thread_running = true;
        music_thread = SDL_CreateThread(music_loop_thread, "MusicThread", NULL);
    }

    music_paused = false;
    return true;
}

/**
 * @brief Stops background music playback and frees associated resources.
 */

void stop_background_music(void) {
    music_thread_running = false; // Signal the music thread to stop
    if (music_thread) {
        SDL_WaitThread(music_thread, NULL); // Wait for thread to finish
        music_thread = NULL; // Clear thread pointer
    }
    // Free music stream
    if (music_stream) {
        SDL_DestroyAudioStream(music_stream);
        music_stream = NULL;
    }
    // Free SFX stream
    if (sfx_stream) {
        SDL_DestroyAudioStream(sfx_stream);
        sfx_stream = NULL;
    }
    // Free music buffer
    if (music_buffer) {
        SDL_free(music_buffer);
        music_buffer = NULL;
    }
    // Reset music state
    current_music_file = NULL;
    music_length = 0;
    music_paused = false;
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

/**
 * @brief Pauses background music playback.
 */

void pause_background_music(void) {
    music_paused = true;
    if (music_stream) SDL_PauseAudioStreamDevice(music_stream);
}

/**
 * @brief Resumes background music playback.
 */

void resume_background_music(void) {
    music_paused = false;
    if (music_stream) SDL_ResumeAudioStreamDevice(music_stream);
}

/* --------------------------------------------------------------------------------------------
 * Sound Effects Playback
 * -------------------------------------------------------------------------------------------- */

/**
 * @brief Plays a short sound effect from a WAV file without interrupting background music.
 * @param filename Path to the WAV file.
 * @return true if the sound effect plays successfully, false otherwise.
 */

bool play_sfx(const char* filename) {
    SDL_AudioSpec sfx_spec; // Spec to hold WAV format
    Uint8 *sfx_buffer; // Buffer for SFX data
    Uint32 sfx_length; // Length of SFX data

    // Load the SFX WAV file
    if (!SDL_LoadWAV(filename, &sfx_spec, &sfx_buffer, &sfx_length)) {
        SDL_Log("Failed to load SFX WAV: %s\n", SDL_GetError());
        return false;
    }
    // Queue the SFX data to the SFX stream
    if (sfx_stream) {
        SDL_PutAudioStreamData(sfx_stream, sfx_buffer, sfx_length);
        SDL_ResumeAudioStreamDevice(sfx_stream);
    }
    // Free the SFX buffer
    SDL_free(sfx_buffer);
    return true;
}