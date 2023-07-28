#include "download.h"
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

// Define your OpenAI API endpoint and key
const char* endpoint = "https://api.openai.com/v1/chat/completions";
const char* api_key = "<YOUR API KEY HERE>";

// Struct to hold the response data
struct ResponseData {
    char* data;
    size_t size;
};

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

// update function
#define wiigptii_dol_path "sd:/apps/wiigptii/boot.dol"
#define wiigptii_meta_path "sd:/apps/wiigptii/meta.xml"
#define wiigptii_icon_path "sd:/apps/wiigptii/icon.png"

// As rewriting the download commands for curl can get quite lengthy, here is a function.
CURLcode curl_download(char *url, FILE *fp) {
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

// Callback function to handle the response data
size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t real_size = size * nmemb;
    struct ResponseData* response_data = (struct ResponseData*)userp;

    char* temp = (char*)realloc(response_data->data, response_data->size + real_size + 1);
    if (temp == NULL) {
        fprintf(stderr, "Error reallocating memory for response data\n");
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
    CURL* curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Error initializing curl\n");
        return NULL;
    }

    // Construct the request payload
    const char* payload_template = "{\"model\":\"gpt-3.5-turbo\", \"messages\": [{\"role\": \"system\", \"content\": \"You are a helpful assistant.\\n.\"}, {\"role\": \"user\", \"content\": \"%s\"}]}";
    size_t payload_size = strlen(payload_template) + strlen(question) + 1;
    char* payload = (char*)malloc(payload_size);
    if (!payload) {
        fprintf(stderr, "Error allocating memory for payload\n");
        curl_easy_cleanup(curl);
        return NULL;
    }
    snprintf(payload, payload_size, payload_template, question);

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
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }

    // Clean up resources
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free(payload);

    // Null-terminate the response data
    response_data.data = (char*)realloc(response_data.data, response_data.size + 1);
    response_data.data[response_data.size] = '\0';

    // save response data into a debug file
    FILE* fp = fopen("sd:/resp.json", "wb");
    //fwrite(response_data.data, response_data.size, 1, fp);

    // test libcJSON
    cJSON *json = cJSON_Parse(response_data.data);
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
    
    return content;
}

size_t write_callback_image(void* contents, size_t size, size_t nmemb, char* output_buffer) {
    size_t total_size = size * nmemb;
    memcpy(output_buffer, contents, total_size);
    return total_size;
}

void save_image(const char* image_url, const char* filename) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Error initializing curl\n");
        return;
    }

    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Error opening file for writing: %s\n", filename);
        curl_easy_cleanup(curl);
        return;
    }

    curl_easy_setopt(curl, CURLOPT_URL, image_url);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }

    fclose(fp);
    curl_easy_cleanup(curl);
}

void generate_image(const char* prompt, int width, int height) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Error initializing curl\n");
        return;
    }

    // Set up the API request headers and parameters
    struct curl_slist* headers = NULL;
    char auth[256];
    sprintf(auth, "Authorization: Bearer %s", api_key);
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, auth);

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/images/generations");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Set up the request data (in JSON format)
    const char* request_template = "{\"prompt\": \"%s\", \"size\": \"%dx%d\"}";
    size_t request_size = strlen(request_template) + strlen(prompt) + 2 + 2; // for %dx%d
    char* request_data = static_cast<char*>(malloc(request_size + 1));
    sprintf(request_data, request_template, prompt, width, height);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_data);

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
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    } else {
        printf("Downloaded!\n");
    }

    // Clean up resources
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free(request_data);

    // url finder
    cJSON *json = cJSON_Parse(output_buffer);
    char* data = cJSON_Print(cJSON_GetArrayItem(cJSON_GetArrayItem(json, 1), 0));
    json = cJSON_Parse(data);
    char* url = cJSON_Print(cJSON_GetArrayItem(json, 0));
    printf(url);
    for(int index=0; index<strlen(url); index++)
    {
        url[index] = url[index+1];
    }
    url[strlen(url)-1] = '\0';
    // Save the image as a PNG file
    printf("Saving image, Please wait...\n");
    save_image(url, "generated_image.png");

    // Don't forget to free the output_buffer when you are done using it
    free(output_buffer);
}

#define VERSION "1.0.3"

UpdateData checkUpdates()
{
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

    curl_easy_setopt(curl, CURLOPT_URL, "https://dakotath.com/wiigptii/checkUpdate.php");
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
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
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

    return data;
}