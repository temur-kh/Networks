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
#include <dirent.h>
#include "msg_types.h"
#include "p2p_types.h"
#include "database.c"
#include "hashtable.c"
#include "utils.c"
#define DEFAULT_SIZE 1024
#define MY_PORT 2000
#define MY_PORT_STR "2000"
#define MY_NAME "TemurKholmatov"
#define SEPARATOR ":"
#define COMMA ","
#define SPACE " "
#include "tcp_functions.c"

peers_list *peers;
hashtable_t *files_table;

void add_my_files(hashtable_t* table) {
  DIR *d;
  struct dirent *dir;
  d = opendir(".");
  if (d) {
    while ((dir = readdir(d)) != NULL) {
      if(valid_filename(dir->d_name)) {
        ht_set(table, dir->d_name, "localhost");
      }
    }
    closedir(d);
  }
}

void concat_data(sync_data* data) {
  char* line = data->data;
  char* my_ip = malloc(DEFAULT_SIZE * sizeof(char));
  if(get_my_ip(my_ip) == -1) {
    printf("[CONCAT DATA]: [ERROR]: Could not get my ip.\n");
    exit(1);
  }
  strcpy(line, "");
  strcat(line, MY_NAME);
  strcat(line, SEPARATOR);
  strcat(line, my_ip);
  strcat(line, SEPARATOR);
  strcat(line, MY_PORT_STR);
  strcat(line, SEPARATOR);
  for (int i=0; i<files_table->size; i++) {
    entry_t *entry = files_table->table[i];
    if (entry != NULL) {
      while(entry != NULL && entry->key != NULL) {
        if (!strcmp(entry->value, "localhost")) {
          strcat(line, entry->key);
          strcat(line, COMMA);
        }
        entry = entry->next;
      }
    }
  }
  if (line[strlen(line)-1] == ',') {
    line[strlen(line)-1] = 0;
  }
  free(my_ip);
}

void concat_peer_d(sync_peer* peer_d, node cur_nd) {
  char* line = peer_d->data;
  strcpy(line, "");
  strcat(line, cur_nd.name);
  strcat(line, SEPARATOR);
  strcat(line, cur_nd.ip_port);
}

void* sync_node(void* nd) {
  node* cur = (node*) nd;
  // printf("ok: %s %s\n", cur->name, cur->ip_port);
  struct sockaddr_in* addr = malloc(sizeof(struct sockaddr_in));
  if(create_node_addr(*cur, addr) == -1) {
    printf("[SYNC NODE]: [ERROR]: The node contains invalid ip_port: %s.\n", cur->ip_port);
    free(addr); free(cur); return NULL;
  }
  int sock = socket_create();
  if (sock == -1) {
    printf("[SYNC NODE]: [ERROR]: Could not create a socket.\n");
    free(addr); free(cur); return NULL;
  }
  if (socket_connect(sock, addr) == -1) {
    printf("[SYNC NODE]: [ERROR]: Could not connect to the remote, deleting node knowledge.\n");
    free(addr); remove_peer(peers, *cur); free(cur); close(sock); return NULL;
  }
  free(addr);

  // send the flag 1
  pcl_flag* flag = malloc(sizeof(pcl_flag));
  flag->flag = 1;
  if (socket_send(sock, (char*) &(flag->flag), sizeof(flag->flag)) == -1) {
    printf("[SYNC NODE]: [ERROR]: Could not send a protocol flag.\n");
    free(flag); free(cur); close(sock); return NULL;
  }
  free(flag);

  // send the data of this node
  sync_data* data = malloc(sizeof(sync_data));
  concat_data(data);
  if (socket_send(sock, (char*) data->data, sizeof(data->data)) == -1) {
    printf("[SYNC NODE]: [ERROR]: Could not send the personal data.\n");
    free(data); free(cur); close(sock); return NULL;
  }
  free(data);

  // send the number of peers (except for the receiving peer)
  num_t* num = malloc(sizeof(num_t));
  if (contains(peers, cur->name, cur->ip_port) != peers->size)
    num->n = (peers->size)-1;
  else
    num->n = peers->size;
  if (socket_send(sock, (char*) &(num->n), sizeof(num->n)) == -1) {
    printf("[SYNC NODE]: [ERROR]: Could not send the peers' number.\n");
    free(num); free(cur); close(sock); return NULL;
  }
  free(num);

  // send the peers one by one (except for the receiving peer)
  for (int i=0; i<peers->size; i++) {
    node cur_nd = peers->nodes[i];
    if (strcmp(cur->ip_port, cur_nd.ip_port)) {
      sync_peer* peer_d = malloc(sizeof(sync_peer));
      concat_peer_d(peer_d, cur_nd);
      if (socket_send(sock, (char*) peer_d->data, sizeof(peer_d->data)) == -1) {
        printf("[SYNC NODE]: [ERROR]: Could not send a peer's data.\n");
        free(peer_d); free(cur); close(sock); return NULL;
      }
      free(peer_d);
    }
  }
  free(cur);
  close(sock);
}

void* send_sync() {
  while (1) {
    pthread_t thread_id[DEFAULT_SIZE];
    for (int i=0; i<peers->size; i++) {
      node *cur = malloc(sizeof(node));
      strcpy(cur->name, peers->nodes[i].name);
      strcpy(cur->ip_port, peers->nodes[i].ip_port);
      pthread_create(&thread_id[i], NULL, sync_node, (void*) cur);
    }
    for (int i=0; i<peers->size; i++) {
      pthread_join(thread_id[i], NULL);
    }
    sleep(1);
  }
}

void* request() {
  while(1) {
    char rqst_file[DEFAULT_SIZE];
    printf("[REQUEST]: Input filename for searching:\n");
    scanf("%s", rqst_file);
    int ok = 0;
    while(!ok) {
      char *val = ht_get(files_table, rqst_file);
      if (val == NULL) {  // file not found
        printf("[REQUEST]: [ERROR]: No such file found.\n");
        break;
      } else if (!strcmp(val, "localhost")) {  // the file found locally
        text* txt = malloc(sizeof(txt));
        if (read_file(rqst_file, txt) == -1) {
          printf("[REQUEST]: [ERROR]: Could not read the file from localhost.\n");
          free(txt); break;
        }
        char text[DEFAULT_SIZE];
        printf("ok1\n");
        strcpy(text, "");
        printf("ok2\n");
        for (int i=0; i<txt->size; i++) {
          printf("ok3\n");
          strcat(text, txt->words[i]);
          strcat(text, SPACE);
          printf("text: %s\n", text);
          if (txt->words[i] == NULL) printf("NOT GOOOOD!\n");
          free(txt->words[i]);
        }
        if (text[strlen(text)-1] == ' ') {
          text[strlen(text)-1] = 0;
        }
        free(txt);
        ok = 1;
        printf("[REQUEST]: The file content: %s\n", text);
      }
      else {  // the file is found somewhere else
        node* cur = malloc(sizeof(node));
        strcpy(cur->name, "");
        strcpy(cur->ip_port, val);
        struct sockaddr_in* addr = malloc(sizeof(struct sockaddr_in));
        create_node_addr(*cur, addr);
        int sock = socket_create();
        if (sock == -1) {
          printf("[REQUEST]: [ERROR]: Could not create a socket.\n");
          free(cur); break;
        }
        if (socket_connect(sock, addr) == -1) {
          printf("[REQUEST]: [ERROR]: Could not connect to the remote, deleting node knowledge.\n");
          remove_peer(peers, *cur); /*TODO remove from hashtable*/; free(cur); close(sock); break;
        }

        // send the flag 0
        pcl_flag* flag = malloc(sizeof(pcl_flag));
        flag->flag = 0;
        if (socket_send(sock, (char*) &(flag->flag), sizeof(flag->flag)) == -1) {
          printf("[REQUEST]: [ERROR]: Could not send a protocol flag.\n");
          free(flag); close(sock); break;
        }
        printf("flag sent\n");
        // send the filename
        file_rqst* rqst = malloc(sizeof(file_rqst));
        strcpy(rqst->filename, rqst_file);
        if (socket_send(sock, (char*) rqst->filename, sizeof(rqst->filename)) == -1) {
          printf("[REQUEST]: [ERROR]: Could not send the filename.\n");
          free(rqst); close(sock); break;
        }
        printf("rqst sent\n");

        // receive the number of words
        num_t* num = malloc(sizeof(num_t));
        if (socket_rcv(sock, (char*) &(num->n), sizeof(num->n)) == -1) {
          printf("[REQUEST]: [ERROR]: Could not receive the number of words.\n");
          free(num); close(sock); break;
        }
        printf("num: %d\n", num->n);

        // receive the words one by one
        char text[DEFAULT_SIZE];
        strcpy(text, "");
        for (int i=0; i<num->n; i++) {
          word* wrd = malloc(sizeof(word));
          memset(wrd->val, 0, sizeof(wrd->val));
          if (socket_rcv(sock, (char*) wrd->val, sizeof(wrd->val)) == -1) {
            printf("[REQUEST]: [ERROR]: Could not receive a word.\n");
            free(num); free(wrd); close(sock); break;
          }
          printf("word: %s\n", wrd->val);
          strcat(text, wrd->val);
          strcat(text, SPACE);
          free(wrd);
        }
        if (text[strlen(text)-1] == ' ') {
          text[strlen(text)-1] = 0;
        }
        free(cur); free(flag); free(rqst); free(num); close(sock);
        ok = 1;
        printf("[REQUEST]: The file content: %s\n", text);
      }
    }
  }
}

void parse_data(sync_data* data) {
  char name[DEFAULT_SIZE];
  char ip_port[DEFAULT_SIZE];
  char rest[DEFAULT_SIZE];
  get_name_and_ip_port(data->data, name, ip_port, rest);
  insert_peer(peers, name, ip_port);
  text* txt = malloc(sizeof(txt));
  get_filenames(rest, txt);
  for (int i=0; i<txt->size; i++) {
    if (txt->words[i] != NULL && strlen(txt->words[i])) {
      ht_set(files_table, txt->words[i], ip_port);
      free(txt->words[i]);
    }
  }
  free(txt);
}

void parse_peer_d(sync_peer* peer_d) {
  char name[DEFAULT_SIZE];
  char ip_port[DEFAULT_SIZE];
  get_name_and_ip_port(peer_d->data, name, ip_port, NULL);
  insert_peer(peers, name, ip_port);
}

void* rcv_sync(void* sck) {
  int* sock = (int*) sck;

  // receive the data of the node
  sync_data* data = malloc(sizeof(sync_data));
  memset(data->data, 0, sizeof(data->data));
  if (socket_rcv(*sock, (char*) data->data, sizeof(data->data)) == -1) {
    printf("[RCV SYNC]: [ERROR]: Could not receive the personal data.\n");
    free(data); close(*sock); free(sock); return NULL;
  }
  parse_data(data);
  free(data);

  // send the number of peers (except for the receiving peer)
  num_t* num = malloc(sizeof(num_t));
  if (socket_rcv(*sock, (char*) &(num->n), sizeof(num->n)) == -1) {
    printf("[RCV SYNC]: [ERROR]: Could not receive the peers' number.\n");
    free(num); close(*sock); free(sock); return NULL;
  }

  // send the peers one by one (except for the receiving peer)
  for (int i=0; i<num->n; i++) {
    sync_peer* peer_d = malloc(sizeof(sync_peer));
    memset(peer_d->data, 0, sizeof(peer_d->data));
    if (socket_rcv(*sock, (char*) peer_d->data, sizeof(peer_d->data)) == -1) {
      printf("[RCV SYNC]: [ERROR]: Could not receive a peer's data.\n");
      free(peer_d); close(*sock); free(sock); return NULL;
    }
    parse_peer_d(peer_d);
    free(peer_d);
  }
  free(num);
  close(*sock); free(sock);
}

void* response(void* sck) {
  int* sock = (int*) sck;

  // receive the filename
  file_rqst* rqst = malloc(sizeof(file_rqst));
  memset(rqst->filename, 0, sizeof(rqst->filename));
  if (socket_rcv(*sock, (char*) rqst->filename, sizeof(rqst->filename)) == -1) {
    printf("[RESPONSE]: [ERROR]: Could not receive the filename.\n");
    free(rqst); close(*sock); free(sock); return NULL;
  }
  printf("received filename: %s\n", rqst->filename);
  if (!valid_filename(rqst->filename)) {
    printf("[RESPONSE]: [SECURITY]: Attempt to access another file.\n");
    free(rqst); close(*sock); free(sock); return NULL;
  }
  text* txt = malloc(sizeof(txt));
  if (read_file(rqst->filename, txt) == -1) {
    printf("[RESPONSE]: [ERROR]: Could not read the file.\n");
    free(rqst); free(txt); close(*sock); free(sock); return NULL;
  }
  free(rqst);

  printf("read file\n");
  // send the number of words
  num_t* num = malloc(sizeof(num_t));
  num->n = txt->size;
  if (socket_send(*sock, (char*) &(num->n), sizeof(num->n)) == -1) {
    printf("[RESPONSE]: [ERROR]: Could not send the number of words.\n");
    free(num); free(txt); close(*sock); free(sock); return NULL;
  }
  printf("num sent: %d\n", num->n);
  free(num);
  // send the words one by one
  for (int i=0; i<txt->size; i++) {
    printf("OK\n");
    word* wrd = malloc(sizeof(word));
    memset(wrd->val, 0, sizeof(wrd->val));
    strcpy(wrd->val, txt->words[i]);
    if (socket_send(*sock, (char*) wrd->val, sizeof(wrd->val)) == -1) {
      printf("[RESPONSE]: [ERROR]: Could not send a word.\n");
      free(wrd); free(txt); close(*sock); free(sock); return NULL;
    }
    printf("words sent: %s", wrd->val);
    free(wrd); free(txt->words[i]);
  }
  free(txt);
  close(*sock); free(sock);
}

void* server() {
  int master_sock = socket_create();
  if (master_sock == -1) {
    printf("[SERVER]: [ERROR]: Could not create a master socket.\n");
    exit(1);
  }
  if (bind_socket(master_sock) == -1) {
    printf("[SERVER]: [ERROR]: Could not bind a master socket.\n");
    exit(1);
  }
  if (listen(master_sock, 10) == -1) {
    printf("[SERVER]: [ERROR]: Could not listen.\n");
    exit(1);
  }
  pthread_t pthread_id[DEFAULT_SIZE];
  int len = sizeof(struct sockaddr_in), ind = 0;
  while(1) {
    int *sock = malloc(sizeof(int));
    struct sockaddr_in addr;
    // printf("ok1\n");
    *sock = accept(master_sock, (struct sockaddr*) &addr, &len);
    // printf("ok2\n");
    if (*sock == -1) {
      printf("[SERVER]: [ERROR]: Could not accept a connection.\n");
      free(sock); continue;
    }

    // receive a flag
    pcl_flag* flag = malloc(sizeof(pcl_flag));
    if (socket_rcv(*sock, (char*) &(flag->flag), sizeof(flag->flag)) == -1) {
      printf("[SERVER]: [ERROR]: Could not receive a flag.\n");
      free(flag); close(*sock); free(sock); continue;
    }
    printf("flag received: %d\n", flag->flag);
    if (flag->flag == 1) {
      pthread_create(&pthread_id[(ind++)%DEFAULT_SIZE], NULL, rcv_sync, (void*) sock);
    } else if (flag->flag == 0){
      pthread_create(&pthread_id[(ind++)%DEFAULT_SIZE], NULL, response, (void*) sock);
    }
    free(flag);
  }
  close(master_sock);
}

int main(int argc, char **argv) {
  files_table = ht_create(DEFAULT_SIZE);
  add_my_files(files_table);
  peers = create_peers();

  char server_ip_port[DEFAULT_SIZE];
  printf("[MAIN] Enter the first peer \"ip:port\" >> ");
  scanf("%s", server_ip_port);
  if (strcmp(server_ip_port, "first")) {
    insert_peer(peers, "", server_ip_port);
  }

  pthread_t thread_id[3];
  pthread_create(&thread_id[0], NULL, server, NULL);
  sleep(1);
  pthread_create(&thread_id[1], NULL, send_sync, NULL);
  pthread_create(&thread_id[2], NULL, request, NULL);
  for (int i=0; i<3; i++) {
    pthread_join(thread_id[i], NULL);
  }
  return 0;
}
