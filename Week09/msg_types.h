typedef struct {
  int flag;
} pcl_flag;

typedef struct {
  char data[1024];
} sync_data;

typedef struct {
  int n;
} num_t;

typedef struct {
  char data[1024];
} sync_peer;

typedef struct {
  char filename[1024];
} file_rqst;

typedef struct {
  char val[1024];
} word;

typedef struct {
  char* words[1024];
  int size;
} text;
