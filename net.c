
#include "hdrs.c"
#include "mh_net.h"
#include <sys/types.h>
#ifdef __WIN32
# include <winsock2.h>
# define socklen_t int
#else
# include <sys/socket.h>
#endif

void net_init() {
#ifdef __WIN32__
  WORD versionWanted = MAKEWORD(1, 1);
  WSADATA wsaData;
  WSAStartup(versionWanted, &wsaData);
#endif
}

int net_listen() {
  /*
  struct addrinfo hints;
  struct addrinfo *server_info;
  // Set up getaddrinfo hints.
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET; // AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  int ai_result = getaddrinfo(NULL, "31909", &hints, &server_info);
  if (0 != ai_result) {
    abort();
  }
  */
  int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (-1 == sock_fd) {
    abort();
  }
  //struct sockaddr my_addr;
  struct sockaddr_in addr;
  int bind_result = bind(sock_fd, (struct sockaddr*)&addr, sizeof addr);
  if (0 != bind_result) {
    abort();
  }
  int listen_result = listen(sock_fd, 3);
  if (0 != listen_result) {
    abort();
  }
  return sock_fd;
}

int net_accept(int sock_fd) {
  struct sockaddr client_addr;
  socklen_t client_addr_len;
  int accepted_fd = accept(sock_fd, &client_addr, &client_addr_len);
  if (-1 == accepted_fd) {
    abort();
  }
  int close_result = net_close(sock_fd);
  if (-1 == close_result) {
    abort();
  }
  return accepted_fd;
}

int net_recv(int sock_fd) {
  // TODO: MSG_DONTWAIT flag for nonblocking
  char buf[BUFSIZ];
  int buf_len = BUFSIZ;
  int recv_len = recv(sock_fd, buf, buf_len, 0);
  if (-1 == recv_len) {
    if (EAGAIN == errno || EWOULDBLOCK == errno) {
      return -1;
    } else {
      abort();
    }
  }
  if (0 == recv_len) {
    // Connection closed.
    return 0;
  }
  return recv_len;
}

int net_send(int sock_fd, char* buf, int buf_len) {
  // TODO: MSG_DONTWAIT flag for nonblocking
  int send_result = send(sock_fd, buf, buf_len, 0);
  if (-1 == send_result) {
    if (EAGAIN == errno || EWOULDBLOCK == errno) {
      return -1;
    } else {
      abort();
    }
  }
  if (send_result < buf_len) {
    // TODO: If not all data is sent, loop, or queue the rest for later.
    fprintf(stderr, "Send truncated.\n");
    abort();
  }
  return 1;
}

int net_close(int sock_fd) {
  return 
#ifdef _WIN32
    closesocket(sock_fd);
#else
    close(sock_fd);
#endif
}

