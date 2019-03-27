#include <ifaddrs.h>

// int concat_ip_port(char* ip, char* port, char* res) {
//   strcpy(res, ip);
//   strcat(res, ":");
//   strcat(res, port);
//   return 0;
// }

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

int get_name_and_ip_port(char* line, char* name, char* ip_port, char* rest) {
  char div[] = ":";
  char *ptr = strtok(line, div);
  if (ptr != NULL) {
    strcpy(name, ptr);
  } else return -1;

  ptr = strtok(NULL, div);
  if (ptr != NULL) {
    strcpy(ip_port, ptr);
  } else return -1;
  ptr = strtok(NULL, div);
  if (ptr != NULL) {
    strcat(ip_port, ":");
    strcat(ip_port, ptr);
  } else return -1;

  ptr = strtok(NULL, div);
  if (ptr != NULL && rest != NULL) {
    strcpy(rest, ptr);
  } else if (rest != NULL) {
    strcpy(rest, "");
  }
  return 0;
}

int get_filenames(char* rest, text* text) {
  char div[] = ",";
  char *ptr = strtok(rest, div);
  text->size = 0;
  while(ptr != NULL) {
    text->words[text->size] = malloc(strlen(ptr));
    strcpy(text->words[text->size], ptr);
    (text->size)++;
    ptr = strtok(NULL, div);
  }
  return text->size;
}

int create_node_addr(node nd, struct sockaddr_in* addr) {
  int port;
  char ip[1024];
  if(get_ip_port(nd.ip_port, ip, &port) == -1) {
    printf("[CREATE NODE ADDR]: [ERROR]: Could not split ip_port: %s.\n", nd.ip_port);
    return -1;
  }
  addr->sin_family = AF_INET;
  addr->sin_port = htons(port);
  struct hostent *host = (struct hostent *)gethostbyname(ip);
  addr->sin_addr = *((struct in_addr *)host->h_addr);
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

int read_file(char *filename, text* text) {
  FILE * fp = fopen(filename, "r");
	if (fp == NULL) return -1;
  char x[1024];
  text->size = 0;
  while (fscanf(fp, " %1023s", x) == 1) {
    text->words[text->size] = malloc(strlen(x));
    strcpy(text->words[text->size], x);
    (text->size)++;
    memset(x, 0, sizeof(x));
  }
	fclose(fp);
  return text->size;
}

int valid_filename(char *file) {
  return strlen(file) > 4 && !strcmp(file + strlen(file) - 4, ".txt");
}
