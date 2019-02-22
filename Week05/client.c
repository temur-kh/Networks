#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <memory.h>
#include "common.h"

#define DEST_PORT            2000
#define SERVER_IP_ADDRESS   "192.168.1.2"

test_struct_t client_data;
result_struct_t result;

void setup_udp_communication() {
    /*Step 1 : Initialization*/
    /*Socket handle*/
    int sockfd = 0,
        sent_recv_bytes = 0;

    int addr_len = 0;

    addr_len = sizeof(struct sockaddr);

    /*to store socket addesses : ip address and port*/
    struct sockaddr_in dest;

    /*Step 2: specify server information*/
    /*Ipv4 sockets, Other values are IPv6*/
    dest.sin_family = AF_INET;

    /*Client wants to send data to server process which is running on server machine, and listening on
     * port on DEST_PORT, server IP address SERVER_IP_ADDRESS.
     * Inform client about which server to send data to : All we need is port number, and server ip address. Pls note that
     * there can be many processes running on the server listening on different no of ports,
     * our client is interested in sending data to server process which is lisetning on PORT = DEST_PORT*/
    dest.sin_port = DEST_PORT;
    struct hostent *host = (struct hostent *)gethostbyname(SERVER_IP_ADDRESS);
    dest.sin_addr = *((struct in_addr *)host->h_addr);

    /*Step 3 : Create a UDP socket*/
    /*Create a socket finally. socket() is a system call, which asks for three paramemeters*/
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    while(1) {

	    /*Prompt the user to enter data*/
	    /*You will want to change the promt for the second task*/
	    printf("Enter name : ");
	    scanf("%s", client_data.name);
	    printf("Enter age : ");
	    scanf("%u", &client_data.age);
      printf("Enter group : ");
	    scanf("%s", client_data.group);

	    /*step 4 : send the data to server*/
	    sent_recv_bytes = sendto(sockfd,
		   &client_data,
		   sizeof(test_struct_t),
		   0,
		   (struct sockaddr *)&dest,
		   sizeof(struct sockaddr));

	    printf("No of bytes sent = %d\n", sent_recv_bytes);

	    /*Step 5 : Client also wants a reply from server after sending data*/
	    sent_recv_bytes =  recvfrom(sockfd, (char *)&result, sizeof(result_struct_t), 0,
		            (struct sockaddr *)&dest, &addr_len);

	    printf("No of bytes received = %d\n", sent_recv_bytes);

      if (sent_recv_bytes != 0) {
        printf("Result received = %s, %d, %s\n", result.name, result.age, result.group);
      }
    }
}


int main(int argc, char **argv) {
    setup_udp_communication();
    printf("application quits\n");
    return 0;
}
