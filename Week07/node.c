#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <memory.h>
#include <errno.h>
// #include <zconf.h>
#include "common.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define DEST_PORT            2000
// #define SERVER_IP_ADDRESS   "192.168.1.2"
#define SERVER_PORT     2000

test_struct_t test_struct;
result_struct_t res_struct;
char data_buffer[1024];
test_struct_t client_data;
result_struct_t result;

void* setup_tcp_client_communication(void* arg) {
    strcpy(client_data.name, "Temur");  //part of my protocol
    client_data.age = 20;
    strcpy(client_data.group, "B17-05");

    int sockfd = 0, sent_recv_bytes = 0;
    int addr_len = 0;
    addr_len = sizeof(struct sockaddr);
    struct sockaddr_in dest;



    while(1) {
      char server_address[20];

      printf("[CLIENT] Enter server address: \n");
	    scanf("%s", server_address);

      dest.sin_family = AF_INET;
      dest.sin_port = DEST_PORT;
      struct hostent *host = (struct hostent *)gethostbyname(server_address);
      dest.sin_addr = *((struct in_addr *)host->h_addr);

      sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

      connect(sockfd, (struct sockaddr *)&dest,sizeof(struct sockaddr));

	    sent_recv_bytes = sendto(sockfd,
		   &client_data,
		   sizeof(test_struct_t),
		   0,
		   (struct sockaddr *)&dest,
		   sizeof(struct sockaddr));

	    printf("[CLIENT] No of bytes sent = %d\n", sent_recv_bytes);

	    sent_recv_bytes =  recvfrom(sockfd, (char *)&result, sizeof(result_struct_t), 0,
		            (struct sockaddr *)&dest, &addr_len);

	    printf("[CLIENT] No of bytes received = %d\n", sent_recv_bytes);

      if (sent_recv_bytes != 0) {
        printf("[CLIENT] Result received = %s, %d, %s\n", result.name, result.age, result.group);
      }
    }
}

void setup_tcp_server_communication() {
    int master_sock_tcp_fd = 0,
            sent_recv_bytes = 0,
            addr_len = 0,
            opt = 1;
    int comm_socket_fd = 0;
    fd_set readfds;
    struct sockaddr_in server_addr, client_addr;

    if ((master_sock_tcp_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("[SERVER] socket creation failed\n");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = SERVER_PORT;

    server_addr.sin_addr.s_addr = INADDR_ANY;

    addr_len = sizeof(struct sockaddr);

    if (bind(master_sock_tcp_fd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr)) == -1) {
        printf("[SERVER] socket bind failed\n");
        return;
    }

    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(master_sock_tcp_fd, (struct sockaddr *)&sin, &len) == -1)
        perror("[SERVER] getsockname");
    else
        printf("[SERVER] port number %d\n", ntohs(sin.sin_port));

    if (listen(master_sock_tcp_fd, 5) < 0) {
        printf("[SERVER] listen failed\n");
        return;
    }

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(master_sock_tcp_fd, &readfds);

        printf("[SERVER] blocked on select System call...\n");

        select(master_sock_tcp_fd + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(master_sock_tcp_fd, &readfds)) {
            printf("[SERVER] New connection recieved recvd, accept the connection. Client and Server completes TCP-3 way handshake at this point\n");
            comm_socket_fd = accept(master_sock_tcp_fd, (struct sockaddr *) &client_addr, &addr_len);
            if (comm_socket_fd < 0) {
                printf("[SERVER] accept error : errno = %d\n", errno);
                exit(0);
            }

            printf("[SERVER] Connection accepted from client : %s:%u\n",
                   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            while (1) {
                printf("[SERVER] Server ready to service client msgs.\n");
                memset(data_buffer, 0, sizeof(data_buffer));

                sent_recv_bytes = recvfrom(comm_socket_fd, (char *) data_buffer, sizeof(data_buffer), 0,
                                           (struct sockaddr *) &client_addr, &addr_len);

                printf("[SERVER] Server recvd %d bytes from client %s:%u\n", sent_recv_bytes,
                       inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

                if (sent_recv_bytes == 0) {
                    close(comm_socket_fd);
                    break;
                }

                test_struct_t *client_data = (test_struct_t *) data_buffer;

                if (client_data->age == 0) {
                    close(comm_socket_fd);
                    printf("[SERVER] Server closes connection with client : %s:%u\n", inet_ntoa(client_addr.sin_addr),
                           ntohs(client_addr.sin_port));
                    break;
                }

                result_struct_t result;  // part of my protocol
                strcpy(result.name, "Kholmatov");
                result.age = 20;
                strcpy(result.group, "B17-05");

                sent_recv_bytes = sendto(comm_socket_fd, (char *) &result, sizeof(result_struct_t), 0,
                                         (struct sockaddr *) &client_addr, sizeof(struct sockaddr));

                printf("[SERVER] Server sent %d bytes in reply to client\n", sent_recv_bytes);
            }
        }
    }
}

int main(int argc, char **argv) {
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, setup_tcp_client_communication, NULL);
    setup_tcp_server_communication();
    pthread_join(thread_id, NULL);
    return 0;
}
