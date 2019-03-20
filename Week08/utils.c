// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <netdb.h>
// #include <memory.h>
// #include <errno.h>
// // #include "common.h"
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <arpa/inet.h>
// #include <pthread.h>
// // #include "node.h"
// #include "message.h"
#include <ifaddrs.h>

int concat_ip_port(char* ip, char* port, char* res) {
  strcpy(res, ip);
  strcat(res, ":");
  strcat(res, port);
  return 0;
}

int get_ip_port(char* ip_port, char* ip, int* port) {
  char div[] = ":";
  char *ptr = strtok(ip_port, div);
  if (ptr != NULL) {
    strcpy(ip, ptr);
  } else return -1;

  ptr = strtok(NULL, div);
  if (ptr != NULL) {
    *port = atol(ptr);
  } else return -1;

  return 0;
}

int create_node_addr(node nd, struct sockaddr_in* addr, int condition) {
  int port;
  char ip[20];
  if(get_ip_port(nd.ip_port, ip, &port) == -1) {
    printf("[ERROR] Could not split ip_port.\n");
    return -1;
  }
  addr->sin_family = AF_INET;
  addr->sin_port = port;
  if (condition) {
    struct hostent *host = (struct hostent *)gethostbyname(ip);
    addr->sin_addr = *((struct in_addr *)host->h_addr);
  }
}

int get_my_ip(char* my_ip) {
  struct ifaddrs *addrs, *tmp;
  getifaddrs(&addrs);
  tmp = addrs;

  while (tmp) {
      if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET) {
          struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
          if (!strcmp("wlp2s0", tmp->ifa_name)) {
            strcpy(my_ip, inet_ntoa(pAddr->sin_addr));
            freeifaddrs(addrs);
            return 0;
          }
      }
      tmp = tmp->ifa_next;
  }

  freeifaddrs(addrs);
  return -1;
}
//
//
// int main() {
//   char ip[20];
//   if (get_my_ip(ip) == -1) {
//     printf("error\n");
//     return -1;
//   }
//
//   printf("%s\n", ip);
//
// }
