#include "tecnicofs-client-api.h"

/**
 * Send message to server
 * Input:
 *  - sbuffer: buffer with the message
 */
void send_message(char * sbuffer){
  if(sendto(sockfd, sbuffer, strlen(sbuffer), 0, (struct sockaddr *)&server_addr, server_len) < 0){
    fprintf(stderr, "tecnicofs-client: error sending message to the server\n");
    exit(EXIT_FAILURE);
  }
}

/**
 * Receive message from server and gets result value form performed operation
 * Returns:
 *  - value of the operation (FAIL or SUCCESS)
 */
int receive_message(){
  char rbuffer[MAX_INPUT_SIZE];
  int nread, result = FAIL;
  if((nread = recvfrom(sockfd, rbuffer, MAX_INPUT_SIZE, 0, 0, 0)) < 0){
    fprintf(stderr, "tecnicofs-client: error receiving message from the server\n");
    exit(EXIT_FAILURE);
  }
  rbuffer[nread] = '\0';
  sscanf(rbuffer, "%d", &result);
  return result;
}

/**
 * Requests create operation
 * Input:
 *  - filename: name of the file to be created
 *  - nodeType: type of file
 * Returns:
 *  - value of the operation (FAIL or SUCCESS)
 */
int tfsCreate(char *filename, char nodeType) {
  char sbuffer[MAX_INPUT_SIZE];
  int result = FAIL;
  sprintf(sbuffer, "%c %s %c", 'c', filename, nodeType);

  send_message(sbuffer);
  result = receive_message();
  return result;
}

/**
 * Requests delete operation
 * Input:
 *  - path: is the path to be deleted
 * Returns:
 *  - value of the operation (FAIL or SUCCESS)
 */
int tfsDelete(char *path) {
  char sbuffer[MAX_INPUT_SIZE];
  int result = FAIL;
  sprintf(sbuffer, "%c %s", 'd', path);

  send_message(sbuffer);
  result = receive_message();
  return result;
}

/**
 * Requests move operation
 * Input:
 *  - from: is the source path
 *  - to: is the destination path
 * Returns:
 *  - value of the operation (FAIL or SUCCESS)
 */
int tfsMove(char *from, char *to) {
  char sbuffer[MAX_INPUT_SIZE];
  int result = FAIL;
  sprintf(sbuffer, "%c %s %s", 'm', from, to);

  send_message(sbuffer);
  result = receive_message();
  return result;
}

/**
 * Requests lookup operation
 * Input:
 *  - path: is the path to be looked up
 * Returns:
 *  - value of the operation (FAIL or SUCCESS)
 */
int tfsLookup(char *path) {
  char sbuffer[MAX_INPUT_SIZE];
  int result = FAIL;
  sprintf(sbuffer, "%c %s", 'l', path);

  send_message(sbuffer);
  result = receive_message();
  return result;
}

/**
 * Mounts client and server sockets
 * Input:
 *  - server_socket_path
 *  - client_socket_path
 * Returns
 * - FAIL or SUCCESS
 */
int tfsMount(char * server_socket_path, char * client_socket_path) {
  /* create socket */
  if((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0){
    fprintf(stderr, "tecnicofs-client: can't open socket\n");
    return FAIL;
  }

  /* just to prevent cases where last application exit with error, without unlinking socket */
  unlink(client_socket_path);

  /* set client socket address */
  client_len = set_socket_address(client_socket_path, &client_addr);

  /* bind client socket */
  if(bind(sockfd, (struct sockaddr *) &client_addr, client_len) < 0){
    fprintf(stderr, "tecnicofs-client: bind error\n");
    return FAIL;
  }

  /* set server socket address */
  server_len = set_socket_address(server_socket_path, &server_addr);

  return SUCCESS;
}

/**
 * Unmounts client and server sockets
 * Input:
 *  - server_socket_path
 *  - client_socket_path
 * Returns
 * - FAIL or SUCCESS
 */
int tfsUnmount(char * client_socket_path) {
  if(unlink(client_socket_path) != 0){
    fprintf(stderr, "tecnicofs-client: error unlinking client socket path\n");
    return FAIL;
  }
  
  if(close(sockfd) != 0){
    fprintf(stderr, "tecnicofs-client: error closing socket\n");
    return FAIL;
  }

  return SUCCESS;
}

/**
 * Set socket address of a specified path
 * Inputs:
 *  - path: path of the socket
 *  - addr: address to be set
 * Returns:
 *  - 0 or length of the address
 */
int set_socket_address(char *path, struct sockaddr_un *addr) {
  if (addr == NULL)
    return 0;

  bzero((char *)addr, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, path);

  return SUN_LEN(addr);
}
