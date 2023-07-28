#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogcsys.h>
#include <unistd.h>
#include <wiiuse/wpad.h>
#include <fat.h>
#include <asndlib.h>

#include "reciter.h"
#include "sam.h"
#include "debug.h"



void PlayWavFromBuffer(char* buffer, int bufferlength)
{
    // Initialize the Wii's audio subsystem
    // Set the audio format
    u32 format = VOICE_MONO_8BIT;
    ASND_SetVoice(3, format, 22050, 0, buffer, bufferlength, 255, 255, NULL);

    // Play the audio
    ASND_Pause(0); // Start audio output
}

void WriteWav(char* filename, char* buffer, int bufferlength)
{
    FILE *file = fopen(filename, "wb");
    if (file == NULL) return;
    //RIFF header
    fwrite("RIFF", 4, 1,file);
    unsigned int filesize=bufferlength + 12 + 16 + 8 - 8;
    fwrite(&filesize, 4, 1, file);
    fwrite("WAVE", 4, 1, file);

    //format chunk
    fwrite("fmt ", 4, 1, file);
    unsigned int fmtlength = 16;
    fwrite(&fmtlength, 4, 1, file);
    unsigned short int format=1; //PCM
    fwrite(&format, 2, 1, file);
    unsigned short int channels=1;
    fwrite(&channels, 2, 1, file);
    unsigned int samplerate = 22050;
    fwrite(&samplerate, 4, 1, file);
    fwrite(&samplerate, 4, 1, file); // bytes/second
    unsigned short int blockalign = 1;
    fwrite(&blockalign, 2, 1, file);
    unsigned short int bitspersample=8;
    fwrite(&bitspersample, 2, 1, file);

    //data chunk
    fwrite("data", 4, 1, file);
    fwrite(&bufferlength, 4, 1, file);
    fwrite(buffer, bufferlength, 1, file);

    fclose(file);
}


void OutputSound() {}

int debug = 0;

int samMain(const char* inText)
{

    int i;
    int phonetic = 0;

    char* wavfilename = "out.wav";
    char *input = malloc(strlen(inText));
    sprintf(input, inText);

    i = 1;
    for(i=0; input[i] != 0; i++)
        input[i] = toupper((int)input[i]);

    if (!phonetic)
    {
        strncat(input, "[", strlen(inText));
        if (!TextToPhonemes((unsigned char *)input)) return 1;
        if (debug)
            printf("phonetic input: %s\n", input);
    } else strncat(input, "\x9b", strlen(inText));

    SetInput(input);
    if(!SAMMain())
    {
        return -1;
    }

    if (wavfilename != NULL)
        //WriteWav(wavfilename, GetBuffer(), GetBufferLength()/50);
        PlayWavFromBuffer(GetBuffer(), GetBufferLength()/50);
    else
        OutputSound();

    free(input);
    return 0;

}
