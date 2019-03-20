typedef struct node_t {
  char name[20];
  char ip_port[30];
} node;

typedef struct ping_message {
  node self;
  node* array_of_known_nodes;
  int array_size;
  char client_rqst[1024];
  char server_rspn[1024];
} ping_msg;
