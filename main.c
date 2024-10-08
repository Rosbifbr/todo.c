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
#define TODO_ARRAY_SIZE 64

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

struct todo {
  char name[256];
  int id;
};
typedef struct todo todo;
todo *todos[TODO_ARRAY_SIZE]; // program memory

// MEDIUM LEVEL
void generate_todos_html(char *buf) {
  char *template = "<form action='/delete' class='todo' method='POST'>"
                   "<input type='hidden' name='id' value='%d'></input>"
                   "<input type='submit' value='Delete'></input>"
                   "<span>    %s</span>"
                   "</form>";

  // TODO: replace by strcat, stop programming like an animal
  char *_buf = buf;
  for (int i = 0; i < sizeof(todos) / sizeof(todos[0]); i++) {
    if (todos[i] == NULL)
      continue;

    sprintf(_buf, template, todos[i]->id, todos[i]->name);
    _buf += strlen(_buf); // move pointer upwards not to overwrite prev text
  }
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
      "<html>"
      "<meta>"
      " <title>This is a todo app</title>"
      "</meta>"
      "<body>"
      "<h1>This is a todo app</h1>"
      "<div name='todos'>"
      "%s"
      "</div>"
      "<form action='/create' method='POST'>"
      "  <input type='text' name='todo' placeholder='Enter your todo'>"
      "  <input type='submit' value='Create'>"
      "</form>"
      "</html>";

  // Generate todos for tempalteing
  char *todos_string = malloc(REQUEST_BUFFER_SIZE);
  generate_todos_html(todos_string);

  // template the todos into res
  char *res = malloc(strlen(template) + strlen(todos_string));
  sprintf(res, template, todos_string);

  // send it all to client
  write_response_headers(200, strlen(res), req_file_desc);
  write(req_file_desc, res, strlen(res));

  // clean up
  free(todos_string);
  free(res);
  printf("root called!\n");
}

char *get_body_offset(char *req_buffer) {
  return strstr(req_buffer, "\r\n\r\n");
}

char *get_post_body_param(char *body, char *param) {
  char *body_key_offset = strstr(body, param);
  if (body_key_offset == NULL)
    return NULL;

  char *value_start = strchr(body_key_offset, '=') + 1;
  char *value_end = strstr(value_start, "\r\n");
  char *value = malloc(value_end - value_start);
  strncpy(value, value_start, value_end - value_start);

  return value;
}

void write_to_todo_array(char *string) {
  for (int i = 0; i < TODO_ARRAY_SIZE; i++) {
    if (todos[i] == NULL) {
      todos[i] = malloc(sizeof(todo));
      strcpy(todos[i]->name, string);
      todos[i]->id = i;
      printf("Wrote TODO to index %d\n", i);
      break;
    }
  }
}

void free_todo_from_array(int key) {
  todo *todo = todos[key];
  if (todo == NULL)
    return;
  todos[key] = NULL;
  free(todo);
}

void post_create_todo(int req_file_desc, char *req_buffer) {
  // get body offset
  char *body = get_body_offset(req_buffer);

  // Get todo param value
  char *todo_text = get_post_body_param(body, "todo");

  // Write to todo array
  write_to_todo_array(todo_text);

  // Redirect to root using proper HTTP headers
  char *redirect_response = "HTTP/1.1 302 Found\r\n"
                            "Location: /\r\n"
                            "Content-Length: 0\r\n"
                            "\r\n";
  write(req_file_desc, redirect_response, strlen(redirect_response));
  printf("Create called!\n");
}

void post_delete_todo(int req_file_desc, char *req_buffer) {
  // Redirect to root using proper HTTP headers
  char *redirect_response = "HTTP/1.1 302 Found\r\n"
                            "Location: /\r\n"
                            "Content-Length: 0\r\n"
                            "\r\n";
  write(req_file_desc, redirect_response, strlen(redirect_response));
  printf("Delete called!\n");
}

void process_request(int req_file_desc) {
  char *buf = malloc(REQUEST_BUFFER_SIZE);
  read(req_file_desc, buf, REQUEST_BUFFER_SIZE);
  printf("%s\n", buf);

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
    } else if (path_start != -1 && (buf[i] == ' ' || buf[i] == '?')) {
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
    post_create_todo(req_file_desc, buf);
  else if (method == POST && 0 == strcmp("/delete", path))
    post_delete_todo(req_file_desc, buf);
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
  // init
  int sock_desc = create_listen_socket(IP, PORT);
  todos[0] = malloc(sizeof(todo));
  todos[0]->id = 1;
  strcpy(todos[0]->name, "This is a testing");

  todos[1] = malloc(sizeof(todo));
  todos[1]->id = 2;
  strcpy(todos[1]->name, "This is the second todo");

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
