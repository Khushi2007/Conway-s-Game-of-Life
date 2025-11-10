// Parv and Omkumar

/**
 * @file audio_manager.h
 * @brief Declarations for audio management functions.
 * 
 * This header defines the interface for initializing, controlling, and shutting down
 * the audio system using SDL3. It includes functions for playing background music and sound effects.
 */

#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <SDL3/SDL.h>
#include <stdbool.h>

bool init_audio_system(void);
bool play_background_music(const char* filename);
void pause_background_music(void);
void resume_background_music(void);
void stop_background_music(void);
bool play_sfx(const char* filename);
void shutdown_audio_system(void);

#endif