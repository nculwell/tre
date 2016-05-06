
#include "hdrs.c"
#include "mh_net.h"
#include <sys/types.h>
#ifdef __WIN32
# include <winsock2.h>
# define socklen_t int
#else
# include <sys/socket.h>
# include <netinet/in.h>
#endif

void net_init() {
#ifdef _WIN32
  WORD versionWanted = MAKEWORD(1, 1);
  WSADATA wsaData;
  if (NO_ERROR != WSAStartup(versionWanted, &wsaData)) {
    fprintf(stderr, "Error in WSAStartup.\n");
    exit(1);
  }
#endif
  atexit(net_cleanup);
}

void net_cleanup() {
#ifdef _WIN32
  WSACleanup();
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
    exit(1);
  }
  */
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (-1 == server_fd) {
    fprintf(stderr, "Unable to open socket.\n");
    exit(1);
  }
  //struct sockaddr my_addr;
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
#ifdef _WIN32
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");
#else
  inet_aton("127.0.0.1", &addr.sin_addr.s_addr);
#endif
  addr.sin_port = htons(31909);
  int bind_result = bind(server_fd, (struct sockaddr*)&addr, sizeof addr);
  if (0 != bind_result) {
    fprintf(stderr, "Unable to bind socket.\n");
    exit(1);
  }
  int listen_result = listen(server_fd, 3);
  if (0 != listen_result) {
    fprintf(stderr, "Unable to listen on socket.\n");
    exit(1);
  }
  return server_fd;
}

int net_accept(int server_fd) {
  //struct sockaddr client_addr;
  //socklen_t client_addr_len;
  //int client_fd = accept(server_fd, &client_addr, &client_addr_len);
  int client_fd = accept(server_fd, NULL, NULL);
  if (-1 == client_fd) {
    fprintf(stderr, "Unable to accept incoming connection.\n");
#ifdef _WIN32
    fprintf(stderr, "Winsock error: %d\n", WSAGetLastError());
#endif
    exit(1);
  }
  int close_result = net_close(server_fd);
  if (-1 == close_result) {
    fprintf(stderr, "Unable to close socket.\n");
    exit(1);
  }
  return client_fd;
}

int net_recv(int client_fd, char* buf, int buf_len) {
  // TODO: MSG_DONTWAIT flag for nonblocking
  //char buf[BUFSIZ];
  //int buf_len = BUFSIZ;
  int recv_len = recv(client_fd, buf, buf_len, 0);
  if (-1 == recv_len) {
    if (EAGAIN == errno || EWOULDBLOCK == errno) {
      return -1;
    } else {
      fprintf(stderr, "Error sending data via socket.\n");
      exit(1);
    }
  }
  if (0 == recv_len) {
    // Connection closed.
    return 0;
  }
  return recv_len;
}

int net_send(int client_fd, char* buf, int buf_len) {
  // TODO: MSG_DONTWAIT flag for nonblocking
  int send_result = send(client_fd, buf, buf_len, 0);
  if (-1 == send_result) {
    if (EAGAIN == errno || EWOULDBLOCK == errno) {
      return -1;
    } else {
      fprintf(stderr, "Error receiving data via socket.\n");
      exit(1);
    }
  }
  if (send_result < buf_len) {
    // TODO: If not all data is sent, loop, or queue the rest for later.
    fprintf(stderr, "Send truncated.\n");
    exit(1);
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

