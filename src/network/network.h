#ifndef NETWORK_H
#define NETWORK_H

#include <winsock2.h>

#define MAX_PLAYERS 40
#define PORT 12345
#define BUFFER_SIZE 1024

typedef enum
{
    ROLE_NONE,
    ROLE_SERVER,
    ROLE_CLIENT
} NetworkRole;

typedef struct
{
    SOCKET socket;
    struct sockaddr_in address;
    NetworkRole role;
    int isRunning;
    int playerCount;
    SOCKET clients[MAX_PLAYERS]; // Nuevo: Lista de clientes conectados
} NetworkManager;

// Funciones comunes
void network_init();
void network_cleanup();

// Funciones de servidor
int server_start(NetworkManager *nm);
unsigned int __stdcall server_accept_clients(void *data);
void broadcast_message(NetworkManager *nm, const char *message);

// Funciones de cliente
int client_connect(NetworkManager *nm, const char *ip);

#endif