#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024
#define PORT_NUMBER 5437
char *IP_AD;

int main(int argc, char *argv[])
{
    int sockfd = 0, n = 0;

    char recvBuff[BUFFER_SIZE];
    struct sockaddr_in serv_addr;
    pid_t clientPID = getpid();

    if (argc != 2)
    {
        printf("\n Usage: %s <ip of server> \n", argv[0]);
        return 1;
    }

    memset(recvBuff, '0', sizeof(recvBuff));
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT_NUMBER);

    if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0)
    {
        printf("\n inet_pton error occured\n");
        return 1;
    }

    for (int i = 0; i < 4; i++)
    {
        if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
            fprintf(stderr, "[Client #%d] Error: %s\n", clientPID, strerror(errno));
            if (i == 3)
            {
                printf("\n4 Attempts have been made to connect and all failed. Make sure server is online. \n");
                return 1;
            }
            printf("[Client #%d] Retrying connection in 3 seconds...\n\n", clientPID);
            sleep(3);
        }
        else
        {
            printf("\n[Client #%d] Connection to server established at %s:%d\n", clientPID, IP_AD, PORT_NUMBER);
            break;
        }
    }

    if (write(sockfd, &clientPID, sizeof(clientPID)) < 0)
    {
        fprintf(stderr, "[Client #%d] Error: failed to send pid to server\n%s\n", clientPID, strerror(errno));
    }
    int ACK = 1;
    if(write(sockfd, &ACK, sizeof(ACK)) < 0){
        fprintf(stderr, "[Client #%d] Error: failed to send ACK for welcome message to server\n%s\n", clientPID, strerror(errno));
    }


    while ((n = read(sockfd, recvBuff, sizeof(recvBuff) - 1)) > 0)
    {
        recvBuff[n] = 0;
        if (fputs(recvBuff, stdout) == EOF)
        {
            printf("\n Error : Fputs error\n");
        }
    }

    if (n < 0)
    {
        printf("\n Read error \n");
    }

    close(sockfd);
    return 0;
}
