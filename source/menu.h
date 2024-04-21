/****************************************************************************
 * libwiigui Template
 * Tantric 2009
 *
 * menu.h
 * Menu flow routines - handles all menu logic
 ***************************************************************************/

#ifndef _MENU_H_
#define _MENU_H_

#include <ogcsys.h>

void InitGUIThreads();
void MainMenu (int menuitem);
int WindowPromptSmall(const char *title, const char *msg, const char *btn1Label, const char *btn2Label);
int WindowPrompt(const char *title, const char *msg, const char *btn1Label, const char *btn2Label);

enum
{
	MENU_EXIT = -1,
	MENU_NONE,
	MENU_CHAT_GPT_CLIENT,
	MENU_OPENAI_DALLE,
	MENU_UPDATES,
	MENU_SETTINGS
};

#endif
