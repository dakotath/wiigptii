#include "menu.h"
#include "download.h"
#include "audio.h"
#include "libcJSON/cJSON.h"
#include "libcJSON/cJSON_Utils.h"
#include "libwiigui/gui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <string>
#include <gccore.h>
#include <ogcsys.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "libsam/sam.h"

// Can we haz updates?
bool canHazUpdate = false;
char* latestVersion = "000.000.000";

// Debug Log.
FILE* debugLog;

// Define your OpenAI API endpoint and key
const char* payload_template = "{\"model\":\"%s\", \"messages\": [{\"role\": \"system\", \"content\": \"You are a helpful assistant.\\n.\"}, {\"role\": \"user\", \"content\": \"%s\"}]}";
const char* request_template = "{\"prompt\": \"%s\", \"model\": \"%s\", \"size\": \"%dx%d\"}";
const char* endpoint = "https://api.openai.com/v1/chat/completions";
const char* image_endpoint = "https://api.openai.com/v1/images/generations";
const char* updateCheck_endpoint = "https://dakotath.com/wiigptii/checkUpdate.php";
char* api_key = "sk-0000000000000000000000000000000000000000000000000000";
const char* text_model = "gpt-4-1106-vision-preview";

// Struct to hold the response data
struct ResponseData {
    char* data;
    size_t size;
};
struct MemoryStruct {
  char *memory;
  size_t size;
};

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

// update function
#define VERSION "1.0.8"
#define wiigptii_dol_path "sd:/apps/wiigptii/boot.dol"
#define wiigptii_meta_path "sd:/apps/wiigptii/meta.xml"
#define wiigptii_icon_path "sd:/apps/wiigptii/icon.png"

// Logger Functions.
void logError(const char* function, char* message)
{
    if(debugLog != NULL)
    {
        fprintf(debugLog, "%s() [Error]: %s\n", function, message);
    }
}

// For debug logger:
void logVersionInfo()
{
    if(debugLog != NULL)
    {
        // Basic Info.
        fprintf(debugLog, "WiiGPTii v%s Debug Log.\n", VERSION);
        fprintf(debugLog, "(c)2024 Dakota Thorpe. WiiGPTii is maintained and created by Dakotath.\n");
        fprintf(debugLog, "Is an update available (canHazUpdate): %s\n", canHazUpdate ? "true": "false");
        fprintf(debugLog, "Latest version on server: %s\n", latestVersion);
        
        // URL's.
        fprintf(debugLog, "Text Completions URL: %s\n", endpoint);
        fprintf(debugLog, "Image Generation URL: %s\n", image_endpoint);
        fprintf(debugLog, "Update Check URL: %s\n", updateCheck_endpoint);

        // Models.
        fprintf(debugLog, "Text Model: %s\n", text_model);

        // Request Templates.
        fprintf(debugLog, "Text Payload Template: %s\n", payload_template);
        fprintf(debugLog, "Image Payload Template: %s\n\n", request_template);
    }
}

// As rewriting the download commands for curl can get quite lengthy, here is a function.
CURLcode curl_download(char *url, FILE *fp) {
    // Log to debugger.
    if (debugLog != NULL) {
        time_t current_time = time(NULL); // Get current time
        char* time_str = ctime(&current_time); // Convert time to string
        time_str[strlen(time_str) - 1] = '\0'; // Remove newline character from time string
        fprintf(debugLog, "%s: called at %s\n", __FUNCTION__, time_str);
    }

    CURL *curl;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    CURLcode res;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            return res;
        }
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();

    return res;
}

void updateSelf(GuiWindow *mainWindow) {
    // Log to debugger.
    if (debugLog != NULL) {
        time_t current_time = time(NULL); // Get current time
        char* time_str = ctime(&current_time); // Convert time to string
        time_str[strlen(time_str) - 1] = '\0'; // Remove newline character from time string
        fprintf(debugLog, "%s: called at %s\n", __FUNCTION__, time_str);
    }

    // download the files
    GuiText dolUpdatingText("Updating dol...", 20, (GXColor){0, 0, 0, 0});
    GuiText metaUpdatingText("Updating meta...", 20, (GXColor){0, 0, 0, 0});
    GuiText iconUpdatingText("Updating icon...", 20, (GXColor){0, 0, 0, 0});
    dolUpdatingText.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
    metaUpdatingText.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
    iconUpdatingText.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
    mainWindow->Append(&dolUpdatingText);
    mainWindow->Append(&metaUpdatingText);
    mainWindow->Append(&iconUpdatingText);

    metaUpdatingText.SetColor((GXColor){255, 255, 255, 255});
    FILE* fp = fopen(wiigptii_meta_path, "wb");
    curl_download("https://dakotath.com/wiigptii/meta.xml", fp);
    fclose(fp);

    metaUpdatingText.SetColor((GXColor){0, 0, 0, 0});
    iconUpdatingText.SetColor((GXColor){255, 255, 255, 255});
    fp = fopen(wiigptii_icon_path, "wb");
    curl_download("https://dakotath.com/wiigptii/icon.png", fp);
    fclose(fp);

    iconUpdatingText.SetColor((GXColor){0, 0, 0, 0});
    dolUpdatingText.SetColor((GXColor){255, 255, 255, 255});
    fp = fopen(wiigptii_dol_path, "wb");
    curl_download("https://dakotath.com/wiigptii/boot.dol", fp);
    fclose(fp);
    dolUpdatingText.SetColor((GXColor){0, 0, 0, 0});

    // cleanup
    mainWindow->Remove(&dolUpdatingText);
    mainWindow->Remove(&metaUpdatingText);
    mainWindow->Remove(&iconUpdatingText);
}

// function to download it to a buffer
CURLcode curl_download_buffer(char *url, char* buffer) {
    // Log to debugger.
    if (debugLog != NULL) {
        time_t current_time = time(NULL); // Get current time
        char* time_str = ctime(&current_time); // Convert time to string
        time_str[strlen(time_str) - 1] = '\0'; // Remove newline character from time string
        fprintf(debugLog, "%s: called at %s\n", __FUNCTION__, time_str);
    }

    CURL *curl;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    CURLcode res;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, buffer);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            return res;
        }
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();

    return res;
}

// Write callback function to save response data
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL) {
    /* out of memory! */
    //printf("not enough memory (realloc returned NULL)\n");
    logError(__FUNCTION__, "Not Enough Memory (realloc returne NULL)");
    return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

void save_audio(const char* audio_url, const char* filename) {
    // Log to debugger.
    if (debugLog != NULL) {
        time_t current_time = time(NULL); // Get current time
        char* time_str = ctime(&current_time); // Convert time to string
        time_str[strlen(time_str) - 1] = '\0'; // Remove newline character from time string
        fprintf(debugLog, "%s: called at %s\n", __FUNCTION__, time_str);
    }

    CURL *curl;
    FILE *fp;
    CURLcode res;

    curl = curl_easy_init();
    if(curl) {
        fp = fopen(filename, "wb");
        if(fp == NULL) {
            char* err = (char*)malloc(256);
            sprintf(err, "Error opening file: %s\n", filename);
            logError(__FUNCTION__, err);
            free(err);
            curl_easy_cleanup(curl);
            return;
        }

        curl_easy_setopt(curl, CURLOPT_URL, audio_url);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            char* err = (char*)malloc(256);
            sprintf(err, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            logError(__FUNCTION__, err);
            free(err);
        }

        fclose(fp);
        curl_easy_cleanup(curl);
    }
}

void generate_audio(const char* message) {
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if(curl) {
    // Set the HTTP headers and options
    struct curl_slist* headers = NULL;
    char auth[256];
    sprintf(auth, "Authorization: Bearer %s", api_key);
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, auth);

    // Set up the request data
    char *data = (char*)malloc(1000*56);
    memset(data, 0, 1000*56);
    sprintf(data, "{\"model\": \"tts-1-hd\", \"response_format\": \"pcm\", \"input\": \"%s\", \"voice\": \"alloy\"}", message);

    // Set up libcurl options
    curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/audio/speech");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

    // Set the file to save the audio output
    FILE *fp = fopen("audio_output.pcm", "wb");
    if(fp) {
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

        // Perform the request
        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            char* err = (char*)malloc(256);
            sprintf(err, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            logError(__FUNCTION__, err);
            free(err);
        } else {
            fprintf(debugLog, "Audio saved as 'audio_output.mp3'\n");
        }

        fclose(fp);
    } else {
        logError(__FUNCTION__, "Failed to open file for writing");
    }

    // Clean up
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    free(data);
    }

    // Load the audio into memory and play it.
    FILE *fp = fopen("audio_output.pcm", "rb");

    // Get size.
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Allocate Memory
    u8 *buffer = (u8*)malloc(size);
    fread(buffer, 1, size, fp);
    fclose(fp);

    // Play it.
    playTTS(buffer, size, 2);
}

// Callback function to handle the response data
size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t real_size = size * nmemb;
    struct ResponseData* response_data = (struct ResponseData*)userp;

    char* temp = (char*)realloc(response_data->data, response_data->size + real_size + 1);
    if (temp == NULL) {
        logError(__FUNCTION__, "Error reallocating memory for response data");
        return 0;
    }

    response_data->data = temp;
    memcpy(&(response_data->data[response_data->size]), contents, real_size);
    response_data->size += real_size;
    response_data->data[response_data->size] = '\0';

    return real_size;
}

// Function to make the API request and handle the response
char* ask_chat_model(const char* question) {
    // Log to debugger.
    if (debugLog != NULL) {
        time_t current_time = time(NULL); // Get current time
        char* time_str = ctime(&current_time); // Convert time to string
        time_str[strlen(time_str) - 1] = '\0'; // Remove newline character from time string
        fprintf(debugLog, "%s: called at %s\n", __FUNCTION__, time_str);
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        logError(__FUNCTION__, "Error initializing cURL");
        return NULL;
    }

    // Construct the request payload
    size_t payload_size = strlen(payload_template) + strlen(text_model) + strlen(question) + 1;
    char* payload = (char*)malloc(payload_size);
    if (!payload) {
        logError(__FUNCTION__, "Error allocating memory for payload");
        curl_easy_cleanup(curl);
        return NULL;
    }
    snprintf(payload, payload_size, payload_template, text_model, question);

    // Set the HTTP headers and options
    struct curl_slist* headers = NULL;
    char auth[256];
    sprintf(auth, "Authorization: Bearer %s", api_key);
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, auth);

    curl_easy_setopt(curl, CURLOPT_URL, endpoint);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);

    // Set up response data struct
    struct ResponseData response_data;
    response_data.data = (char*)malloc(1);
    response_data.size = 0;

    // Set the callback function to handle the response data
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

    // Perform the HTTP request
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        char* err = (char*)malloc(256);
        sprintf(err, "curl_easy_perform() failed: %s", curl_easy_strerror(res));
        logError(__FUNCTION__, err);
        free(err);
    }

    // Save the response data into a file
    FILE* fp = fopen("response.json", "wb");
    if (fp) {
        fwrite(response_data.data, response_data.size, 1, fp);
        fclose(fp);
    } else {
        logError(__FUNCTION__, "Error saving response data to file\n");
    }

    // Clean up resources
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free(payload);

    // Null-terminate the response data
    response_data.data = (char*)realloc(response_data.data, response_data.size + 1);
    response_data.data[response_data.size] = '\0';

    // save response data into a debug file
    //FILE* fp = fopen("sd:/resp.json", "wb");
    //fwrite(response_data.data, response_data.size, 1, fp);

    // test libcJSON
    cJSON *json = cJSON_Parse(response_data.data);

    cJSON* error = cJSON_GetObjectItemCaseSensitive(json, "error");
    if (error) {
        cJSON* message = cJSON_GetObjectItemCaseSensitive(error, "message");
        char* errorData = (char*)malloc(128);
        if (message) {
            sprintf(errorData, "Error message: %s", message->valuestring);
        }
        for(int index=0; index<strlen(errorData); index++)
        {
            errorData[index] = errorData[index+1];
        }
        errorData[strlen(errorData)-1] = '\0';
        logError(__FUNCTION__, errorData);
        return errorData;
    }

    char *choices = cJSON_Print(cJSON_GetArrayItem(cJSON_GetArrayItem(json, 4), 0));
    fwrite(choices, strlen(choices), 1, fp);
    json = cJSON_Parse(choices);
    char *message = cJSON_Print(cJSON_GetArrayItem(json, 1));
    json = cJSON_Parse(message);
    char* content = cJSON_Print(cJSON_GetArrayItem(json, 1));

    for(int index=0; index<strlen(content); index++)
    {
        content[index] = content[index+1];
    }
    content[strlen(content)-1] = '\0';

    fclose(fp);
    //samMain(content);
    // alloc memory for that and 5 new lines
    //char* newContent = (char*)malloc(sizeof(char)*strlen((char*)content)+5);
    //memset(newContent, 0, sizeof(char)*strlen((char*)content)+5);
    //strcpy(newContent, content); // copy the content
    // add 5 new lines
    //strcat(newContent, "\n\n\n\n\n"); // concatenate 5 new lines (append)
    generate_audio(content);
    return content;
}

size_t write_callback_image(void* contents, size_t size, size_t nmemb, char* output_buffer) {
    size_t total_size = size * nmemb;
    strncat(output_buffer, (const char*)contents, total_size);
    fprintf(stderr, "%s", (const char*)contents); // Print received data for debugging
    return total_size;
}

void save_image(const char* image_url, const char* filename) {
    // Log to debugger.
    if (debugLog != NULL) {
        time_t current_time = time(NULL); // Get current time
        char* time_str = ctime(&current_time); // Convert time to string
        time_str[strlen(time_str) - 1] = '\0'; // Remove newline character from time string
        fprintf(debugLog, "%s: called at %s\n", __FUNCTION__, time_str);
        fprintf(debugLog, "%s: URL: %s\n", __FUNCTION__, image_url);
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        logError(__FUNCTION__, "Error initializing cURL");
        return;
    }

    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        char* err = (char*)malloc(256);
        sprintf(err, "Error opening file for writing: %s\n", filename);
        logError(__FUNCTION__, err);
        free(err);

        curl_easy_cleanup(curl);
        return;
    }

    curl_easy_setopt(curl, CURLOPT_URL, image_url);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        char* err = (char*)malloc(256);
        sprintf(err, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        free(err);
        logError(__FUNCTION__, err);
    }

    fclose(fp);
    curl_easy_cleanup(curl);
}

void generate_image(const char* prompt, int width, int height) {
    // Log to debugger.
    if (debugLog != NULL) {
        time_t current_time = time(NULL); // Get current time
        char* time_str = ctime(&current_time); // Convert time to string
        time_str[strlen(time_str) - 1] = '\0'; // Remove newline character from time string
        fprintf(debugLog, "%s: called at %s\n", __FUNCTION__, time_str);
    }

    fprintf(debugLog, "Generating image!\n");
    CURL* curl = curl_easy_init();
    if (!curl) {
        logError(__FUNCTION__, "Error initializing cURL");
        return;
    }

    // Set up the API request headers and parameters
    struct curl_slist* headers = NULL;
    char auth[256];
    sprintf(auth, "Authorization: Bearer %s", api_key);
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, auth);

    curl_easy_setopt(curl, CURLOPT_URL, image_endpoint);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Set up the request data (in JSON format)
    size_t request_size = strlen(request_template) + strlen(prompt) + 2 + 2; // for %dx%d
    char* request_data = static_cast<char*>(malloc(request_size + 1));

    // Ask user what model they want.
    int choice = WindowPromptSmall("Model?", "Do you want to try DALL-E-3?", "Yes", "No");
    char* image_model = "dall-e-2";
    width = 256;
    height = 256;
    if(choice == 1)
    {
        width = 1024;
        height = 1024;
        image_model = "dall-e-3";
    }
    sprintf(request_data, request_template, prompt, image_model, width, height);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_data);

    // Prepare the buffer to hold the response
    const size_t output_size = 1024 * 8192; // Adjust this based on the expected response size
    char* output_buffer = (char*)(malloc(output_size));
    // clean it up.
    memset(output_buffer, 0, output_size);
    //output_buffer[0] = '\0';

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback_image);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, output_buffer);

    // Perform the HTTP request
    fprintf(debugLog, "Connecting...\n");
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        char* err = (char*)malloc(256);
        sprintf(err, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        free(err);
        logError(__FUNCTION__, err);
    } else {
        fprintf(debugLog, "Downloaded!\n");
    }

    // Print out the content of the output buffer for debugging
    fprintf(debugLog, "Output Buffer Content:\n%s\n", output_buffer);

    // Clean up resources
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free(request_data);

    // url finder
    cJSON *json = cJSON_Parse(output_buffer);

    cJSON* error = cJSON_GetObjectItemCaseSensitive(json, "error");
    if (error) {
        cJSON* message = cJSON_GetObjectItemCaseSensitive(error, "message");
        char* errorData = (char*)malloc(128);
        if (message) {
            sprintf(errorData, "Error message: %s", message->valuestring);
        }
        for(int index=0; index<strlen(errorData); index++)
        {
            errorData[index] = errorData[index+1];
        }
        errorData[strlen(errorData)-1] = '\0';
        WindowPromptSmall("Error", errorData, "Ok", NULL);
        logError(__FUNCTION__, errorData);
    }

    char* data = cJSON_Print(cJSON_GetArrayItem(cJSON_GetObjectItem(json, "data"), 0));
    json = cJSON_Parse(data);
    const cJSON* urlItem = cJSON_GetObjectItem(json, "url");
    if (urlItem && cJSON_IsString(urlItem)) {
        const char* url = urlItem->valuestring;
        printf("URL: %s\n", url);
        // Save the image as a PNG file
        printf("Saving image, Please wait...\n");
        save_image(url, "generated_image.png");
    } else {
        printf("URL not found in JSON data.\n");
    }


    // Don't forget to free the output_buffer when you are done using it
    free(output_buffer);
}

UpdateData checkUpdates()
{
    // Log to debugger.
    if (debugLog != NULL) {
        time_t current_time = time(NULL); // Get current time
        char* time_str = ctime(&current_time); // Convert time to string
        time_str[strlen(time_str) - 1] = '\0'; // Remove newline character from time string
        fprintf(debugLog, "%s: called at %s\n", __FUNCTION__, time_str);
    }

    printf("Checking for Updates...\n");
    CURL* curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Error initializing curl\n");
        UpdateData errorData;
        errorData.update_available = false;
        errorData.version = "Error with CURL";
        errorData.relNotes = "Error with CURL";
        return errorData;
    }

    // Set up the API request headers and parameters
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, updateCheck_endpoint);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Prepare the buffer to hold the response
    const size_t output_size = 1024 * 1024; // Adjust this based on the expected response size
    char* output_buffer = static_cast<char*>(malloc(output_size));
    output_buffer[0] = '\0';

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback_image);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, output_buffer);

    // Perform the HTTP request
    printf("Connecting...\n");
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        char* err = (char*)malloc(256);
        sprintf(err, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        free(err);
        logError(__FUNCTION__, err);
    } else {
        printf("Downloaded!\n");
    }

    // Clean up resources
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    // checks
    cJSON *json = cJSON_Parse(output_buffer);
    char* ver = cJSON_Print(cJSON_GetArrayItem(json, 0));
    char* relNotes = cJSON_Print(cJSON_GetArrayItem(json, 1));
    api_key = cJSON_Print(cJSON_GetArrayItem(json, 2));

    // remove the quoute at the start and end of the string
    for(int index=0; index<strlen(ver); index++)
    {
        ver[index] = ver[index+1];
    }
    ver[strlen(ver)-1] = '\0';

    for(int index=0; index<strlen(relNotes); index++)
    {
        relNotes[index] = relNotes[index+1];
    }
    relNotes[strlen(relNotes)-1] = '\0';

    for(int index=0; index<strlen(api_key); index++)
    {
        api_key[index] = api_key[index+1];
    }
    api_key[strlen(api_key)-1] = '\0';

    // compare versions
    UpdateData data;
    if(strcmp(ver, VERSION) == 0)
    {
        data.update_available = false;
    }
    else
    {
        data.update_available = true;
    }
    data.version = ver;
    data.relNotes = relNotes;

    // New Update UI.
    if(data.update_available)
    {
        canHazUpdate = true;
    }
    latestVersion = ver;

    return data;
}