// KIRSTEN BOYLES LAB 4 470

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <math.h>
#include <iostream>
#include <atomic>
#include <queue>
#include <pthread.h>
#include <time.h>
using namespace std;

#define THREAD_POOL 20 // amount of parallel clients allowed
#define MAX_BUFF 1024

int PORT_NUMBER;
int MAX_BURGERS;
int burgersLeft;
int CHEFS;

pthread_t workerThreads[THREAD_POOL];

atomic<int> burgersGone;
pthread_mutex_t queueLock;
pthread_mutex_t chefLock;
pthread_cond_t waitCondition = PTHREAD_COND_INITIALIZER;
queue<int> pool;

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

bool isValid(int argc, char **args)
{
        for (int i = 1; i < argc; i++)
        {
                if (!isValidNum(string(args[i])))
                {
                        printf("\nAll arguemnts must be numbers, an argument provided didn't have a number. Retry\n\n");
                        return false;
                }
        }
        PORT_NUMBER = stoi(string(args[1]));
        if (argc == 4)
        {
                MAX_BURGERS = stoi(args[2]);
                CHEFS = stoi(args[3]);
        }
        return true;
}

void printBurgers()
{
        printf("\n\nHow many burgers are left: %d\n\n", burgersLeft);
        return;
}

void handleRequests(int connfd)
{
        char statusBuff[MAX_BUFF];
        int new_socket;
        pid_t clientPID = -1;

        // recieve clientPID

        int gotPID = read(connfd, &clientPID, sizeof(clientPID));
        if (gotPID = 0)
        {
                printf("\nSERVER - Connection to the client was lost\n");
                return;
        }
        if (gotPID < 0)
        {
                fprintf(stderr, "SERVER - Error: failed to retrieve PID from the client\n%d\n", errno);
                return;
        }

        printf("\n\nSERVER - A connection has been established with [Client# %d]", clientPID);
        printf("\nSERVER - Current number of burgers available is %d\n", burgersLeft);
        while (burgersGone == 0)
        {

                // send burger total at shop
                memset(statusBuff, 0, sizeof(statusBuff));

                sprintf(statusBuff, "%d", burgersLeft);
                send(connfd, statusBuff, strlen(statusBuff), 0);

                if (burgersLeft == 0)
                {
                        burgersGone = 1;
                        break;
                }

                memset(statusBuff, 0, sizeof(statusBuff));
                // recieve request for burger

                int valread = read(connfd, statusBuff, sizeof(statusBuff));

                statusBuff[valread] = 0;
                // printf("%s\n", statusBuff);
                int burger = stoi(statusBuff);

                printf("\n[Client #%d] is requesting a burger, they will want %d burgers\n", clientPID, burger);

                pthread_mutex_lock(&chefLock);
                srand(time(NULL));
                burgersLeft--;
                pthread_mutex_unlock(&chefLock);

                int wait = rand() % 4 + 3;
                sleep(wait);

                memset(statusBuff, 0, sizeof(statusBuff));
                sprintf(statusBuff, "\nSERVER - Here is your burger!\n");
                send(connfd, statusBuff, strlen(statusBuff), 0);

                int ACK = 0;
                int n = read(connfd, &ACK, sizeof(ACK));

                if (n == 0)
                {
                        printf("\n[Server] Connection to [Client #%d] was lost...\n", clientPID);
                        return;
                }
                if (n < 0)
                {
                        fprintf(stderr, "[Server] Error: couldn't recieve the client ACK\n%s\n", strerror(errno));
                        return;
                }
                if (ACK != 1)
                {
                        printf("[Server] Error: failed to recieve an ACK\n");
                        return;
                }

                printf("[Server] Burger was eaten by client\n");
                if (burger == 1)
                {
                        break;
                }
        }

        return;
}

void *workerThreadFunc(void *arg)
{
        while (burgersGone == 0)
        {
                int p_connfd = -1;
                if (burgersGone == 1)
                {
                        break;
                }
                // mutex locking to no two threads can dequeue at the same time
                // the conditional wait lets threads sleep until a new job enteres the queue
                pthread_mutex_lock(&queueLock);
                // if there no work in the queue, the threads wait
                if (pool.empty())
                {
                        pthread_cond_wait(&waitCondition, &queueLock);
                        p_connfd = pool.front();
                        pool.pop();
                }
                // otherwise, they pull from the queue and work
                else
                {
                        p_connfd = pool.front();
                        pool.pop();
                }
                pthread_mutex_unlock(&queueLock);

                // if a job can be pulled from the queue, then the thread will handle it
                if (p_connfd != -1)
                {
                        handleRequests(p_connfd);
                        if (close(p_connfd) < 0)
                        {
                                fprintf(stderr, "\n\nSERVER - ERROR:  failed to close file descriptor for client socket connection\n\n%s\n\n", strerror(errno));
                        }
                }
        }
        printf("There are no more burgers, connection will now close\n");
        return NULL;
}

int main(int argc, char *argv[])
{
        if (argc != 4)
        {
                printf("\n\nUsage: %s <port number> <max number of burgers available> <max number of chefs> \n\n", argv[0]);
                return 1;
        }
        if (!isValid(argc, argv))
        {
                return 1;
        }

        int listenfd = 0, connfd = 0;
        struct sockaddr_in serv_addr;

        char sendBuff[MAX_BUFF];

        listenfd = socket(AF_INET, SOCK_STREAM, 0);
        if (listenfd < 0)
        {
                fprintf(stderr, "SERVER - ERROR: %s\n", strerror(errno));
        }

        int enable = 1;
        if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)))
        {
                fprintf(stderr, "\nSERVER - ERROR: %s\n", strerror(errno));
        }

#ifdef SO_REUSEPORT

        if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable)))
        {
                fprintf(stderr, "\nSERVER - ERROR: %s\n", strerror(errno));
        }
#endif

        memset(&serv_addr, '0', sizeof(serv_addr));
        memset(sendBuff, '0', sizeof(sendBuff));

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr.sin_port = htons(PORT_NUMBER);

        if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
                fprintf(stderr, "\nSERVER - ERROR %s\n", strerror(errno));
        }

        if (listen(listenfd, 10) < 0)
        {
                fprintf(stderr, "\nSERVER - ERROR %s\n", strerror(errno));
        }

        printf("\n\nStarting server on port %d ...\n\n", PORT_NUMBER);
        printf("[~~~~~~~~~~~~~~~ SERVER LIVE ~~~~~~~~~~~~~~~]\n\n");

        // starting the threadpool handling clients
        for (int i = 0; i < CHEFS; i++)
        {
                pthread_create(&workerThreads[i], NULL, workerThreadFunc, NULL);
        }

        burgersLeft = MAX_BURGERS;

        while (burgersGone == 0)
        {
                printBurgers();

                if (burgersLeft == 0 || burgersGone == 1)
                {
                        printf("\nSERVER - All of the burgers at the resteraunt have been eaten for the day.\n\n");
                        burgersGone = 1;

                        break;
                }

                connfd = accept(listenfd, (struct sockaddr *)NULL, NULL);
                if (connfd < 0)
                {
                        fprintf(stderr, "SERVER - ERROR: %s\n", strerror(errno));
                        continue;
                }

                // adds the new socket client to the work queue so the threads can grab it
                // mutex lock the threads so no two threadds can enqueue at the same time
                // signal condition notifies the threads that are sleeping that there is work
                pthread_mutex_lock(&queueLock);
                pool.push(connfd);
                pthread_cond_signal(&waitCondition);
                pthread_mutex_unlock(&queueLock);
        }

        printf("test");

        for (int i = 0; i < CHEFS; i++)
        {
                pthread_join(workerThreads[i], NULL);
        }

        printf("\n\n~~~~~~~~~ SERVER GOING OFFLINE ~~~~~~~~~\n\n");

        close(connfd);

        return 0;
}
