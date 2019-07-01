peers_list* create_peers() {
  peers_list* peers = malloc(sizeof(peers_list));
  return peers;
}

int contains(peers_list *peers, char* name, char* ip_port) {
  for (int i=0; i<peers->size; i++) {
    node cur = peers->nodes[i];
    if (!strcmp(name, cur.name) && !strcmp(ip_port, cur.ip_port)) {
      return -1;  // found
    } else if (!strcmp(ip_port, cur.ip_port)) {
      return i;  // ip_port is found
    }
  }
  return peers->size;  // not found
}

int insert_peer(peers_list *peers, char* name, char* ip_port) {
  if (peers == NULL || ip_port == NULL) return -1;  // invalid arguments
  int st = contains(peers, name, ip_port);
  if (st == peers->size) {  // insert if not exists
    strcpy(peers->nodes[peers->size].name, name);
    strcpy(peers->nodes[peers->size].ip_port, ip_port);
    (peers->size)++;
    return 2;
  } else if (st == -1) {  // do not insert
    return 0;
  } else {  // update the name of the existing node
    strcpy(peers->nodes[peers->size].name, name);
    return 1;
  }
}

int remove_peer(peers_list *peers, node nd) {
  int st = contains(peers, nd.name, nd.ip_port);
  if (st == peers->size) return -1;
  else {
    int mv_left = 0;
    for (int i=0; i<peers->size; i++) {
      node cur = peers->nodes[i];
      if (mv_left) {
        peers->nodes[i-1] = peers->nodes[i];
      }
      if (!mv_left && !strcmp(nd.ip_port, cur.ip_port)) {
        mv_left = 1;
      }
    }
    peers->nodes[peers->size-1] = (node) {0};
    (peers->size)--;
  }
}
