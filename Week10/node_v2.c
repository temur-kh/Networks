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
#include "hashtable_v2.c"
#include "utils.c"
#define DEFAULT_SIZE 1024
#define MY_PORT 2000
#define BLDB_LIMIT 5
#define MY_PORT_STR "2000"
#define MY_NAME "TemurKholmatov"
#define SEPARATOR ":"
#define COMMA ","
#define SPACE " "
#include "tcp_functions.c"

peers_list *peers;
hashtable_t *kdb;
hashtable_t *cdb;
hashtable_t *bldb;

pthread_mutex_t cdb_mx, bldb_mx;

void add_my_files(hashtable_t* table) {
  DIR *d;
  struct dirent *dir;
  d = opendir(".");
  if (d) {
    while ((dir = readdir(d)) != NULL) {
      if(valid_filename(dir->d_name)) {
        ht_put(table, dir->d_name, (void*) "localhost");
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
  for (int i=0; i<kdb->size; i++) {
    hash_elem_t *entry = kdb->table[i];
    if (entry != NULL) {
      while(entry != NULL && entry->key != NULL) {
        if (!strcmp(entry->data, "localhost")) {
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
    sleep(5);
  }
}

void* request() {
  while(1) {
    char rqst_file[DEFAULT_SIZE];
    printf("[REQUEST]: Input filename for searching:\n");
    scanf("%s", rqst_file);
    int ok = 0;
    while(!ok) {
      char *val = ht_get(kdb, rqst_file);
      if (val == NULL) {  // file not found
        printf("[REQUEST]: [ERROR]: No such file found.\n");
        break;
      } else if (!strcmp(val, "localhost")) {  // the file found locally
        text* txt = malloc(sizeof(text));
        if (read_file(rqst_file, txt) == -1) {
          printf("[REQUEST]: [ERROR]: Could not read the file from localhost.\n");
          free(txt); break;
        }
        char text[DEFAULT_SIZE];
        strcpy(text, "");
        for (int i=0; i<txt->size; i++) {
          strcat(text, txt->words[i]);
          strcat(text, SPACE);
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
          printf("[REQUEST]: [ERROR]: Could not connect to the remote, deleting node knowledge: %s.\n", cur->ip_port);
          remove_peer(peers, *cur); /*TODO remove from hashtable*/; free(cur); close(sock); break;
        }

        // send the flag 0
        pcl_flag* flag = malloc(sizeof(pcl_flag));
        flag->flag = 0;
        if (socket_send(sock, (char*) &(flag->flag), sizeof(flag->flag)) == -1) {
          printf("[REQUEST]: [ERROR]: Could not send a protocol flag.\n");
          free(flag); close(sock); break;
        }

        // send the filename
        file_rqst* rqst = malloc(sizeof(file_rqst));
        strcpy(rqst->filename, rqst_file);
        if (socket_send(sock, (char*) rqst->filename, sizeof(rqst->filename)) == -1) {
          printf("[REQUEST]: [ERROR]: Could not send the filename.\n");
          free(rqst); close(sock); break;
        }

        // receive the number of words
        num_t* num = malloc(sizeof(num_t));
        if (socket_rcv(sock, (char*) &(num->n), sizeof(num->n)) == -1) {
          printf("[REQUEST]: [ERROR]: Could not receive the number of words.\n");
          free(num); close(sock); break;
        }

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
          strcat(text, wrd->val);
          strcat(text, SPACE);
          free(wrd);
          usleep(20000);
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

void parse_data(sync_data* data, char* ip_port_out) {
  char name[DEFAULT_SIZE];
  char ip_port[DEFAULT_SIZE];
  char rest[DEFAULT_SIZE];
  // printf("[PARSE DATA] data: %s\n", data->data);
  get_name_and_ip_port(data->data, name, ip_port, rest);
  // printf("[PARSE DATA]: info: '%s' '%s' '%s' '%s'\n", data->data, name, ip_port, rest);
  strcpy(ip_port_out, ip_port);
  insert_peer(peers, name, ip_port);
  text* txt = malloc(sizeof(text));
  get_filenames(rest, txt);
  for (int i=0; i<txt->size; i++) {
    if (txt->words[i] != NULL && strlen(txt->words[i])) {
      ht_put(kdb, txt->words[i], (void*) ip_port);
    }
  }
  free(txt);
}

void parse_peer_d(sync_peer* peer_d) {
  char name[DEFAULT_SIZE];
  char ip_port[DEFAULT_SIZE];
  if (peer_d->data[0] == ':') return;
  get_name_and_ip_port(peer_d->data, name, ip_port, NULL);
  // printf("peer data: %s %s\n", name, ip_port);
  insert_peer(peers, name, ip_port);
}

void* rcv_sync(void* sck) {
  int* sock = (int*) sck;
  struct sockaddr_in addr;
  int alen;
  if (getsockname(*sock, (struct sockaddr*)&addr, &alen) == -1) {
    printf("[RCV SYNC]: [ERROR]: Could not read addr of socket.\n");
    close(*sock); free(sock); return NULL;
  }
  char *ip_port = inet_ntoa(addr.sin_addr);
  printf("[RCV SYNC]: IP in socket addr to be hashed: %s\n", ip_port);

  pthread_mutex_lock(&cdb_mx);
  int *cdb_data = (int*) ht_get(cdb, ip_port);
  pthread_mutex_unlock(&cdb_mx);
  if (cdb_data != NULL) {
    int as = *cdb_data;
    printf("[RCV SYNC]: count: %d\n", as);
  }
  if (ht_get(bldb, ip_port) != NULL) {
    printf("[RCV SYNC]: [WARNING]: The node %s is in the black list.\n", ip_port);
    /*free(ip_port);*/ close(*sock); free(sock); return NULL;
  } else if (cdb_data != NULL) {
    if (*cdb_data > BLDB_LIMIT) {
      // printf("HERE\n");
      printf("[RCV SYNC] Current the cdb count: %d.\n", *cdb_data);
      pthread_mutex_lock(&bldb_mx);
      ht_put(bldb, ip_port, (void*) cdb_data);
      pthread_mutex_unlock(&bldb_mx);
      pthread_mutex_lock(&cdb_mx);
      ht_remove(cdb, ip_port);
      pthread_mutex_unlock(&cdb_mx);
      printf("[RCV SYNC]: [WARNING]: The node %s is put to the black list: %d.\n", ip_port, *cdb_data);
      /*free(ip_port);*/ close(*sock); free(sock); return NULL;
    } else {
      pthread_mutex_lock(&cdb_mx);
      printf("[RCV SYNC] Current the cdb count: %d.\n", *cdb_data);
      int new_data = (*cdb_data) + 1;
      printf("[RCV SYNC]: Increase the cdb count of %s - %d.\n", ip_port, new_data);
      ht_put(cdb, ip_port, (void*) &new_data);
      pthread_mutex_unlock(&cdb_mx);
    }
  } else {
    // cdb_data = malloc(sizeof(int));
    int new_data = 1;
    pthread_mutex_lock(&cdb_mx);
    printf("[RCV SYNC]: Put the cdb count of %s to 1.\n", ip_port);
    ht_put(cdb, ip_port, (void*) &new_data);
    pthread_mutex_unlock(&cdb_mx);
    // free(cdb_data);
  }

  // receive the data of the node
  int recv_bytes = 0;
  sync_data* data = malloc(sizeof(sync_data));
  memset(data->data, 0, sizeof(data->data));
  if ((recv_bytes = socket_rcv(*sock, (char*) data->data, sizeof(data->data))) == -1) {
    printf("[RCV SYNC]: [ERROR]: Could not receive the personal data.\n");
    free(data); close(*sock); free(sock); return NULL;
  }
  if (recv_bytes == 0) {
    printf("[RCV SYNC]: [WARNING]: No data send by the peer %s\n", ip_port);
    free(data); close(*sock); free(sock); return NULL;
  }
  char *ip = malloc(DEFAULT_SIZE);
  parse_data(data, ip);
  free(ip);
  free(data);

  // receive the number of peers (except for the receiving peer)
  num_t* num = malloc(sizeof(num_t));
  recv_bytes = 0;
  if ((recv_bytes = socket_rcv(*sock, (char*) &(num->n), sizeof(num->n))) == -1) {
    printf("[RCV SYNC]: [ERROR]: Could not receive the peers' number.\n");
    free(num); close(*sock); free(sock); return NULL;
  }
  if (recv_bytes == 0) {
    printf("[RCV SYNC]: [WARNING]: No data send by the peer %s\n", ip_port);
    free(num); close(*sock); free(sock); return NULL;
  }
  printf("[RCV SYNC]: Received number of peers: %d\n", num->n );

  // receive the peers one by one
  for (int i=0; i<num->n; i++) {
    sync_peer* peer_d = malloc(sizeof(sync_peer));
    memset(peer_d->data, 0, sizeof(peer_d->data));
    if (socket_rcv(*sock, (char*) peer_d->data, sizeof(peer_d->data)) == -1) {
      printf("[RCV SYNC]: [ERROR]: Could not receive a peer's data.\n");
      free(num); free(peer_d); close(*sock); free(sock); return NULL;
    }
    printf("[RCV SYNC]: Received peer data: %s\n", peer_d->data);
    parse_peer_d(peer_d);
    free(peer_d);
    usleep(20000);
  }
  free(num);

  pthread_mutex_lock(&cdb_mx);
  cdb_data = (int*) ht_get(cdb, ip_port);
  int new_data = (*cdb_data) - 1;
  // printf("[RCV SYNC]: Decrease the cdb count for %s - %d.\n", ip_port, new_data);
  ht_put(cdb, ip_port, (void*) &new_data);
  // cdb_data = (int*) ht_get(cdb, ip_port);
  // new_data = (*cdb_data);
  printf("[RCV SYNC]: Decreased the cdb count for %s - %d.\n", ip_port, new_data);
  pthread_mutex_unlock(&cdb_mx);
  close(*sock); free(sock);
  // printf("FINISH\n");
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
  if (!valid_filename(rqst->filename)) {
    printf("[RESPONSE]: [SECURITY]: Attempt to access another file.\n");
    free(rqst); close(*sock); free(sock); return NULL;
  }
  text* txt = malloc(sizeof(text));
  if (read_file(rqst->filename, txt) == -1) {
    printf("[RESPONSE]: [ERROR]: Could not read the file.\n");
    free(rqst); free(txt); close(*sock); free(sock); return NULL;
  }
  free(rqst);

  // send the number of words
  num_t* num = malloc(sizeof(num_t));
  num->n = txt->size;
  if (socket_send(*sock, (char*) &(num->n), sizeof(num->n)) == -1) {
    printf("[RESPONSE]: [ERROR]: Could not send the number of words.\n");
    free(num); free(txt); close(*sock); free(sock); return NULL;
  }
  free(num);

  // send the words one by one
  for (int i=0; i<txt->size; i++) {
    word* wrd = malloc(sizeof(word));
    memset(wrd->val, 0, sizeof(wrd->val));
    strcpy(wrd->val, txt->words[i]);
    if (socket_send(*sock, (char*) wrd->val, strlen(wrd->val)) == -1) {
      printf("[RESPONSE]: [ERROR]: Could not send a word.\n");
      free(wrd); free(txt); close(*sock); free(sock); return NULL;
    }
    free(wrd);
    usleep(20000);
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
  if (listen(master_sock, 25) == -1) {
    printf("[SERVER]: [ERROR]: Could not listen.\n");
    exit(1);
  }
  pthread_t pthread_id[DEFAULT_SIZE];
  int len = sizeof(struct sockaddr_in), ind = 0;
  while(1) {
    int *sock = malloc(sizeof(int));
    struct sockaddr_in addr;
    *sock = accept(master_sock, (struct sockaddr*) &addr, &len);
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
    printf("[SERVER]: Incoming flag: %d\n", flag->flag);
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
  kdb = ht_create(DEFAULT_SIZE);
  cdb = ht_create(DEFAULT_SIZE);
  bldb = ht_create(DEFAULT_SIZE / 4);
  add_my_files(kdb);
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
