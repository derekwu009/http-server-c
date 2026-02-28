#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 6767
#define BUFSZ 8192

#define CRLF "\r\n"
#define ROOT_FILENAME "index.html"

typedef enum { FIELD_NONE, FIELD_HOST, FIELD_AGENT } field_t;

typedef struct {
  char *method;
  char *uri;
  char *version;
  char *host;
  char *agent;
} http_req_t;

typedef struct {
  char *version;
  char *status;
  char *reason;
} status_line_t;

typedef struct {
  status_line_t status_line;
  char *content_type;
  uint32_t *content_length;
} http_res_t;

int parse_http_req(char *buf, http_req_t *req) {
  char *line_ptr;
  char *line = strtok_r(buf, CRLF, &line_ptr);

  // parse method
  char *tok_ptr;
  char *method_tok = strtok_r(line, " ", &tok_ptr);
  size_t method_tok_len = strlen(method_tok);

  req->method = calloc(method_tok_len + 1, sizeof(char));
  if (!req->method)
    return -1;

  strncpy(req->method, method_tok, method_tok_len + 1);

  // parse uri
  char *uri_tok = strtok_r(NULL, " ", &tok_ptr);
  size_t uri_tok_len = strlen(uri_tok);

  req->uri = calloc(uri_tok_len + 1, sizeof(char));
  if (!req->uri) {
    free(req->method);
    return -1;
  }
  strncpy(req->uri, uri_tok, uri_tok_len + 1);

  // parse version
  char *ver_tok = strtok_r(NULL, " ", &tok_ptr);
  size_t ver_tok_len = strlen(ver_tok);

  req->version = calloc(ver_tok_len + 1, sizeof(char));
  if (!req->version) {
    free(req->method);
    free(req->uri);
    return -1;
  }
  strncpy(req->version, ver_tok, ver_tok_len + 1);

  // parse headers
  while (line) {
    line = strtok_r(NULL, CRLF, &line_ptr);

    char *field_tok;
    char *field_tok_ptr;

    field_tok = strtok_r(line, " ", &field_tok_ptr);
    field_t curr_field = FIELD_NONE;

    while (field_tok) {
      size_t field_tok_len = strlen(field_tok);
      if (strcmp("Host:", field_tok) == 0) {
        curr_field = FIELD_HOST;
      } else if (strcmp("User-Agent:", field_tok) == 0) {
        curr_field = FIELD_AGENT;
      } else {
        if (curr_field == FIELD_HOST) {
          req->host = calloc(field_tok_len + 1, sizeof(char));
          if (!req->host) {
            free(req->method);
            free(req->uri);
            free(req->version);
            return -1;
          }
          strncpy(req->host, field_tok, field_tok_len + 1);
        } else if (curr_field == FIELD_AGENT) {
          req->agent = calloc(field_tok_len + 1, sizeof(char));
          if (!req->agent) {
            free(req->method);
            free(req->uri);
            free(req->version);
            return -1;
          }
          strncpy(req->agent, field_tok, field_tok_len + 1);
        }
      }
      field_tok = strtok_r(NULL, " ", &field_tok_ptr);
    }
  }

  return 0;
}

int handle_client(int client_fd) {
  char buf[BUFSZ] = {0};
  int bytes_read = 0;

  while ((bytes_read = recv(client_fd, buf, sizeof(buf) - 1, 0)) != 0) {
    if (bytes_read == -1) {
      perror("Read client error");
      close(client_fd);
      return -1;
    }

    buf[bytes_read] = 0;
    http_req_t req = {0};

    if (parse_http_req(buf, &req) == -1) {
      close(client_fd);
      return -1;
    }

    char tmp[256];
    if (strcmp("/", req.uri) != 0) {
      snprintf(tmp, sizeof(tmp), "./%s", ROOT_FILENAME);
    } else {
      if (strstr(req.uri, ".html")) {
        const char *not_found = "HTTP/1.1 404 Not Found\r\n"
                                "Content-Length: 0\r\n"
                                "\r\n";
        send(client_fd, not_found, strlen(not_found), 0);
        return -1;
      }
    }

    req.uri = realloc(req.uri, strlen(tmp) + 1);
    if (!req.uri) {
      perror("Req URI Realloc Fail");
      return -1;
    }
    strncpy(req.uri, tmp, strlen(tmp) + 1);

    FILE *fd = fopen(req.uri, "r");
  }

  close(client_fd);
  return 0;
}

char *create_http_response(char *content, http_res_t *res) {
  res->status_line.version = "HTTP/1.1";
  res->status_line.status = "200";
  res->status_line.reason = "OK";
}

int main(int argc, char *argv[]) {
  int socket_fd;
  struct sockaddr_in sock_addr;

  if ((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Create Socket error");
    return -1;
  }

  memset(&sock_addr, 0, sizeof(sock_addr));

  sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  sock_addr.sin_family = AF_INET;
  sock_addr.sin_len = sizeof(sock_addr);
  sock_addr.sin_port = htons(PORT);

  int yes = 1;
  setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

  if (bind(socket_fd, (const struct sockaddr *)&sock_addr, sizeof(sock_addr)) ==
      -1) {
    perror("Bind socket error");
    close(socket_fd);
    return -1;
  }

  if (listen(socket_fd, 10) != 0) {
    perror("Listening erorr");
    close(socket_fd);
    return -1;
  }

  // client
  int client_fd;
  struct sockaddr_in client_addr;
  socklen_t client_addr_size = sizeof(client_addr);

  for (;;) {
    if ((client_fd = accept(socket_fd, (struct sockaddr *)&client_addr,
                            &client_addr_size)) == -1) {
      perror("Accept error");
      continue;
    }

    int child_client;

    if ((child_client = fork()) == -1) {
      perror("Fork error");
      close(client_fd);
    } else if (child_client == 0) {
      handle_client(client_fd);
      close(socket_fd);
      return (0);
    } else {
      close(client_fd);
    }
  }

  return 0;
}
