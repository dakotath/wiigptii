/****************************************************************************
 * libwiigui Template
 * Tantric 2009
 *
 * audio.h
 * Audio support
 ***************************************************************************/

#ifndef _AUDIO_H_
#define _AUDIO_H_

#include <stdbool.h>

void InitAudio();
void playSfx(const void* buffer, long buffer_size, int channel);
bool audioPlaying(int channel);
void ShutdownAudio();

#endif
