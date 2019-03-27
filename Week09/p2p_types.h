typedef struct {
  char name[1024];
  char ip_port[1024];
} node;

typedef struct {
  int size;
  node nodes[1024];
} peers_list;
