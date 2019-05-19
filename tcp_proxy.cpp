// This is TCP proxy server source snippet.
// Based on http://www.martinbroadhurst.com/tcp-proxy-for-windows.html
// add support for multiple connection.

#include "stdafx.h"

#ifndif WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <process.h>

#define BACKLOG 10
#define BUF_SIZE 4096

unsigned int transfer(SOCKET from, SOCKET to)
{
    char buf[BUF_SIZE];
    unsigned int disconnected = 0;
    size_t bytes_read, bytes_written;
    bytes_read = recv(from, buf, BUF_SIZE, 0);
    if (bytes_read == 0)
    {
        disconnected = 1;
    }
    else
    {
        bytes_written = send(to, buf, bytes_read, 0);
        if (bytes_written == -1)
        {
            disconnected = 1;
        }
    }
    return disconnected;
}

void handle(SOCKET client, const char *remote_host, const char *remote_port)
{
    struct addrinfo hints, *res;
    SOCKET server = -1;
    unsigned int disconnected = 0;
    fd_set set;
    unsigned int max_sock;

    ZeroMemory(&hints, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(remote_host, remote_port, &hints, &res) != 0)
    {
        perror("getaddrinfo");
        closesocket(client);
    }

    server = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (server == INVALID_SOCKET)
    {
        perror("socket");
        closesocket(client);
    }

    if (connect(server, res->ai_addr, res->aiaddrlen) == -1)
    {
        perror("connect");
        closesocket(client);
        return;
    }

    if (client > server)
    {
        max_sock = client;
    }
    else
    {
        max_sock = server;
    }

    while (!disconnected)
    {
        FD_ZERO(&set);
        FD_SET(client, &set);
        FD_SET(server, &set);
        if (select(max_sock + 1, &set, NULL, NULL, NULL) == SOCKET_ERROR)
        {
            perror("select");
            break;
        }
        if (FD_ISSET(client, &set))
        {
            disconnected = transfer(client, server);
        }
        if (FD_ISSET(server, &set))
        {
            disconnected = transfer(server, client);
        }
    }
    if (server != -1)
    {
        closesocket(server);
    }
    closesocket(client);
}

typedef struct client {
    int client_socket;
    char* remote_host;
    char* remote_port;
} *pclient;

unsigned __stdcal Handle(void *data)
{
    handle(((struct client*)data)->client_socket, ((struct client*)data)->remote_host, ((struct cient*)data)->remote_port);
    return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
    WORD wVersion = MAKEWORD(2, 2);
    WSADATA wsaData;
    int iResult;
    SOCKET sock;
    struct addrinfo hints, *res;
    int reuseaddr = 1;
    //const char *local_host, *local_port, *remote_host, *remote_port;
    char *local_host, *local_port, *remote_host, *remote_port;

    if (iResult = (WSAStartup(wVersion, &wsaData)) != 0)
    {
        fprintf(stderr, "WSAStartup failed: %d\n", iResult);
        return 1;
    }

    if (argc < 5)
    {
        fprintf(stderr, "Usage: tcp_proxy.exe local_host local_port remote_host remote_port \n");
        return 1;
    }

    //local_host = argv[1];
    local_host = (char*)malloc(wcslen(argv[1]) + 1);
    wcstombs(local_host, argv[1], wcslen(argv[1]) + 1);
    //local_port = argv[2];
    local_port = (char*)malloc(wcslen(argv[2]) + 1);
    wcstombs(local_port, argv[2], wcslen(argv[2]) + 1);
    //remote_host = argv[3];
    remote_host = (char*)malloc(wcslen(argv[3]) + 1);
    wcstombs(remote_host, argv[3], wcslen(argv[3]) + 1);
    //remote_port = argv[4];
    remote_port = (char*)malloc(wcslen(argv[4]) + 1);
    wcstombs(remote_port, argv[4], wcslen(argv[4]) + 1);

    ZeroMemory(&hints, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(local_host, local_port, &hints, &res) != 0)
    {
        perror("getaddrinfo");
        WSACleanup();
        return 1;
    }

    sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock == INVALID_SOCKET)
    {
        perror("socket");
        WSACleanup();
        return 1;
    }

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuseaddr, sizeof(int)) == SOCKET_ERROR)
    {
        perror("setsockopt");
        WSACleanup();
        return 1;
    }

    if (bind(sock, res->ai_addr, res->ai_addrlen) == SOCKET_ERROR)
    {
        perror("bind");
        WSACleanup();
        return 1;
    }

    if (listen(sock, BACKLOG) == SOCKET_ERROR)
    {
        perror("listen");
        WSACleanup();
        return 1;
    }

    freeaddrinfo(res);

    while (1)
    {
        size_t size = sizeof(struct sockaddr_in);
        struct sockaddr_in their_addr;
        int newsock = accept(sock, (struct sockaddr*)&their_addr, (int *)&size);
        if (newsock == INVALID_SOCKET)
        {
            perror("accept");
        }
        else
        {
            printf("Got a connection from %s on port %d\n", inet_ntoa(their_addr.sin_addr), htons(their_addr.sin_port));
            //handle(newsock, remote_host, remote_port);
            unsigned threadID;
            struct client* data;
            data = (struct client *)malloc(sizeof(struct client));
            data->client_socket = newsock;
            data->remote_host = remote_host;
            data->remote_port = remote_port;
            HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, &Handle, (void *)data, 0, &threadID);
        }
    }

    closesocket(sock);
    WSACleanup();

    return 0;
}
