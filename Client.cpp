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
#include <iostream>
using namespace std;

#define BUFFER_SIZE 1024
//#define PORT_NUMBER 5437
char *IP_AD;
int PORT;


bool isValidNum(string str) {
    for (int i = 0; i < str.length(); i++) {
        if (! isdigit(str[i])) {
            return false;
        }
    }

    try {
        if (stoi(str) <= 0)
            return false;
    }
    catch (invalid_argument& e) {
        return false;
    }

    return true;
}


bool isValid(char** args){
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, args[1], &(sa.sin_addr));

    if (result == 0) {
        printf("\nIP address entered is invalid\n\n");
        return false;
    }
    else if(result < 0){
        printf("\n\nIP address entered is not part of a valid address family\n\n");
        return false;
    }
    else if(!isValidNum(args[2]) || stoi(string(args[2])) <= 0){
        printf("\n\nPort is not a valid number\n\n");
        return false;
    }
    else if(!isValidNum(args[3]) || stoi(string(args[3])) <= 0){
        printf("\n\nNumber not given for amount of burgers client can eat\n\n");
        return false;
    }
    return true;
}



int main(int argc, char *argv[])
{
     if (argc != 4)
    {
        printf("\n Usage: %s <ip of server> <port number> <amount of burgers the client can eat> \n", argv[0]);
        return 1;
    }
    if (!isValid(argv)){
        return 1;
    }
    int sockfd = 0, n = 0;

    char recvBuff[BUFFER_SIZE];
    struct sockaddr_in serv_addr;
    pid_t clientPID = getpid();
    PORT = stoi(string(argv[2]));

    IP_AD = argv[1];
   
   
    memset(recvBuff, '0', sizeof(recvBuff));
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

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
            printf("\n[Client #%d] Connection to server established at %s:%d\n", clientPID, IP_AD, PORT);
            printf("est");
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
