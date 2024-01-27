/****************************************************************************
 * libwiigui Template
 * Tantric 2009
 *
 * menu.cpp
 * Menu flow routines - handles all menu logic
 ***************************************************************************/

#include <gccore.h>
#include <ogcsys.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <debug.h>
#include <wiiuse/wpad.h>
#include <wiikeyboard/keyboard.h>
#include <wiikeyboard/usbkeyboard.h>

#include "libwiigui/gui.h"
#include "menu.h"
#include "audio.h"
#include "demo.h"
#include "input.h"
#include "filelist.h"
#include "filebrowser.h"
#include "button_over_pcm.h"
#include "popup_pcm.h"
#include "bg_music_ogg.h"
#include "networking.h"
#include "download.h"
#include "loading_pcm.h"
#include "tada_pcm.h"

#define THREAD_SLEEP 100

static GuiImageData * pointer[4];
static GuiImage * bgImg = NULL;
static GuiSound * bgMusic = NULL;
static GuiWindow * mainWindow = NULL;
static lwp_t guithread = LWP_THREAD_NULL;
static lwp_t kbthread = LWP_THREAD_NULL;
static bool guiHalt = true;
// function to printf to usbgecko
void usbprintf(const char *fmt, ...){
	// buffer to store our formatted string
	char buffer[256];

	// format our string
	va_list argp;
	va_start(argp, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, argp);
	va_end(argp);

	// print it to usbgecko
	sendDataToUSBGecko(buffer, strlen(buffer));

	// return
	return;
}
/****************************************************************************
 * ResumeGui
 *
 * Signals the GUI thread to start, and resumes the thread. This is called
 * after finishing the removal/insertion of new elements, and after initial
 * GUI setup.
 ***************************************************************************/
static void
ResumeGui()
{
	guiHalt = false;
	LWP_ResumeThread (guithread);
}

/****************************************************************************
 * HaltGui
 *
 * Signals the GUI thread to stop, and waits for GUI thread to stop
 * This is necessary whenever removing/inserting new elements into the GUI.
 * This eliminates the possibility that the GUI is in the middle of accessing
 * an element that is being changed.
 ***************************************************************************/
static void
HaltGui()
{
	guiHalt = true;

	// wait for thread to finish
	while(!LWP_ThreadIsSuspended(guithread))
		usleep(THREAD_SLEEP);
}

int getLineCount(char* text)
{
	// increment untill we start seeing \0
	int i = 0;
	int chrPos = 0;
	int lines = 0;
	while(text[i] != '\0') // 30 is the max number of characters per line, this helps prevent buffer overflows
	{
		if(chrPos > 30)
		{
			lines++;
			chrPos = 0;
		}
		chrPos++;
		i++;
	}
	lines++;
	return lines;
}

char* getLines(char* t, int scrollBarPos)
{
	// see how many lines we have (includes \n and text wrapping)
	int lines = 0;
	int i = 0;
	int lineLength = 0; // length of the current line
	int lineStart = 0; // index of the start of the current line
	
	while(t[i] != '\0')
	{
		if(t[i] == '\n')
		{
			lines++;
			lineLength = 0;
			lineStart = i+1;
		}
		else
		{
			lineLength++;
			if(lineLength > 30) // wrap text
			{
				lines++;
				lineLength = 0;
				lineStart = i+1;
			}
		}
		i++;
	}
	lines++; // add one for the last line

	// allocate memory for the lines
	char** line = (char**)malloc(sizeof(char*)*lines+128);
	memset(line, 0, sizeof(char*)*lines+128);
	for(i = 0; i < lines; i++)
	{
		line[i] = (char*)malloc(sizeof(char)*31);
		memset(line[i], 0, 31);
	}

	// copy the lines
	int lineNum = 0;
	int linePos = 0;
	i = 0;
	while(t[i] != '\0')
	{
		if(t[i] == '\n')
		{
			lineNum++;
			linePos = 0;
		}
		else
		{
			line[lineNum][linePos] = t[i];
			linePos++;
			if(linePos > 30) // wrap text
			{
				lineNum++;
				linePos = 0;
			}
		}
		i++;
	}
	lineNum++;

	// now we have the lines, we can return the one we want
	return line[scrollBarPos];
}

#define maxLines 4

char* getLine(char* t, int startLine)
{
	// allocate memory for the lines
	char * lines[maxLines];
	for(int i = 0; i < maxLines; i++)
	{
		lines[i] = (char*)malloc(sizeof(char)*31);
		memset(lines[i], 0, 31);
	}

	// copy the lines
	for(int i=startLine; i<startLine+1*maxLines; i++)
	{
		char* curLine = getLines(t, i);
		strcpy(lines[i-startLine], curLine);
		free(curLine);
	}

	char* allLines = (char*)malloc(sizeof(char)*31*maxLines);
	memset(allLines, 0, 31*maxLines);
	for(int i=0; i<maxLines; i++)
	{
		strcat(allLines, lines[i]);
		strcat(allLines, "\n");
		free(lines[i]);
	}

	return allLines;
}

char* replacedoubleBackslashEscapeCodeWithStandardEscapeCode(char* text)
{
	char* newText = (char*)malloc(sizeof(char)*strlen(text));
	memset(newText, 0, strlen(text));
	int i = 0;
	int j = 0;
	while(text[i] != '\0')
	{
		if(text[i] == '\\')
		{
			if(text[i+1] == '\\')
			{
				newText[j] = '\\';
				i++;
			}
			else if(text[i+1] == 'n')
			{
				newText[j] = '\n';
				i++;
			}
			else
			{
				newText[j] = text[i];
			}
		}
		else
		{
			newText[j] = text[i];
		}
		i++;
		j++;
	}
	return newText;
}

/****************************************************************************
 * WindowPrompt
 *
 * Displays a prompt window to user, with information, an error message, or
 * presenting a user with a choice
 ***************************************************************************/
int
WindowPrompt(const char *title, const char *msg, const char *btn1Label, const char *btn2Label)
{
	#ifdef DEBUG
	usbprintf("Window prompt!\n");
	#endif
	playSfx(popup_pcm, popup_pcm_size, 4);
	int choice = -1;

	GuiWindow promptWindow(448,288);
	promptWindow.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
	promptWindow.SetPosition(0, -10);
	GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
	GuiImageData btnOutline(button_png);
	GuiImageData btnOutlineOver(button_over_png);
	GuiTrigger trigA;
	GuiTrigger trigDown;
	GuiTrigger trigUp;
	GuiTrigger trigDownWpadDown;
	GuiTrigger trigUpWpadUp;

	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
	trigDown.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
	trigUp.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);

	// set up triggers for wiimote
	trigDownWpadDown.SetSimpleTrigger(-1, WPAD_BUTTON_DOWN | WPAD_CLASSIC_BUTTON_DOWN, PAD_BUTTON_DOWN);
	trigUpWpadUp.SetSimpleTrigger(-1, WPAD_BUTTON_UP | WPAD_CLASSIC_BUTTON_UP, PAD_BUTTON_UP);

	// hidden buttons for wiimote
	GuiButton trigDownWpadHidden(0, 0);
	trigDownWpadHidden.SetTrigger(&trigDownWpadDown);
	GuiButton trigUpWpadHidden(0, 0);
	trigUpWpadHidden.SetTrigger(&trigUpWpadUp);

	GuiImageData dialogBox(dialogue_box_png);
	GuiImage dialogBoxImg(&dialogBox);

	GuiText titleTxt(title, 26, (GXColor){0, 0, 0, 255});
	titleTxt.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
	titleTxt.SetPosition(0,40);

	GuiText btn1Txt(btn1Label, 22, (GXColor){0, 0, 0, 255});
	GuiImage btn1Img(&btnOutline);
	GuiImage btn1ImgOver(&btnOutlineOver);
	GuiButton btn1(btnOutline.GetWidth(), btnOutline.GetHeight());

	if(btn2Label)
	{
		btn1.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
		btn1.SetPosition(20, -25);
	}
	else
	{
		btn1.SetAlignment(ALIGN_CENTRE, ALIGN_BOTTOM);
		btn1.SetPosition(0, -25);
	}

	btn1.SetLabel(&btn1Txt);
	btn1.SetImage(&btn1Img);
	btn1.SetImageOver(&btn1ImgOver);
	btn1.SetSoundOver(&btnSoundOver);
	btn1.SetTrigger(&trigA);
	btn1.SetState(STATE_SELECTED);
	btn1.SetEffectGrow();

	GuiText btn2Txt(btn2Label, 22, (GXColor){0, 0, 0, 255});
	GuiImage btn2Img(&btnOutline);
	GuiImage btn2ImgOver(&btnOutlineOver);
	GuiButton btn2(btnOutline.GetWidth(), btnOutline.GetHeight());
	btn2.SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
	btn2.SetPosition(-20, -25);
	btn2.SetLabel(&btn2Txt);
	btn2.SetImage(&btn2Img);
	btn2.SetImageOver(&btn2ImgOver);
	btn2.SetSoundOver(&btnSoundOver);
	btn2.SetTrigger(&trigA);
	btn2.SetEffectGrow();

	// make 2 arrows for scrolling text
    GuiImageData arrowDown(scrollbar_arrowdown_png);
    GuiImage arrowDownImg(&arrowDown);
    GuiImage arrowDownOverImg(&arrowDown);
	GuiButton arrowDownBtn(arrowDown.GetWidth(), arrowDown.GetHeight());
    arrowDownBtn.SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
    arrowDownBtn.SetPosition(-20, -25);
    arrowDownBtn.SetImage(&arrowDownImg);
    arrowDownBtn.SetImageOver(&arrowDownOverImg);
    arrowDownBtn.SetTrigger(&trigDown);
    arrowDownBtn.SetEffectGrow();

    GuiImageData arrowUp(scrollbar_arrowup_png);
    GuiImage arrowUpImg(&arrowUp);
    GuiImage arrowUpOverImg(&arrowUp);
	GuiButton arrowUpBtn(arrowUp.GetWidth(), arrowUp.GetHeight());
    arrowUpBtn.SetAlignment(ALIGN_RIGHT, ALIGN_TOP);
    arrowUpBtn.SetPosition(-20, 25);
    arrowUpBtn.SetImage(&arrowUpImg);
    arrowUpBtn.SetImageOver(&arrowUpOverImg);
    arrowUpBtn.SetTrigger(&trigUp);
    arrowUpBtn.SetEffectGrow();

	// create 4 lines of text
	GuiText msgTxt1(getLines((char*)msg, 0), 22, (GXColor){0, 0, 0, 255});
	GuiText msgTxt2(getLines((char*)msg, 1), 22, (GXColor){0, 0, 0, 255});
	GuiText msgTxt3(getLines((char*)msg, 2), 22, (GXColor){0, 0, 0, 255});
	GuiText msgTxt4(getLines((char*)msg, 3), 22, (GXColor){0, 0, 0, 255});
	msgTxt1.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
	msgTxt1.SetPosition(0, 70);
	msgTxt2.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
	msgTxt2.SetPosition(0, 100);
	msgTxt3.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
	msgTxt3.SetPosition(0, 130);
	msgTxt4.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
	msgTxt4.SetPosition(0, 160);

	// debuggin text
	GuiText debugTxt("Debugging text", 22, (GXColor){0, 0, 0, 255});
	// put it in the top left
	debugTxt.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	debugTxt.SetPosition(0, 0);

	promptWindow.Append(&dialogBoxImg);
	promptWindow.Append(&titleTxt);
	promptWindow.Append(&msgTxt1);
	promptWindow.Append(&msgTxt2);
	promptWindow.Append(&msgTxt3);
	promptWindow.Append(&msgTxt4);
	promptWindow.Append(&btn1);
	promptWindow.Append(&arrowDownBtn);
	promptWindow.Append(&arrowUpBtn);
	promptWindow.Append(&trigDownWpadHidden);
	promptWindow.Append(&trigUpWpadHidden);
	promptWindow.Append(&debugTxt);

	if(btn2Label)
		promptWindow.Append(&btn2);

	promptWindow.SetEffect(EFFECT_SLIDE_TOP | EFFECT_SLIDE_IN, 50);
	HaltGui();
	mainWindow->SetState(STATE_DISABLED);
	mainWindow->Append(&promptWindow);
	mainWindow->ChangeFocus(&promptWindow);
	ResumeGui();

	int l = 0;

	// allocate memory for size of message + 5 (for \n and \0)
	char* msg2 = (char*)malloc(sizeof(char)*(strlen(msg)+50));
	memset(msg2, '\0', sizeof(char) * (strlen(msg) + 50)); // Initialize the buffer with null characters
	
	// copy the message into the new string
	strcpy(msg2, msg);
	// fill the rest of the buffer with \0
	for(int i = strlen(msg); i < strlen(msg)+50; i++)
	{
		msg2[i] = '\0';
	}
	// get the number of lines in the message
	volatile int lines = getLineCount((char*)msg2); // volatile so that the compiler doesn't optimize it out
	char dbgMsg[256];
	sprintf(dbgMsg, "lines: %d", lines);
	debugTxt.SetText(dbgMsg);
	// some cool debugging shizz
	#ifdef DEBUG
	usbprintf("msg2: %s\r\n", msg2);
	usbprintf("lines: %d\r\n", lines);
	#endif

	while(choice == -1)
	{
		usleep(THREAD_SLEEP);

		if(btn1.GetState() == STATE_CLICKED)
			choice = 1;
		else if(btn2.GetState() == STATE_CLICKED)
			choice = 0;
		
		// set the text to display at plus one line if the down button is pressed
        if (arrowDownBtn.GetState() == STATE_CLICKED || trigDownWpadHidden.GetState() == STATE_CLICKED)
		{
            l++;
			memset(dbgMsg, 0, 256);
			sprintf(dbgMsg, "l: %d", l);
			debugTxt.SetText(dbgMsg);
			if(l >= (lines) ) // if l is greater than the number of lines in the message minus the number of lines we can display
			{
				#ifdef DEBUG
				usbprintf("l: %d\r\n", l);
				usbprintf("lines: %d\r\n", lines);
				#endif
				//l--; // don't go past the end of the message
				debugTxt.SetColor((GXColor){255, 0, 0, 255});
				// check for \0 in the bottom line, if it starts with \0, then we're at the end of the message
				if(msg2[l*29] == '\0') // get the character at the start of the line
				{
					l--; // don't go past the end of the message
				}
			}
			// filter out \0 to spaces
			char* line1 = getLines((char*)msg2, 0 + l);
			char* line2 = getLines((char*)msg2, 1 + l);
			char* line3 = getLines((char*)msg2, 2 + l);
			char* line4 = getLines((char*)msg2, 3 + l);

			// replace \0 with spaces
			for(int i = 0; i < 30; i++)
			{
				if(line1[i] == '\0')
				{
					line1[i] = ' ';
				}
				if(line2[i] == '\0')
				{
					line2[i] = ' ';
				}
				if(line3[i] == '\0')
				{
					line3[i] = ' ';
				}
				if(line4[i] == '\0')
				{
					line4[i] = ' ';
				}
			}

			// set the text
            msgTxt1.SetText(line1);
			msgTxt2.SetText(line2);
			msgTxt3.SetText(line3);
			msgTxt4.SetText(line4);
            // wait 100ms before scrolling again
            usleep(10000);
            // untrigger the button
            arrowDownBtn.SetState(STATE_DEFAULT);
			trigDownWpadHidden.SetState(STATE_DEFAULT);
        }
        // set the text to display at minus one line if the up button is pressed
        else if (arrowUpBtn.GetState() == STATE_CLICKED || trigUpWpadHidden.GetState() == STATE_CLICKED)
        {
            l--;
			memset(dbgMsg, 0, 256);
			sprintf(dbgMsg, "l: %d", l);
			debugTxt.SetText(dbgMsg);
			if(l < 0)
			{
				l = 0; // don't go past the start of the message
			}
            msgTxt1.SetText(getLines((char*)msg2, 0 + l));
            msgTxt2.SetText(getLines((char*)msg2, 1 + l));
            msgTxt3.SetText(getLines((char*)msg2, 2 + l));
            msgTxt4.SetText(getLines((char*)msg2, 3 + l));
            // wait 100ms before scrolling again
            usleep(10000);
            // untrigger the button
            arrowUpBtn.SetState(STATE_DEFAULT);
			trigUpWpadHidden.SetState(STATE_DEFAULT);
        }
	}

	promptWindow.SetEffect(EFFECT_SLIDE_TOP | EFFECT_SLIDE_OUT, 50);
	while(promptWindow.GetEffect() > 0) usleep(THREAD_SLEEP);
	HaltGui();
	mainWindow->Remove(&promptWindow);
	mainWindow->SetState(STATE_DEFAULT);
	ResumeGui();
	return choice;
}
int
WindowPromptSmall(const char *title, const char *msg, const char *btn1Label, const char *btn2Label)
{
	playSfx(popup_pcm, popup_pcm_size, 4);
	int choice = -1;

	GuiWindow promptWindow(448,288);
	promptWindow.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
	promptWindow.SetPosition(0, -10);
	GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
	GuiImageData btnOutline(button_png);
	GuiImageData btnOutlineOver(button_over_png);
	GuiTrigger trigA;
	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);

	GuiImageData dialogBox(dialogue_box_png);
	GuiImage dialogBoxImg(&dialogBox);

	GuiText titleTxt(title, 26, (GXColor){0, 0, 0, 255});
	titleTxt.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
	titleTxt.SetPosition(0,40);
	GuiText msgTxt(msg, 22, (GXColor){0, 0, 0, 255});
	msgTxt.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
	msgTxt.SetPosition(0,-20);
	msgTxt.SetWrap(true, 400);

	GuiText btn1Txt(btn1Label, 22, (GXColor){0, 0, 0, 255});
	GuiImage btn1Img(&btnOutline);
	GuiImage btn1ImgOver(&btnOutlineOver);
	GuiButton btn1(btnOutline.GetWidth(), btnOutline.GetHeight());

	if(btn2Label)
	{
		btn1.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
		btn1.SetPosition(20, -25);
	}
	else
	{
		btn1.SetAlignment(ALIGN_CENTRE, ALIGN_BOTTOM);
		btn1.SetPosition(0, -25);
	}

	btn1.SetLabel(&btn1Txt);
	btn1.SetImage(&btn1Img);
	btn1.SetImageOver(&btn1ImgOver);
	btn1.SetSoundOver(&btnSoundOver);
	btn1.SetTrigger(&trigA);
	btn1.SetState(STATE_SELECTED);
	btn1.SetEffectGrow();

	GuiText btn2Txt(btn2Label, 22, (GXColor){0, 0, 0, 255});
	GuiImage btn2Img(&btnOutline);
	GuiImage btn2ImgOver(&btnOutlineOver);
	GuiButton btn2(btnOutline.GetWidth(), btnOutline.GetHeight());
	btn2.SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
	btn2.SetPosition(-20, -25);
	btn2.SetLabel(&btn2Txt);
	btn2.SetImage(&btn2Img);
	btn2.SetImageOver(&btn2ImgOver);
	btn2.SetSoundOver(&btnSoundOver);
	btn2.SetTrigger(&trigA);
	btn2.SetEffectGrow();

	promptWindow.Append(&dialogBoxImg);
	promptWindow.Append(&titleTxt);
	promptWindow.Append(&msgTxt);
	promptWindow.Append(&btn1);

	if(btn2Label)
		promptWindow.Append(&btn2);

	promptWindow.SetEffect(EFFECT_SLIDE_TOP | EFFECT_SLIDE_IN, 50);
	HaltGui();
	mainWindow->SetState(STATE_DISABLED);
	mainWindow->Append(&promptWindow);
	mainWindow->ChangeFocus(&promptWindow);
	ResumeGui();

	while(choice == -1)
	{
		usleep(THREAD_SLEEP);

		if(btn1.GetState() == STATE_CLICKED)
			choice = 1;
		else if(btn2.GetState() == STATE_CLICKED)
			choice = 0;
	}

	promptWindow.SetEffect(EFFECT_SLIDE_TOP | EFFECT_SLIDE_OUT, 50);
	while(promptWindow.GetEffect() > 0) usleep(THREAD_SLEEP);
	HaltGui();
	mainWindow->Remove(&promptWindow);
	mainWindow->SetState(STATE_DEFAULT);
	ResumeGui();
	return choice;
}
/****************************************************************************
 * UpdateGUI
 *
 * Primary thread to allow GUI to respond to state changes, and draws GUI
 ***************************************************************************/
bool doLoading=false;
void loadSound();

static void *
UpdateGUI (void *arg)
{
	int i;

	while(1)
	{
		if(guiHalt)
		{
			LWP_SuspendThread(guithread);
			VIDEO_WaitVSync();
		}
		else
		{
			loadSound();
			UpdatePads();
			mainWindow->Draw();

			#ifdef HW_RVL
			for(i=3; i >= 0; i--) // so that player 1's cursor appears on top!
			{
				if(userInput[i].wpad->ir.valid)
					Menu_DrawImg(userInput[i].wpad->ir.x-48, userInput[i].wpad->ir.y-48,
						96, 96, pointer[i]->GetImage(), userInput[i].wpad->ir.angle, 1, 1, 255);
				DoRumble(i);
			}
			#endif

			Menu_Render();

			for(i=0; i < 4; i++)
				mainWindow->Update(&userInput[i]);

			if(ExitRequested)
			{
				for(i = 0; i <= 255; i += 15)
				{
					mainWindow->Draw();
					Menu_DrawRectangle(0,0,screenwidth,screenheight,(GXColor){0, 0, 0, i},1);
					Menu_Render();
				}
				ExitApp();
			}
		}
	}
	return NULL;
}
bool kbdOn = false;
// kbThread stack size
#define STACKSIZE 16384
// stack for kbThread
static char kbThreadStack[STACKSIZE] ATTRIBUTE_ALIGN(32);
static void * KeyboardThread(void* arg)
{
	keyboard_event ke;
	KEYBOARD_Init(kbEvent);
	while(1)
	{
		usleep(THREAD_SLEEP);
		if(kbdOn)
			USBKeyboard_Scan();
			KEYBOARD_GetEvent(&ke);
			KEYBOARD_FlushEvents();
	}
	return NULL;
}

/****************************************************************************
 * InitGUIThread
 *
 * Startup GUI threads
 ***************************************************************************/
void loadSound()
{
	if(doLoading)
	{
		if(!audioPlaying(4))
		{
			playSfx(loading_pcm, loading_pcm_size, 4);
		}
	}
}

void
InitGUIThreads()
{
	//LWP_CreateThread (&kbthread, KeyboardThread, NULL, NULL, 0, 71);
	LWP_CreateThread (&guithread, UpdateGUI, NULL, NULL, 0, 70);
	LWP_CreateThread (&kbthread, KeyboardThread, NULL, kbThreadStack, STACKSIZE, 70);
}

char kb_buffer[1024];
int kb_pos = 0;
bool kb_done = false;
void keyPress_cb(char sym)
{
	usleep(THREAD_SLEEP);

	// show changes on screen
	VIDEO_WaitVSync();
}
static char* getUsbInput()
{
	// init usb keyboard
	KEYBOARD_Init(keyPress_cb);

	usleep(THREAD_SLEEP);

	// video is finicky, so we need to wait for a vsync
	VIDEO_WaitVSync();

	// fill kb buffer with 0
	memset(kb_buffer, 0, 1024);

	// reset kb pos
	kb_pos = 0;

	// reset kb done
	kb_done = false;

	bool shift = false;

	// print prompt
	// back to the top left
	printf("\x1b[0;0H");
	printf("Enter text: ");

	// wait for kb done
	while(!kb_done)
	{
		usleep(THREAD_SLEEP);
		char sym = getchar();
		if(sym == '\b')
		{
			if(kb_pos > 0)
			{
				kb_buffer[kb_pos-1] = '\0';
				kb_pos--;
				printf("\b");
			}
		}
		// shift
		else if(sym == '\x1b')
		{
			shift = true;
		}
		else if(sym == '\r') // enter
		{
			kb_done = true;
		}
		else
		{
			if(shift)
			{
				if(sym >= 'a' && sym <= 'z')
				{
					sym -= 32;
				}
				shift = false;
			}
			else {
				kb_buffer[kb_pos] = sym;
				kb_pos++;
				putchar(sym);
			}
		}
	}

	// copy kb buffer to var
	char* var = (char*)malloc(sizeof(char)*kb_pos);
	memset(var, 0, kb_pos);
	strcpy(var, kb_buffer);

	// deinit usb keyboard
	KEYBOARD_Deinit();

	return var;
}

/****************************************************************************
 * OnScreenKeyboard
 *
 * Opens an on-screen keyboard window, with the data entered being stored
 * into the specified variable.
 ***************************************************************************/
static void OnScreenKeyboard(char * var, u16 maxlen)
{
	int save = -1;

	GuiKeyboard keyboard(var, maxlen);

	GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
	GuiImageData btnOutline(button_png);
	GuiImageData btnOutlineOver(button_over_png);
	GuiTrigger trigA;
	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);

	GuiText okBtnTxt("OK", 22, (GXColor){0, 0, 0, 255});
	GuiImage okBtnImg(&btnOutline);
	GuiImage okBtnImgOver(&btnOutlineOver);
	GuiButton okBtn(btnOutline.GetWidth(), btnOutline.GetHeight());

	okBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
	okBtn.SetPosition(25, -25);

	okBtn.SetLabel(&okBtnTxt);
	okBtn.SetImage(&okBtnImg);
	okBtn.SetImageOver(&okBtnImgOver);
	okBtn.SetSoundOver(&btnSoundOver);
	okBtn.SetTrigger(&trigA);
	okBtn.SetEffectGrow();

	GuiText cancelBtnTxt("Cancel", 22, (GXColor){0, 0, 0, 255});
	GuiImage cancelBtnImg(&btnOutline);
	GuiImage cancelBtnImgOver(&btnOutlineOver);
	GuiButton cancelBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
	cancelBtn.SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
	cancelBtn.SetPosition(-25, -25);
	cancelBtn.SetLabel(&cancelBtnTxt);
	cancelBtn.SetImage(&cancelBtnImg);
	cancelBtn.SetImageOver(&cancelBtnImgOver);
	cancelBtn.SetSoundOver(&btnSoundOver);
	cancelBtn.SetTrigger(&trigA);
	cancelBtn.SetEffectGrow();

	// option for usb keyboard
	GuiText usbBtnTxt("USB Keyboard", 22, (GXColor){0, 0, 0, 255});
	GuiImage usbBtnImg(&btnOutline);
	GuiImage usbBtnImgOver(&btnOutlineOver);
	GuiButton usbBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
	usbBtn.SetAlignment(ALIGN_CENTRE, ALIGN_BOTTOM);
	usbBtn.SetPosition(0, -25);
	usbBtn.SetLabel(&usbBtnTxt);
	usbBtn.SetImage(&usbBtnImg);
	usbBtn.SetImageOver(&usbBtnImgOver);
	usbBtn.SetSoundOver(&btnSoundOver);
	usbBtn.SetTrigger(&trigA);
	usbBtn.SetEffectGrow();

	keyboard.Append(&okBtn);
	keyboard.Append(&usbBtn);
	keyboard.Append(&cancelBtn);

	HaltGui();
	mainWindow->SetState(STATE_DISABLED);
	mainWindow->Append(&keyboard);
	mainWindow->ChangeFocus(&keyboard);
	ResumeGui();

	bool usbKeyboard = false;
	kbdOn = true;
	while(save == -1)
	{
		usleep(THREAD_SLEEP);

		if(okBtn.GetState() == STATE_CLICKED)
			save = 1;
		else if(usbBtn.GetState() == STATE_CLICKED)
		{
			usbKeyboard = true;
			// stop the gui and move the console to the top left
			HaltGui();
			//getUsbInput();
			// get the input from the usb keyboard
			char* usbInput = getUsbInput();
			printf("\n%s", usbInput);
			snprintf(var, maxlen, "%s", usbInput);
			usleep(1000000); // wait 1 second
			ResumeGui();
			save = 1;
		}
		else if(cancelBtn.GetState() == STATE_CLICKED)
			save = 0;
	}

	if(save && !usbKeyboard)
	{
		snprintf(var, maxlen, "%s", keyboard.kbtextstr);
	}
	kbdOn = false;
	HaltGui();
	mainWindow->Remove(&keyboard);
	mainWindow->SetState(STATE_DEFAULT);
	ResumeGui();
}

/****************************************************************************
 * MenuSettings
 ***************************************************************************/
static int MenuSettings()
{
	FILE* f;

	int menu = MENU_NONE;

	GuiText titleTxt("OpenAI Client", 28, (GXColor){255, 255, 255, 255});
	titleTxt.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	titleTxt.SetPosition(50,50);

	GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
	GuiImageData btnOutline(button_png);
	GuiImageData btnOutlineOver(button_over_png);
	GuiImageData btnLargeOutline(button_large_png);
	GuiImageData btnLargeOutlineOver(button_large_over_png);

	GuiTrigger trigA;
	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
	GuiTrigger trigHome;
	trigHome.SetButtonOnlyTrigger(-1, WPAD_BUTTON_HOME | WPAD_CLASSIC_BUTTON_HOME, 0);

	GuiText exitBtnTxt("Exit", 22, (GXColor){0, 0, 0, 255});
	GuiImage exitBtnImg(&btnOutline);
	GuiImage exitBtnImgOver(&btnOutlineOver);
	GuiButton exitBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
	exitBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
	exitBtn.SetPosition(100, -35);
	exitBtn.SetLabel(&exitBtnTxt);
	exitBtn.SetImage(&exitBtnImg);
	exitBtn.SetImageOver(&exitBtnImgOver);
	exitBtn.SetSoundOver(&btnSoundOver);
	exitBtn.SetTrigger(&trigA);
	exitBtn.SetTrigger(&trigHome);
	exitBtn.SetEffectGrow();

	// our own button for Chat GPT Client
	GuiText chatBtnTxt("OpenAI Client", 22, (GXColor){0, 0, 0, 255});
	chatBtnTxt.SetWrap(true, btnLargeOutline.GetWidth()-30);
	GuiImage chatBtnImg(&btnLargeOutline);
	GuiImage chatBtnImgOver(&btnLargeOutlineOver);
	GuiButton chatBtn(btnLargeOutline.GetWidth(), btnLargeOutline.GetHeight());
	chatBtn.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
	chatBtn.SetPosition(0, 100);
	chatBtn.SetLabel(&chatBtnTxt);
	chatBtn.SetImage(&chatBtnImg);
	chatBtn.SetImageOver(&chatBtnImgOver);
	chatBtn.SetSoundOver(&btnSoundOver);
	chatBtn.SetTrigger(&trigA);
	chatBtn.SetEffectGrow();

	GuiText openAIBtnTxt("Dalle", 22, (GXColor){0, 0, 0, 255});
	openAIBtnTxt.SetWrap(true, btnLargeOutline.GetWidth()-30);
	GuiImage openAIBtnImg(&btnLargeOutline);
	GuiImage openAIBtnImgOver(&btnLargeOutlineOver);
	GuiButton openAIBtn(btnLargeOutline.GetWidth(), btnLargeOutline.GetHeight());
	openAIBtn.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
	openAIBtn.SetPosition(0, 250);
	openAIBtn.SetLabel(&openAIBtnTxt);
	openAIBtn.SetImage(&openAIBtnImg);
	openAIBtn.SetImageOver(&openAIBtnImgOver);
	openAIBtn.SetSoundOver(&btnSoundOver);
	openAIBtn.SetTrigger(&trigA);
	openAIBtn.SetEffectGrow();

	GuiText updateBtnTxt("Update", 22, (GXColor){0, 0, 0, 255});
	GuiImage updateBtnImg(&btnOutline);
	GuiImage updateBtnImgOver(&btnOutlineOver);
	GuiButton updateBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
	updateBtn.SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
	updateBtn.SetPosition(-100, -35);
	updateBtn.SetLabel(&updateBtnTxt);
	updateBtn.SetImage(&updateBtnImg);
	updateBtn.SetImageOver(&updateBtnImgOver);
	updateBtn.SetSoundOver(&btnSoundOver);
	updateBtn.SetTrigger(&trigA);
	updateBtn.SetEffectGrow();

	HaltGui();
	GuiWindow w(screenwidth, screenheight);
	w.Append(&titleTxt);
	w.Append(&chatBtn);
	w.Append(&updateBtn);
	w.Append(&openAIBtn);

	w.Append(&exitBtn);
	
	mainWindow->Append(&w);

	ResumeGui();

	while(menu == MENU_NONE)
	{
		usleep(THREAD_SLEEP);

		if(chatBtn.GetState() == STATE_CLICKED)
		{
			menu = MENU_CHAT_GPT_CLIENT;
		}
		else if(updateBtn.GetState() == STATE_CLICKED)
		{
			menu = MENU_UPDATES;
		}
		else if(openAIBtn.GetState() == STATE_CLICKED)
		{
			menu = MENU_OPENAI_DALLE;
		}
		else if(exitBtn.GetState() == STATE_CLICKED)
		{
			menu = MENU_EXIT;
		}
	}

	HaltGui();
	mainWindow->Remove(&w);
	return menu;
}

/****************************************************************************
 * ChatGPTClient
 ***************************************************************************/

static int ChatGPTClient()
{
	// define some cool variables
	int menu = MENU_NONE;

	// define our buttons
	GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
	GuiImageData btnOutline(button_png);
	GuiImageData btnOutlineOver(button_over_png);

	GuiTrigger trigA;
	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);

	GuiText backBtnTxt("Go Back", 22, (GXColor){0, 0, 0, 255});
	GuiImage backBtnImg(&btnOutline);
	GuiImage backBtnImgOver(&btnOutlineOver);
	GuiButton backBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
	backBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
	backBtn.SetPosition(100, -35);
	backBtn.SetLabel(&backBtnTxt);
	backBtn.SetImage(&backBtnImg);
	backBtn.SetImageOver(&backBtnImgOver);
	backBtn.SetSoundOver(&btnSoundOver);
	backBtn.SetTrigger(&trigA);
	backBtn.SetEffectGrow();

	// put that pasta together
	HaltGui();
	GuiWindow w(screenwidth, screenheight); // our window
	w.Append(&backBtn); // append our back button.
	mainWindow->Append(&w); // append our window
	ResumeGui();
	// end of that pasta
	int index = 0;
	while(menu == MENU_NONE) {
		usleep(THREAD_SLEEP); // sleep for a bit

		//WindowPrompt("Chat GPT Client", "This is a test", "OK", NULL);
		char output[256];
		memset(output, 0, 256);
		OnScreenKeyboard(output, 256);
		//WindowPrompt("Chat GPT Client", output, "OK", NULL);
		// ask openai for a response
		char* response;
		char *prompt = output;
		doLoading=true;
		response = ask_chat_model(prompt);
		doLoading=false;
		WindowPrompt("Chat GPT Client", replacedoubleBackslashEscapeCodeWithStandardEscapeCode(response), "OK", NULL);
		menu = MENU_SETTINGS; // set menu to MENU_SETTINGS

		// add somethings to our main window

		if(backBtn.GetState() == STATE_CLICKED) // set menu to MENU_SETTINGS if back button is clicked
		{
			menu = MENU_SETTINGS;
		}
	}
	HaltGui(); // halt the gui
	mainWindow->Remove(&w); // remove the window
	return menu; // return the menu
}

static int OpenAIDalle()
{
	// define some cool variables
	int menu = MENU_NONE;

	// define our buttons
	GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
	GuiImageData btnOutline(button_png);
	GuiImageData btnOutlineOver(button_over_png);

	GuiTrigger trigA;
	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);

	GuiText backBtnTxt("Go Back", 22, (GXColor){0, 0, 0, 255});
	GuiImage backBtnImg(&btnOutline);
	GuiImage backBtnImgOver(&btnOutlineOver);
	GuiButton backBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
	backBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
	backBtn.SetPosition(100, -35);
	backBtn.SetLabel(&backBtnTxt);
	backBtn.SetImage(&backBtnImg);
	backBtn.SetImageOver(&backBtnImgOver);
	backBtn.SetSoundOver(&btnSoundOver);
	backBtn.SetTrigger(&trigA);
	backBtn.SetEffectGrow();

	// put that pasta together
	HaltGui();
	GuiWindow w(screenwidth, screenheight); // our window
	w.Append(&backBtn); // append our back button.
	mainWindow->Append(&w); // append our window
	ResumeGui();
	// end of that pasta
	int index = 0;
	bool disp=false;
	while(menu == MENU_NONE) {
		usleep(THREAD_SLEEP); // sleep for a bit

		//WindowPrompt("Chat GPT Client", "This is a test", "OK", NULL);
		if(!disp)
		{
			disp=true;
			char output[256];
			memset(output, 0, 256);
			OnScreenKeyboard(output, 256);
			//WindowPrompt("Chat GPT Client", output, "OK", NULL);
			// ask openai for a response
			char* response;
			char *prompt = output;
			//response = ask_chat_model(prompt);
			doLoading=true;
			generate_image(prompt, 256, 256);
			playSfx(tada_pcm, tada_pcm_size, 5);
			FILE *fp = fopen("generated_image.png", "rb");
			fseek(fp, 0, SEEK_END);
			int size = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			u8 *buffer = (u8*)malloc(size);
			fread(buffer, 1, size, fp);
			fclose(fp);
			GuiImageData downloadedImage(buffer);
			GuiImage downloadedImageImg(&downloadedImage);
			// set its position to the center of the screen
			downloadedImageImg.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
			downloadedImageImg.SetPosition(0, 0);
			doLoading=false;
			HaltGui();
			w.Append(&downloadedImageImg);
			ResumeGui();
		}
		// add somethings to our main window

		if(backBtn.GetState() == STATE_CLICKED) // set menu to MENU_SETTINGS if back button is clicked
		{
			menu = MENU_SETTINGS;
		}
	}
	HaltGui(); // halt the gui
	mainWindow->Remove(&w); // remove the window
	return menu; // return the menu
}

static int SelfUpdaterMenu()
{
	int menu = MENU_NONE;

	// define our buttons
	GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
	GuiImageData btnOutline(button_png);
	GuiImageData btnOutlineOver(button_over_png);

	GuiTrigger trigA;
	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);

	GuiText backBtnTxt("Go Back", 22, (GXColor){0, 0, 0, 255});
	GuiImage backBtnImg(&btnOutline);
	GuiImage backBtnImgOver(&btnOutlineOver);
	GuiButton backBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
	backBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
	backBtn.SetPosition(100, -35);
	backBtn.SetLabel(&backBtnTxt);
	backBtn.SetImage(&backBtnImg);
	backBtn.SetImageOver(&backBtnImgOver);
	backBtn.SetSoundOver(&btnSoundOver);
	backBtn.SetTrigger(&trigA);
	backBtn.SetEffectGrow();

	// put that pasta together
	HaltGui();
	GuiWindow w(screenwidth, screenheight); // our window
	w.Append(&backBtn); // append our back button.
	mainWindow->Append(&w); // append our window
	ResumeGui();
	// end of that pasta
	while(menu == MENU_NONE) {
		usleep(THREAD_SLEEP); // sleep for a bit

		int choice = WindowPromptSmall("Welcome to self Updater", "Check for updates?", "OK", "NO");
		if(choice == 1)
		{
			// check for updates
			WindowPromptSmall("Welcome to self Updater", "Checking for updates...", "OK", NULL);
			UpdateData updateDat = checkUpdates();
			if(updateDat.update_available)
			{
				WindowPromptSmall("Welcome to self Updater", "Update available!", "OK", NULL);
				WindowPromptSmall("The version on the server:", updateDat.version, "OK", NULL);
				WindowPromptSmall("Release Notes:", updateDat.relNotes, "OK", NULL);
				WindowPromptSmall("Downloading update...", "Downloading update...", "OK", NULL);
				doLoading=true;
				updateSelf(mainWindow);
				doLoading=false;
				WindowPromptSmall("Welcome to self Updater", "Update downloaded!", "OK", NULL);
				menu = MENU_EXIT;
			}
			else
			{
				WindowPromptSmall("Welcome to self Updater", "No updates available!", "OK", NULL);
				WindowPromptSmall("The version on the server:", updateDat.version, "OK", NULL);
				menu = MENU_SETTINGS;
			}
		}
		else if(choice == 0)
		{
			// do nothing
		}

		if(backBtn.GetState() == STATE_CLICKED) // set menu to MENU_SETTINGS if back button is clicked
		{
			menu = MENU_SETTINGS;
		}
	}
	HaltGui(); // halt the gui
	mainWindow->Remove(&w); // remove the window
	return menu; // return the menu
}

/****************************************************************************
 * MainMenu
 ***************************************************************************/
void MainMenu(int menu)
{
	int currentMenu = menu;

	#ifdef HW_RVL
	pointer[0] = new GuiImageData(player1_point_png);
	pointer[1] = new GuiImageData(player2_point_png);
	pointer[2] = new GuiImageData(player3_point_png);
	pointer[3] = new GuiImageData(player4_point_png);
	#endif

	mainWindow = new GuiWindow(screenwidth, screenheight);

	bgImg = new GuiImage(screenwidth, screenheight, (GXColor){116, 170, 156});
	mainWindow->Append(bgImg);

	GuiTrigger trigA;
	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);

	ResumeGui();

	bgMusic = new GuiSound(bg_music_ogg, bg_music_ogg_size, SOUND_OGG);
	bgMusic->SetVolume(64);
	bgMusic->SetLoop(1);
	bgMusic->Play(); // startup music

	while(currentMenu != MENU_EXIT)
	{
		switch (currentMenu)
		{
			case MENU_SETTINGS:
				currentMenu = MenuSettings();
				break;
			case MENU_CHAT_GPT_CLIENT:
				currentMenu = ChatGPTClient();
				break;
			case MENU_OPENAI_DALLE:
				currentMenu = OpenAIDalle();
				break;
			case MENU_UPDATES:
				currentMenu = SelfUpdaterMenu();
				break;
			default: // unrecognized menu
				currentMenu = MenuSettings();
				break;
		}
	}

	ResumeGui();
	ExitRequested = 1;
	while(1) usleep(THREAD_SLEEP);

	HaltGui();

	bgMusic->Stop();
	delete bgMusic;
	delete bgImg;
	delete mainWindow;

	delete pointer[0];
	delete pointer[1];
	delete pointer[2];
	delete pointer[3];

	mainWindow = NULL;
}
