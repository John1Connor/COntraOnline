#include "network.h"
#include <stdio.h>
#include <process.h>
#include <string.h>

void network_init()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
}

void network_cleanup()
{
    WSACleanup();
}

unsigned int __stdcall client_handler(void *data)
{
    SOCKET clientSocket = (SOCKET)data;
    char buffer[BUFFER_SIZE];

    while (1)
    {
        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived <= 0)
        {
            closesocket(clientSocket);
            return 0;
        }

        buffer[bytesReceived] = '\0';
        printf("Mensaje recibido: %s\n", buffer);
    }
    return 0;
}

void broadcast_message(NetworkManager *nm, const char *message)
{
    for (int i = 0; i < nm->playerCount; i++)
    {
        send(nm->clients[i], message, strlen(message), 0);
    }
}

int server_start(NetworkManager *nm)
{
    nm->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (nm->socket == INVALID_SOCKET)
        return -1;

    nm->address.sin_family = AF_INET;
    nm->address.sin_addr.s_addr = INADDR_ANY;
    nm->address.sin_port = htons(PORT);

    if (bind(nm->socket, (struct sockaddr *)&nm->address, sizeof(nm->address)) == SOCKET_ERROR)
    {
        closesocket(nm->socket);
        return -1;
    }

    if (listen(nm->socket, SOMAXCONN) == SOCKET_ERROR)
    {
        closesocket(nm->socket);
        return -1;
    }

    nm->isRunning = 1;
    nm->playerCount = 0;
    return 0;
}

unsigned int __stdcall server_accept_clients(void *data)
{
    NetworkManager *nm = (NetworkManager *)data;
    while (nm->isRunning && nm->playerCount < MAX_PLAYERS)
    {
        SOCKET clientSocket = accept(nm->socket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET)
            continue;

        nm->clients[nm->playerCount++] = clientSocket;
        char buffer[BUFFER_SIZE];
        sprintf(buffer, "PLAYERS:%d", nm->playerCount);
        broadcast_message(nm, buffer);

        _beginthreadex(NULL, 0, client_handler, (void *)clientSocket, 0, NULL);
    }
    return 0;
}

int client_connect(NetworkManager *nm, const char *ip)
{
    nm->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (nm->socket == INVALID_SOCKET)
        return -1;

    nm->address.sin_family = AF_INET;
    nm->address.sin_addr.s_addr = inet_addr(ip);
    nm->address.sin_port = htons(PORT);

    if (connect(nm->socket, (struct sockaddr *)&nm->address, sizeof(nm->address)) == SOCKET_ERROR)
    {
        closesocket(nm->socket);
        return -1;
    }

    // Notificar al servidor que el cliente está listo
    send(nm->socket, "CLIENT_READY", 12, 0);
    return 0;
}