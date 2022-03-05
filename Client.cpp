// KIRSTEN BOYLES LAB 4 470

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
int burger;

bool isValidNum(string str)
{
    for (int i = 0; i < str.length(); i++)
    {
        if (!isdigit(str[i]))
        {
            return false;
        }
    }

    try
    {
        if (stoi(str) <= 0)
            return false;
    }
    catch (invalid_argument &e)
    {
        return false;
    }

    return true;
}

bool isValid(char **args)
{
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, args[1], &(sa.sin_addr));

    if (result == 0)
    {
        printf("\nIP address entered is invalid\n\n");
        return false;
    }
    else if (result < 0)
    {
        printf("\n\nIP address entered is not part of a valid address family\n\n");
        return false;
    }
    else if (!isValidNum(args[2]) || stoi(string(args[2])) <= 0)
    {
        printf("\n\nPort is not a valid number\n\n");
        return false;
    }
    else if (!isValidNum(args[3]) || stoi(string(args[3])) <= 0)
    {
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
    if (!isValid(argv))
    {
        return 1;
    }
    // connecting to server
    int sockfd = 0, n = 0;

    char recvBuff[BUFFER_SIZE];
    struct sockaddr_in serv_addr;
    pid_t clientPID = getpid();
    PORT = stoi(string(argv[2]));
    burger = stoi(argv[3]);

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

    // 4 connection attempts with 3 seconds between each interval. exits after the 4th attempt
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
            break;
        }
    }

    // sending the client pid to the server
    send(sockfd, &clientPID, sizeof(clientPID), 0);

    //

    while (burger != 0)
    {

        // char statusBuff[BUFFER_SIZE];
        //  recieve burger total from shop
        memset(recvBuff, '0', sizeof(recvBuff));

        n = read(sockfd, recvBuff, sizeof(recvBuff));
        recvBuff[n] = 0;
        if (!isValidNum(recvBuff))
        {
            printf("\nNo more burgers left :(\n");
            break;
        }
        

        printf("\n\n[Client #%d] There are %s burgers left.\n", clientPID, recvBuff);
        // send request for burger

        memset(recvBuff, '0', sizeof(recvBuff));

        sprintf(recvBuff, "%d", burger);

        send(sockfd, recvBuff, sizeof(recvBuff), 0);
        printf("\n[Client #%d] Request for burger sent.\n", clientPID);

        memset(recvBuff, '0', sizeof(recvBuff));
        n = read(sockfd, recvBuff, sizeof(recvBuff));
        if (n = 0)
        {
            printf("\nSERVER - Connection to the client was lost\n");
            return 1;
        }
        if (n < 0)
        {
            fprintf(stderr, "SERVER - Error: failed to retrieve PID from the client\n%d\n", errno);
            return 1;
        }
        recvBuff[n] = 0;
        printf("%s\n", recvBuff);

        srand(time(NULL));
        int wait = rand() % 5 + 4;
        sleep(wait);

        memset(recvBuff, '0', sizeof(recvBuff));

        int ACK = 1;
        send(sockfd, &ACK, sizeof(ACK), 0);

        burger--;
        printf("\n[Client #%d] I ate my burger, I want %d more burger(s).\n", clientPID, burger);
    }

    close(sockfd);
    return 0;
}
