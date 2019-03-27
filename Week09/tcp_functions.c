int socket_create(){
  int sock;
  sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  return sock;
}

int socket_connect(int sock, struct sockaddr_in* remote){
  int res=-1;
  res = connect(sock, (struct sockaddr*) remote, sizeof(struct sockaddr));
  return res;
}

int bind_socket(int master_sock){
  int res=-1;
  struct sockaddr_in remote={0};
  remote.sin_family = AF_INET; /* Internet address family */
  remote.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
  remote.sin_port = htons(MY_PORT); /* Local port */
  res = bind(master_sock,(struct sockaddr *) &remote, sizeof(remote));
  return res;
}

int socket_send(int sock, char* msg, int len) {
  int res = -1;
  struct timeval tv;
  tv.tv_sec = 20; /* 20 Secs Timeout */
  tv.tv_usec = 0;
  if(setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv,sizeof(tv)) < 0) {
    printf("[SOCKET SEND]: [ERROR]: Time Out\n");
    return -1;
  }
  res = send(sock, msg, len, 0);
  return res;
}

int socket_rcv(int sock, char* msg, int len) {
  int res = -1;
  struct timeval tv;
  tv.tv_sec = 20; /* 20 Secs Timeout */
  tv.tv_usec = 0;
  if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(tv)) < 0) {
    printf("[SOCKET RCV]: [ERROR]: Time Out\n");
    return -1;
  }
  res = recv(sock, msg, len, 0);
  return res;
}
