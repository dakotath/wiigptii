#ifdef __cplusplus
extern "C" {
#endif
// ChatGPT client entry point (WiiGPT 2023 for Larsen V)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <fat.h>

// some structs
// struct for a socket connection

// struct for an http request
typedef struct {
    char* host;
    char* path;
    char* method;
    char* body;
    char* headers;
} HttpRequest;

// struct for an http response
typedef struct {
    char* body;
    char* headers;
    int statusCode;
} HttpResponse;

bool init_network();
void networkInit(void);
void networkDeinit(void);
int connectToServer(void);

char* getIp(char* host);
char* getHost(char* ip);

/*
SocketConnection connectServer(char* host, int port);
s32* socket_write(SocketConnection connection, void* buffer, int length);
s32* socket_read(SocketConnection connection, void* buffer, int length);

char* socket_readHTTPHeader(SocketConnection connection);
*/

void sendTestRequest(void);
void sendMessage(char* message);

#ifdef __cplusplus
}
#endif