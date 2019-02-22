//Taken from Abhishek Sagar

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <memory.h>
#include <errno.h>
#include <zconf.h>
#include "common.h"
#include <arpa/inet.h>
#include <pthread.h>

/*Server process is running on this port no. Client has to send data to this port no*/
#define SERVER_PORT 2000
#define THREADS_COUNT 10

test_struct_t test_struct;
result_struct_t res_struct;
char data_buffer[1024];
worker_struct_t threads[THREADS_COUNT];

void *worker(void *num) {
  int i = *((int *)num);
  worker_struct_t *data = (worker_struct_t *) &threads[i];
  test_struct_t *client_data = &(data->client_data);
  struct sockaddr_in *client_addr = data->client_addr;
  int sock_udp_fd = data->sock_udp_fd;
  printf("Thread #%d received = %s, %d, %s\n", data->thread_num, client_data->name, client_data->age, client_data->group);
  /* If the client sends a special msg to server, then server close the client connection
   * for forever*/
  if (client_data->age == 0) {
      close(sock_udp_fd);
      printf("Server closes connection\n");
      exit(0);
  }

  result_struct_t result;
  strcpy(result.name, client_data->name);
  result.age = client_data->age;
  strcpy(result.group, client_data->group);

  /* Server replying back to client now*/
  int sent_recv_bytes = sendto(sock_udp_fd, (char *) &result, sizeof(result_struct_t), 0,
                           (struct sockaddr *)client_addr, sizeof(struct sockaddr));

  printf("Thread sent %d bytes in reply to client\n", sent_recv_bytes);
  free(threads[i].client_addr);
}

void setup_udp_server_communication() {

    /*Step 1 : Initialization*/
    /*Socket handle and other variables*/
    /*Master socket file descriptor, used to accept new client connection only, no data exchange*/
    int master_sock_udp_fd = 0,
            sent_recv_bytes = 0,
            addr_len = 0,
            opt = 1;

    /* Set of file descriptor on which select() polls. Select() unblocks whever data arrives on
     * any fd present in this set*/
    fd_set readfds;
    /*variables to hold server information*/
    struct sockaddr_in server_addr;

    /*step 2: udp master socket creation*/
    if ((master_sock_udp_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        printf("socket creation failed\n");
        exit(1);
    }

    /*Step 3: specify server Information*/
    server_addr.sin_family = AF_INET;/*This socket will process only ipv4 network packets*/
    server_addr.sin_port = SERVER_PORT;/*Server will process any data arriving on port no 2000*/

    /*3232249957; //( = 192.168.56.101); Server's IP address,
    //means, Linux will send all data whose destination address = address of any local interface
    //of this machine, in this case it is 192.168.56.101*/

    server_addr.sin_addr.s_addr = INADDR_ANY;

    addr_len = sizeof(struct sockaddr);

    /* Bind the server. Binding means, we are telling kernel(OS) that any data
     * you recieve with dest ip address = 192.168.56.101, and udp port no = 2000, pls send that data to this process
     * bind() is a mechnism to tell OS what kind of data server process is interested in to recieve. Remember, server machine
     * can run multiple server processes to process different data and service different clients. Note that, bind() is
     * used on server side, not on client side*/

    if (bind(master_sock_udp_fd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr)) == -1) {
        printf("socket bind failed\n");
        return;
    }

    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(master_sock_udp_fd, (struct sockaddr *)&sin, &len) == -1)
        perror("getsockname");
    else
        printf("port number %d\n", ntohs(sin.sin_port));


    int i = 0;


    /* Server infinite loop for servicing the client*/
    while (1) {
        printf("Server ready to service client msgs.\n");
        /*Drain to store client info (ip and port) when data arrives from client, sometimes, server would want to find the identity of the client sending msgs*/
        memset(data_buffer, 0, sizeof(data_buffer));

        /*Step 4: Server recieving the data from client. Client IP and PORT no will be stored in client_addr
         * by the kernel. Server will use this client_addr info to reply back to client*/

        /*Like in client case, this is also a blocking system call, meaning, server process halts here untill
         * data arrives on this master_sock_udp_fd from client whose connection request has been accepted via accept()*/
        threads[i].client_addr = malloc(sizeof(struct sockaddr_in));
        sent_recv_bytes = recvfrom(master_sock_udp_fd, (char *) data_buffer, sizeof(data_buffer), 0,
                                   (struct sockaddr *) threads[i].client_addr, &addr_len);
        printf("Server recvd %d bytes from client %s:%u\n", sent_recv_bytes,
               inet_ntoa(threads[i].client_addr->sin_addr), ntohs(threads[i].client_addr->sin_port));

        if (sent_recv_bytes == 0) {
            close(master_sock_udp_fd);
            break;
        }

        test_struct_t* tmp_client_data = (test_struct_t*)data_buffer;

        threads[i].thread_num = i;
        threads[i].sock_udp_fd = master_sock_udp_fd;
        threads[i].client_data = *tmp_client_data;

        if (pthread_create(&(threads[i].thread_id), NULL, worker, (void *)&(threads[i].thread_num)) != 0) {
          printf("thread creation failed\n");
          exit(1);
        }
        i = (i+1) < THREADS_COUNT ? i+1 : 0;
    }
}

int main(int argc, char **argv) {
    setup_udp_server_communication();
    return 0;
}
