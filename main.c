#include <arpa/inet.h>
#include <bits/types/struct_iovec.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// Constants
#define PORT 8080
#define IP "127.0.0.1"
#define REQUEST_BUFFER_SIZE 64 * 1024 // hugeaass file

enum http_methods {
  GET,
  POST,
  PUT,
  DELETE,
  PATCH,
  OPTIONS,
  HEAD,
  TRACE,
  CONNECT
};

typedef struct {
  char name[256];
  int id;
} todo;

// stack alllocation for arra
todo *todos[64];

// MEDIUM LEVEL
void generate_todos_html(char *buf) {
  char *template = "<div id='todo%d' class='todo'>%s</div>\n";

  for (int i = 0; i < sizeof(todos) / sizeof(todos[0]); i++) {
    if (todos[i] == NULL)
      continue;
    sprintf(buf, template, todos[i]->id, todos[i]->name);
  }
  // free(template);
}

void write_response_headers(uint status, uint content_length, int fd) {
  // TODO: map status code
  char answer_headers[512];
  char *template = "HTTP/1.1 %d OK\r\n"
                   "Content-Length: %d\r\n"
                   "Content-Type: text/html\r\n"
                   "Server: Ródŕîgô'ŝ himself\r\n\r\n";

  sprintf(answer_headers, template, status, content_length);
  write(fd, answer_headers, strlen(answer_headers));
}

// HIGH LEVEL - ROUTES
void get_root_page(int req_file_desc) {
  // after writing headers, write page
  char *template =
      "<html>\n"
      "<meta>\n"
      "<title>This is a todo app</title>\n"
      "</meta>\n"
      "<body>\n"
      "<h1>This is a todo app</h1>\n"
      "<div name='todos'>"
      "%s" // TODO: todos go here
      "</div>"
      "<form action=\"/create\" method=\"POST\">\n"
      "  <input type=\"text\" name=\"todo\" placeholder=\"Enter your todo\">\n"
      "  <input type=\"submit\" value=\"Create\">\n"
      "</form>\n"
      "</html>\n";
  // Generate todos for tempalteing
  char *todos_string = malloc(REQUEST_BUFFER_SIZE);
  generate_todos_html(todos_string);

  // template the todos into res
  char *res = malloc(strlen(template));
  sprintf(res, template, todos_string);
  free(todos_string);

  // send it all to clietn
  write_response_headers(200, strlen(res), req_file_desc);
  write(req_file_desc, res, strlen(res));
  free(res);
  printf("root called!\n");
}
void post_create_todo(int req_file_desc) {
  write_response_headers(404, 0, req_file_desc);
  printf("Create called!\n");
}
void post_delete_todo(int req_file_desc) {
  write_response_headers(404, 0, req_file_desc);
  printf("delete called!\n");
}

void process_request(int req_file_desc) {
  char *buf = malloc(REQUEST_BUFFER_SIZE);
  read(req_file_desc, buf, REQUEST_BUFFER_SIZE);
  printf("%s\n", buf); // TODO: mmaybe turn this off

  // Map our request to other functions
  enum http_methods method;
  if (strncmp(buf, "GET", 3) == 0) {
    method = GET;
  } else if (strncmp(buf, "POST", 4) == 0) {
    method = POST;
  } else {
    write_response_headers(404, 0, req_file_desc);
    close(req_file_desc);
    return;
  }

  // getting the path
  char *path = malloc(256);
  int path_start = -1;
  int path_end = -1;
  for (int i = 0; i < REQUEST_BUFFER_SIZE; i++) {
    if (path_start == -1 && buf[i] == '/') {
      path_start = i;
    } else if (path_start != -1 && buf[i] == ' ') {
      path_end = i;
      break;
    } else if (buf[i] == '\0') {
      // break at null chars for now
      break;
    }
  }
  memcpy(path, buf + path_start, path_end - path_start);
  path[path_end - path_start] = '\0'; // Add null terminator to end of path

  if (method == GET && 0 == strcmp("/", path))
    get_root_page(req_file_desc);
  else if (method == POST && 0 == strcmp("/create", path))
    post_create_todo(req_file_desc);
  else if (method == POST && 0 == strcmp("/destroy", path))
    post_delete_todo(req_file_desc);
  else
    write_response_headers(404, 0, req_file_desc);

  // Free allocated memory after request
  free(path);
  free(buf);

  // Close fd after processing
  close(req_file_desc);
}

void handle_sigint(int fd) {
  shutdown(fd, SHUT_RDWR);
  exit(0);
}

// LOW LEVEL
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
    close(sockfd);
    return -1;
  }

  if (listen(sockfd, 8) < 0) {
    perror("Shit has happened at socket listeining");
  }

  printf("Listening on IP %s and port %d\n", ip, port);

  // Return file descriptor
  return sockfd;
}

int main() {
  todos[0]->id = 1;
  strcpy(todos[0]->name, "This is a testing");
  // init
  int sock_desc = create_listen_socket(IP, PORT);

  // Process C-c and the like so that socket is not hanging
  signal(SIGINT, (void (*)(int))handle_sigint);

  // loop setup
  int req_file_desc;
  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);

  // main server loop
  while (1) {
    req_file_desc =
        accept(sock_desc, (struct sockaddr *)&client_addr, &client_len);
    if (req_file_desc < 0) {
      perror("Error accepting socket connection");
      continue;
    }

    process_request(req_file_desc);
  }

  return 0;
}
