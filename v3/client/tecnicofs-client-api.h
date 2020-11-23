#ifndef API_H
#define API_H

#include "../tecnicofs-api-constants.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/stat.h>

int sockfd;
socklen_t server_len, client_len;
struct sockaddr_un server_addr, client_addr;

int tfsCreate(char *path, char nodeType);
int tfsDelete(char *path);
int tfsLookup(char *path);
int tfsMove(char *from, char *to);
int tfsMount(char* serverName);
int tfsUnmount();

int set_socket_address(char*, struct sockaddr_un*);

#endif /* CLIENT_H */
