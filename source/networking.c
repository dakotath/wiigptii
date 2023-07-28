// ChatGPT client entry point (WiiGPT 2023 for Larsen V)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <fat.h>
#include <wiisocket.h>

#include "networking.h"

// Definitions
#define SERVER "66.29.142.107"
#define PORT 1139

// Our public network socket
struct hostent *dns;

// Network initialisation function
void networkInit(void) {
    // Initialise the network subsystem
    // taking a whole new approach to this (using if_config instead of net_init)
    //net_init();

    // Initialise the network interface
    //if_config(NULL, NULL, NULL, true, 30);
    wiisocket_init();
}

bool init_network()
{
    bool ok = false;
    return ok;
}

// Network deinitialisation function
void networkDeinit(void) {
    // Deinitialise the network interface
    //if_config(NULL, NULL, NULL, false, 30);
    // Deinitialise the network subsystem
}

// function to get the IP address of a host
char* getIp(char* host) {
    // get the host by name
    dns = net_gethostbyname(host);
    // check if failed
    if(dns == NULL) {
        return NULL;
    }
    // return the IP address
    //return inet_ntoa(*(struct in_addr*)dns->h_addr_list[0]);
    return NULL;
}

// function to get the host name of an IP address
char* getHost(char* ip) {
    // get the host by IP
    //dns = net_gethostbyaddr(ip, strlen(ip), AF_INET);
    // check if failed
    if(dns == NULL) {
        return NULL;
    }
    // return the host name
    //return dns->h_name;
    return NULL;
}

// function to get a key value from an HTTP response header
char* getHttpResponseHeaderValue(char* response, char* header) {
    // get the header value
    char* headerValue = NULL;
    char* headerStart = strstr(response, header);
    if(headerStart != NULL) {
        headerValue = malloc(256);
        sscanf(headerStart, "%*s %s", headerValue);
    }
    // return the header value
    return headerValue;
}

/*
SocketConnection connectServer(char* host, int port) {
    // Initialise the socket
    s32 sock = net_socket(AF_INET, SOCK_STREAM, 0); // create socket
    dns = net_gethostbyname(host); // get host by name
    // check if failed
    if(dns == NULL) {
        return (SocketConnection){-1};
    }
    printf("Socket: %d\n", sock);
    printf("Dedicated Nameservers: %d\n", sizeof(dns->h_addr_list));

    memcpy(&sockAddrIn.sin_addr.s_addr, dns->h_addr, dns->h_length); // copy address
    sockAddrIn.sin_family = AF_INET; // set family
    sockAddrIn.sin_port = htons(port); // set the port

    // connect to the server
    if(net_connect(sock, (struct sockaddr*)&sockAddrIn, sizeof(sockAddrIn)) < 0) {
        return (SocketConnection){-2};
    }

    // return the socket connection
    return (SocketConnection){sock, sockAddrIn};
}

s32* socket_write(SocketConnection connection, void* buffer, int length) {
    // write to the socket
    return net_write(connection.socket, buffer, length);
}
s32* socket_read(SocketConnection connection, void* buffer, int length) {
    // read from the socket
    return net_read(connection.socket, buffer, length);
}
// function to read header until the end of the header
char* socket_readHTTPHeader(SocketConnection connection)
{
    // read the header
    char* header = malloc(8192);
    char* headerEnd = "\r\n\r\n";
    char* headerEndPtr = NULL;
    int bytesRead = 0;
    while ((bytesRead = net_read(connection.socket, header, 8192)) > 0)
    {
        // check if we have reached the end of the header
        headerEndPtr = strstr(header, headerEnd);
        if (headerEndPtr != NULL)
        {
            // we have reached the end of the header
            // calculate the number of bytes after the end of the header
            int remainingBytes = (header - headerEndPtr) + 4;
            if (remainingBytes > 0)
            {
                // move the remaining bytes to the beginning of the buffer
                memmove(header, headerEndPtr + 4, remainingBytes);
            }
            // update the bytesRead variable accordingly
            bytesRead = remainingBytes;
            break;
        }
    }
    // return the header
    return header;
}
*/

// Function to connect to the server
int connectToServer(void) {
    // Initialise the socket

}
