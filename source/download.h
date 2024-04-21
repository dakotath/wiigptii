#include "libwiigui/gui.h"
void updateSelf(GuiWindow *mainWindow);

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <curl/curl.h>

// struct for update check
struct UpdateData {
  bool update_available;
  char* version;
  char* relNotes;
};

void logVersionInfo();
CURLcode curl_download(char *url, FILE *fp);
CURLcode curl_download_buffer(char *url, char* buffer);
UpdateData checkUpdates();
char* ask_chat_model(const char* question);
void generate_image(const char* prompt, int width, int height);

#ifdef __cplusplus
}
#endif
