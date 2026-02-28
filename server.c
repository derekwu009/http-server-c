#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 6767
#define BUFFER_SZ 256

int main(int argc, char *argv[]) {
  int socket_fd;
  struct sockaddr_in sock_addr;

  if ((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Create Socket error");
    return EXIT_FAILURE;
  }

  memset(&sock_addr, 0, sizeof(sock_addr));

  sock_addr.sin_addr.s_addr = htons(INADDR_ANY);
  sock_addr.sin_family = AF_INET;
  sock_addr.sin_len = sizeof(sock_addr);
  sock_addr.sin_port = htons(PORT);

  if (bind(socket_fd, (const struct sockaddr *)&sock_addr, sizeof(sock_addr)) ==
      -1) {
    perror("Bind socket error");
    close(socket_fd);
    return EXIT_FAILURE;
  }

  if (listen(socket_fd, 10) != 0) {
    perror("Listening erorr");
    close(socket_fd);
    return EXIT_FAILURE;
  }

  // client
  int client_fd;
  char buffer[BUFFER_SZ] = {0};
  struct sockaddr_in client_addr;
  socklen_t client_addr_size = sizeof(client_addr);

  if ((client_fd = accept(socket_fd, (struct sockaddr *)&client_addr,
                          &client_addr_size)) == -1) {
    perror("Accept error");
    close(socket_fd);
    return EXIT_FAILURE;
  }

  if (recv(client_fd, buffer, BUFFER_SZ, 0) == -1) {
    perror("Read error");
    close(socket_fd);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
