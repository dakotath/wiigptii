/****************************************************************************
 * libwiigui Template
 * Tantric 2009
 *
 * audio.cpp
 * Audio support
 ***************************************************************************/

#include <gccore.h>
#include <ogcsys.h>
#include <asndlib.h>
#include <stdbool.h>

/****************************************************************************
 * InitAudio
 *
 * Initializes the Wii's audio subsystem
 ***************************************************************************/
void InitAudio()
{
	AUDIO_Init(NULL);
	ASND_Init();
	ASND_Pause(0);
}

void playSfx(const void* buffer, long buffer_size, int channel)
{
	u32 format = VOICE_STEREO_8BIT;
    ASND_SetVoice(channel, format, 32000, 0, (void*)buffer, buffer_size, 255, 255, NULL);
	ASND_Pause(0);
}
bool audioPlaying(int channel)
{
	if(ASND_StatusVoice(channel) == SND_WORKING || ASND_StatusVoice(channel) == SND_WAITING)
		return true;
	else
		return false;
}

/****************************************************************************
 * ShutdownAudio
 *
 * Shuts down audio subsystem. Useful to avoid unpleasant sounds if a
 * crash occurs during shutdown.
 ***************************************************************************/
void ShutdownAudio()
{
	ASND_Pause(1);
	ASND_End();
}
