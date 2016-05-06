
#include "hdrs.c"
#include "mh_net.h"
#include <sys/types.h>
#ifdef __WIN32
# include <winsock2.h>
# define socklen_t int
#else
# include <sys/socket.h>
#endif

int net_listen() {
#ifdef __WIN32__
  WORD versionWanted = MAKEWORD(1, 1);
  WSADATA wsaData;
  WSAStartup(versionWanted, &wsaData);
#endif
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
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (-1 == sockfd) {
    abort();
  }
  //struct sockaddr my_addr;
  struct sockaddr_in addr;
  int bind_result = bind(sockfd, (struct sockaddr*)&addr, sizeof addr);
  if (0 != bind_result) {
    abort();
  }
  int listen_result = listen(sockfd, 3);
  if (0 != listen_result) {
    abort();
  }
  return sockfd;
}

int net_accept(int sockfd) {
  struct sockaddr client_addr;
  socklen_t client_addr_len;
  int accept_result = accept(sockfd, &client_addr, &client_addr_len);
  if (0 != accept_result) {
    abort();
  }
  return 1;
}

