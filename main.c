#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// Constants
#define PORT 8080
#define IP "127.0.0.1"
#define REQUEST_BUFFER_SIZE 4096

int create_listen_socket(char *ip, int port) {
  int sockfd;
  struct sockaddr_in server_addr;

  // Create socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("Error opening socket");
    return -1;
  }

  // Initialize server address structure
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(ip);
  server_addr.sin_port = htons(port);

  // Bind to socket
  if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("Error binding socket");
    // close(sockfd);
    return -1;
  }

  if (listen(sockfd, 3) < 0) {
    perror("Shit has happened at socket listeining");
  }

  printf("Listening on IP %s and port %d\n", ip, port);

  // Return file descriptor
  return sockfd;
}

void write_response_headers(uint status, uint content_length, int fd) {
  // TODO: map status code
  char answer_headers[512];
  char *template = "HTTP/1.1 %d OK\r\n"
                   "Content-Length: %d\r\n"
                   "Content-Type: text/plain\r\n"
                   "Server: Ródŕîgô'ŝ himself\r\n\r\n";

  sprintf(answer_headers, template, status,
          strlen(answer_headers) + content_length);
  write(fd, answer_headers, strlen(answer_headers));
}

void process_request() {}

int main() {
  // init
  int sock_desc = create_listen_socket(IP, PORT);

  // loop setup
  int req_file_desc;
  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);
  char *buf = malloc(REQUEST_BUFFER_SIZE);

  // main server loop
  while (1) {
    req_file_desc =
        accept(sock_desc, (struct sockaddr *)&client_addr, &client_len);
    if (req_file_desc < 0) {
      perror("Error accepting socket connection");
      continue;
    }

    // Get basic information from fd.
    // TODO: truncate data stream so not to log all data
    read(req_file_desc, buf, REQUEST_BUFFER_SIZE);
    printf("%s\n", buf);

    write_response_headers(200, 100, req_file_desc);

    // Close fd ungracefully
    close(req_file_desc);
  }

  return 0;
}
