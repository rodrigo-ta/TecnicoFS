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

void send_message(char*);
int receive_message();

int tfsCreate(char*, char);
int tfsDelete(char*);
int tfsLookup(char*);
int tfsMove(char*, char*);
int tfsMount(char*, char*);
int tfsUnmount(char*);

int set_socket_address(char*, struct sockaddr_un*);

#endif /* CLIENT_H */
