#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <memory.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "p2p_types.h"
#include "utils.c"
#include "hashtable.c"

#define MY_PORT "2000"

char data_buffer[10000];
char my_ip[20];
char server_ip[20];
int server_port;
ping_msg out_ping;
hashtable_t *files_table;


void create_out_ping() {
  strcpy(out_ping.self.name, "TemurKholmatov");
  if(get_my_ip(my_ip) == -1) {
    printf("[ERROR] Could not get my ip.\n");
    exit(-1);
  }
  concat_ip_port(my_ip, MY_PORT, out_ping.self.ip_port);

  out_ping.array_of_known_nodes = malloc(1 * sizeof(node));
  out_ping.array_size = 1;
  strcpy(out_ping.client_rqst, "");
  strcpy(out_ping.server_rspn, "");

  node *nd = &(out_ping.array_of_known_nodes[0]);
  strcpy(nd->name, out_ping.self.name);
  strcpy(nd->ip_port, out_ping.self.ip_port);
  // strcpy(nd->name, "FIRST PEER");
  // char port[10];
  // sprintf(port, "%d", server_port);
  // concat_ip_port(server_ip, port, nd->ip_port);
}

int add_to_list(node* nodes, int* list_sz, node nd) {
  for(int i=0;i<*list_sz;i++) {
    // printf("OK222\n");
    if (!strcmp(nd.name, nodes[i].name) && !strcmp(nd.ip_port, nodes[i].ip_port)) {
      // printf("OK333\n");
      return 0;
    }
  }
  // printf("OK111\n");
  *list_sz = (*list_sz) + 1;
  nodes = realloc(nodes, (*list_sz) * sizeof(node));
  // printf("OK444\n");
  return 1;
}

void update_my_knowledge(ping_msg in_ping) {
  node* nd_list = out_ping.array_of_known_nodes;
  int* list_sz = &out_ping.array_size;
  // printf("[UPDATING] Msg from name:ip:port = %s:%s\n", in_ping.self.name, in_ping.self.ip_port);
  node* nodes = in_ping.array_of_known_nodes;
  for(int i=0;i<in_ping.array_size;i++) {
    if(add_to_list(nd_list, list_sz, nodes[i])==1);
      // printf("[UPDATING] Node: %s:%s\n", nodes[i].name, nodes[i].ip_port);
  }
}

int ping(node nd, ping_msg** in_ping) {
  int sockfd = 0, sent_recv_bytes = 0;
  int addr_len = sizeof(struct sockaddr);
  struct sockaddr_in dest;

  if (create_node_addr(nd, &dest, 1) == -1) {
    exit(1);
  }
  sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  connect(sockfd, (struct sockaddr *) &dest, sizeof(struct sockaddr));
  // printf("rqst: %s\n", out_ping.client_rqst);
  sent_recv_bytes = sendto(sockfd, &out_ping, sizeof(out_ping), 0,
    (struct sockaddr *) &dest, sizeof(struct sockaddr));
  // printf("[PING PEERS] No of bytes sent = %d\n", sent_recv_bytes);

  memset(data_buffer, 0, sizeof(data_buffer));
  sent_recv_bytes =  recvfrom(sockfd, (char *)&data_buffer, sizeof(data_buffer), 0,
            (struct sockaddr *)&dest, &addr_len);
  *in_ping = (ping_msg *) data_buffer;
  // printf("[PING PEERS] No of bytes received = %d\n", sent_recv_bytes);
  close(sockfd);
  // printf("OK11\n");
  return sent_recv_bytes;
}

void* ping_peers(void* arg) {

  node* nd_list = out_ping.array_of_known_nodes;
  int* list_sz = &out_ping.array_size;

  while(1) {
    for (int i=0; i<(*list_sz);i++) {
      ping_msg *in_ping;
      int sent_recv_bytes;
      // printf("OK10\n");
      sent_recv_bytes = ping(nd_list[i], &in_ping);
      // printf("OK13.5\n");
      if (sent_recv_bytes != 0) {
        // printf("OK14\n");
        update_my_knowledge(*in_ping);
        // printf("OK15\n");
      } else continue;

      if (!strcmp(in_ping->server_rspn, "ok")) {
        printf("[FILE] OK\n");
        ht_set(files_table, out_ping.client_rqst, nd_list[i].ip_port);
        strcpy(out_ping.client_rqst, "next...");
        ping(nd_list[i], &in_ping);
        int words_num = atol(in_ping->server_rspn);
        printf("[FILE] Size in words: %d\n", words_num);
        printf("[FILE] Text:");
        for (int j=0;j<words_num;j++) {
          strcpy(out_ping.client_rqst, "next...");
          ping(nd_list[i], &in_ping);
          char *word = in_ping->server_rspn;
          printf(" %s", word);
          // printf("OK12\n");
        }
        printf("\n");
        break;
      }
    }
    strcpy(out_ping.client_rqst, "");
    strcpy(out_ping.server_rspn, "");
    // printf("ok\n");
    sleep(2);
  }
}

void *find_file() {
  sleep(1);
  while(1) {
    while(strcmp(out_ping.client_rqst, ""));// printf("OK123\n");
    printf("[FIND FILE] Input file name: ");
    scanf("%s", out_ping.client_rqst);
  }
}

int server_rcv(struct sockaddr_in* client_addr, ping_msg** in_ping, int master_sock_tcp_fd, int* comm_socket_fd) {
  int addr_len = sizeof(struct sockaddr);
  *comm_socket_fd = accept(master_sock_tcp_fd, (struct sockaddr *) client_addr, &addr_len);
  // printf("ok1\n");
  if (*comm_socket_fd < 0) {
      printf("[ANS PEERS] accept error : errno = %d\n", errno);
      exit(1);
  }
  memset(data_buffer, 0, sizeof(data_buffer));
  // printf("ok2\n");
  int sent_recv_bytes = recvfrom(*comm_socket_fd, (char *) data_buffer, sizeof(data_buffer), 0,
                            (struct sockaddr *) client_addr, &addr_len);
  // printf("ok3 %d\n", sent_recv_bytes);
  if (sent_recv_bytes == 0) {
     close(*comm_socket_fd);
     return 0;
  }
  // printf("ok4\n");
  *in_ping = (ping_msg *) data_buffer;
  // printf("ok5 %s\n", (*in_ping)->client_rqst);
  return sent_recv_bytes;
}

void server_snd(struct sockaddr_in* client_addr, int comm_socket_fd) {
  int sent_recv_bytes = sendto(comm_socket_fd, (char *) &out_ping, sizeof(out_ping), 0,
                          (struct sockaddr *) client_addr, sizeof(struct sockaddr));
  // printf("ok6 %d\n", sent_recv_bytes);
  close(comm_socket_fd);
  // printf("ok7\n");
}

void read_file(char *filename, char** words, int* words_num) {
  FILE * fp = fopen(filename, "r");
	if (fp == NULL) exit(1);
  // char *x = malloc(1024);
  // words = malloc(1024*sizeof(char *));
  char x[1024];
  while (fscanf(fp, " %1023s", x) == 1) {
    // words[*words_num] = x;
    words[*words_num] = malloc(sizeof(x));
    strcpy(words[*words_num], x);
    (*words_num)++;
    memset(x, 0, sizeof(x));
  }
	fclose(fp);
}

void *answer_peers() {
    int master_sock_tcp_fd = 0, sent_recv_bytes = 0;
    fd_set readfds;
    struct sockaddr_in my_addr;

    if ((master_sock_tcp_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("[ANS PEERS] socket creation failed\n");
        exit(1);
    }
    create_node_addr(out_ping.self, &my_addr, 0);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(master_sock_tcp_fd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr)) == -1) {
        printf("[ANS PEERS] socket bind failed\n");
        exit(1);
    }
    if (listen(master_sock_tcp_fd, 5) < 0) {
        printf("[ANS PEERS] listen failed\n");
        exit(1);
    }

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(master_sock_tcp_fd, &readfds);
        select(master_sock_tcp_fd + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(master_sock_tcp_fd, &readfds)) {
          ping_msg* in_ping;
          struct sockaddr_in client_addr;
          int comm_socket_fd;
          // printf("Heell\n");
          server_rcv(&client_addr, &in_ping, master_sock_tcp_fd, &comm_socket_fd);
          // printf("in rqst: %s\n", in_ping->client_rqst);
          // printf("hell\n");
          update_my_knowledge(*in_ping);
          // printf("ok312\n");
          if (strcmp(in_ping->client_rqst, "")) {
            // printf("1\n");
            char filename[1024];
            strcpy(filename, in_ping->client_rqst);
            if (access(filename, F_OK) != -1) {

              strcpy(out_ping.server_rspn, "ok");
              server_snd(&client_addr, comm_socket_fd);

              char *words[1024];
              int words_num = 0;
              read_file(filename, words, &words_num);

              server_rcv(&client_addr, &in_ping, master_sock_tcp_fd, &comm_socket_fd);

              char num[20];
              sprintf(num, "%d", words_num);
              strcpy(out_ping.server_rspn, num);
              server_snd(&client_addr, comm_socket_fd);
              // printf("ok09\n");
              for (int i=0;i<words_num;i++) {
                // printf("ok10\n");
                server_rcv(&client_addr, &in_ping, master_sock_tcp_fd, &comm_socket_fd);
                // printf("ok11 %s\n", words[i]);
                strcpy(out_ping.server_rspn, words[i]);
                // printf("ok12\n");
                server_snd(&client_addr, comm_socket_fd);
                // printf("ok13\n");
                free(words[i]);
              }
            } else {
              // printf("ok08\n");
              strcpy(out_ping.server_rspn, "not ok");
              server_snd(&client_addr, comm_socket_fd);
              close(comm_socket_fd);
            }
            // printf("1:\n");
          } else {
            // printf("2\n");
            server_snd(&client_addr, comm_socket_fd);
            // printf("2:\n");
          }

        }
    }
}

int main(int argc, char **argv) {
  files_table = ht_create(10000);
  printf("[MAIN] Enter the first peer \"ip port\" >> ");
  scanf("%s", server_ip);
  scanf("%d", &server_port);
  create_out_ping();

  pthread_t thread_id[3];
  pthread_create(&thread_id[0], NULL, ping_peers, NULL);
  pthread_create(&thread_id[1], NULL, answer_peers, NULL);
  pthread_create(&thread_id[2], NULL, find_file, NULL);
  pthread_join(thread_id[0], NULL);
  pthread_join(thread_id[1], NULL);
  pthread_join(thread_id[2], NULL);
  return 0;
}
