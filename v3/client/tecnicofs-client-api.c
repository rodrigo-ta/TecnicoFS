#include "tecnicofs-client-api.h"

char * clientsocketpath = "../temp/clientsocket";

void send_message(char * sbuffer){
  if(sendto(sockfd, sbuffer, strlen(sbuffer), 0, (struct sockaddr *)&server_addr, server_len) < 0){
    fprintf(stderr, "tecnicofs-client: error sending message to the server\n");
    exit(EXIT_FAILURE);
  }
}

int receive_message(){
  char rbuffer[MAX_INPUT_SIZE];
  int nread, result;
  if((nread = recvfrom(sockfd, rbuffer, MAX_INPUT_SIZE, 0, 0, 0)) < 0){
    fprintf(stderr, "tecnicofs-client: error receiving message from the server\n");
    exit(EXIT_FAILURE);
  }
  rbuffer[nread] = '\0';
  sscanf(rbuffer, "%d", &result);
  return result;
}

int tfsCreate(char *filename, char nodeType) {
  char sbuffer[MAX_INPUT_SIZE];
  int result = FAIL;
  sprintf(sbuffer, "%c %s %c", 'c', filename, nodeType);

  send_message(sbuffer);
  result = receive_message();
  return result;
}

int tfsDelete(char *path) {
  char sbuffer[MAX_INPUT_SIZE];
  int result = FAIL;
  sprintf(sbuffer, "%c %s", 'd', path);

  send_message(sbuffer);
  result = receive_message();
  return result;
}

int tfsMove(char *from, char *to) {
  char sbuffer[MAX_INPUT_SIZE];
  int result = FAIL;
  sprintf(sbuffer, "%c %s %s", 'm', from, to);

  send_message(sbuffer);
  result = receive_message();
  return result;
}

int tfsLookup(char *path) {
  char sbuffer[MAX_INPUT_SIZE];
  int result = FAIL;
  sprintf(sbuffer, "%c %s", 'l', path);

  send_message(sbuffer);
  result = receive_message();
  return result;
}

int tfsMount(char * sockPath) {
  /* create socket */
  if((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0){
    fprintf(stderr, "tecnicofs-client: can't open socket\n");
    return FAIL;
  }

  /* just to prevent cases where last application exit with error, without unlinking socket */
  unlink(clientsocketpath);

  /* set client socket address */
  client_len = set_socket_address(clientsocketpath, &client_addr);

  /* bind client socket */
  if(bind(sockfd, (struct sockaddr *) &client_addr, client_len) < 0){
    fprintf(stderr, "tecnicofs-client: bind error\n");
    return FAIL;
  }

  /* set server socket address */
  server_len = set_socket_address(sockPath, &server_addr);

  return SUCCESS;
}

int tfsUnmount() {
  if(unlink(clientsocketpath) != 0){
    fprintf(stderr, "tecnicofs-client: error unlinking client socket path\n");
    return FAIL;
  }
  
  if(close(sockfd) != 0){
    fprintf(stderr, "tecnicofs-client: error closing socket\n");
    return FAIL;
  }

  return SUCCESS;
}

int set_socket_address(char *path, struct sockaddr_un *addr) {
  if (addr == NULL)
    return 0;

  bzero((char *)addr, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, path);

  return SUN_LEN(addr);
}
