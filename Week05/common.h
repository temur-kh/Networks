#include <sys/socket.h>

typedef struct _test_struct{

    char name[20];
    unsigned int age;
    char group[10];

} test_struct_t;


typedef struct result_struct_{

  char name[20];
  unsigned int age;
  char group[10];

} result_struct_t;

typedef struct worker_struct_{
  pthread_t thread_id;
  int thread_num;
  int sock_udp_fd;
  test_struct_t client_data;
  struct sockaddr_in *client_addr;
} worker_struct_t;
