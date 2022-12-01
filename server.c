#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<string.h>
#include<errno.h>
#include<unistd.h>
#include<stdarg.h>
#include<pthread.h>

//Mariah Bray, Dakota Sliter, and Selmir lelak

#define MAXLINE 4096 /*max text line length*/
#define SERV_PORT 3000 /*port*/
#define LISTENQ 8 /*maximum number of client connections*/

#define READ 0
#define WRITE 1

typedef struct{
    int flag[2];
    int turn;
    int client_count;
    int current_count;
}permissions;
//handles shared memory vital to control critical sections

typedef struct{
    char str[MAXLINE];
    pid_t pid;
    int connf;
}data;
//Handles data from the client

void *to_client(void *arg);
void *from_client(void *arg);

bool canrun = true;
permissions *p;
data *d;

int main(int argc, char **argv) {
  
    int listenfd, connfd, n;
    pid_t childpid;
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;
    pthread_t thread1;     //thread for to_client
    pthread_t thread2;     //thread for from_client
    int thr1;
    int thr2;

    bool canrun = true;
    int data_id;
    int perm_id;

    // ftok to generate unique key
    key_t key1 = ftok("server.c",1);
    key_t key2 = ftok("server.c",2);

    // Attempt to create shared memory
    data_id = shmget(key1,sizeof(data), 0660 | IPC_CREAT);
    perm_id = shmget(key2,sizeof(permissions), 0660 | IPC_CREAT);

    if (data_id == -1 || perm_id == -1) {
        perror("Shared memory");
        exit(1);
    }

    d = shmat(data_id, NULL, 0);
    p = shmat(perm_id, NULL, 0);

    if (p == (void *) -1 || d == (void *) -1) {
        perror("Shared memory attach");
        exit(1);
    }

    //Set up starting values
    p->flag[READ] = false;
    p->flag[WRITE] = true;
    p->turn = 0;
    p->client_count = 0;
    p->current_count = 0;
    d->pid = 0;

    //Create a socket for the server
    //If sockfd<0 there was an error in the creation of the socket
    if ((listenfd = socket (AF_INET, SOCK_STREAM, 0)) <0) {
        perror("Problem in creating the socket");
        exit(2);
    }

    //preparation of the socket address
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);

    //bind the socket
    bind (listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

    //listen to the socket by creating a connection queue, then wait for clients
    listen (listenfd, LISTENQ);

    printf("%s\n","Server running...waiting for connections.");

    // Weird syntax for infinite loop
    for ( ; ; ) {
        // Sets length of client IP address
        clilen = sizeof(cliaddr);
        //accept a connection
        connfd = accept (listenfd, (struct sockaddr *) &cliaddr, &clilen);
        
        //get socket descriptor
        d->connf = connfd;

        printf("%s\n","Received request...");

        if ( (childpid = fork ()) == 0 ) {//if it's 0, it's the child process

            printf ("%s\n","Child created for dealing with client requests");
        
            ++p->client_count;
        
            //close listening socket
            close (listenfd);

            
            if ((thr1 = pthread_create(&thread1, NULL, to_client, NULL)) != 0) {
                fprintf(stderr, "Thread create error %d: %s\n", thr1, strerror(thr1));
                exit(1);
            }
            pthread_detach(thread1);
            if ((thr2 = pthread_create(&thread2, NULL, from_client, NULL)) != 0) {
                fprintf(stderr, "Thread create error %d: %s\n", thr2, strerror(thr2));
                exit(1);
            }
        
            //Wait until thread two is done to continue
            pthread_join(thread2,NULL);

            printf("User %d has left\n",getpid());

            exit(0);
        }

        //close socket of the server
        close(connfd);
        
    }

        //detach shared memory
    if (shmdt(d) == -1 || shmdt(p) == -1) {
        perror("shmdt");
        return 1;
    }
        //remove shared memory
    if (shmctl(data_id, IPC_RMID, 0) == -1 || shmctl(perm_id, IPC_RMID, 0) == -1) {
        perror("shmctl");
        return 1;
    }
    
    return 0;
}



//Sends messages to client 
void *to_client(void *arg){
    while(canrun){
        if(p->flag[READ]){
            //critical section 
            if(d->pid != getpid()){
                //Convert string to add client pid
                ssize_t bufsz = snprintf(NULL, 0, "User %d: %s\n", d->pid, d->str);
                char* message = malloc(bufsz);
                memset(message,0,bufsz);
                snprintf(message, bufsz, "User %d: %s\n", d->pid, d->str);
                
                //send string to client
                send(d->connf, message, MAXLINE + 100, 0);
                free(message);
   
            }  
     
            ++p->current_count;
            //Check if message has been sent to all clients
            while(p->current_count < p->client_count){
                sleep(1);
            }
            p->flag[READ] = false;
            sleep(1);
            //Turn complete
            if(d->pid == getpid()){
                --p->turn;
            }
            //End of critical section 
            //Allow more writes if all turns are done
            if(p->turn == 0){
                p->flag[WRITE] = true;
            }
            
        }
    }
    pthread_exit(NULL);
}

//the server gets data from the client
void *from_client(void *arg){
    while(canrun){
        if(p->flag[WRITE]){
            int turn;
            char buf[MAXLINE];
            memset(buf, 0, MAXLINE);

            if( recv(d->connf, buf, MAXLINE,0) > 0){
                //critical section 
                p->flag[WRITE] = false;

                ++p->turn;
                turn = p->turn;

                if(strcmp(buf,"quit\n")==0){
                    //strcpy(buf,"User left\n");
                    --p->client_count;
                    canrun = false;
                }
                else{
                    //Multiple threads may enter this section when write is true
                    //The threads will get stuck here in turns so written data isn't lost
                    while(p->turn!=turn){
                        sleep(1);
                    }
                    //The Start of a turn which will complete in to_client
                    p->current_count = 0;
                    //read data to shared memory
                    memset(d->str,0,MAXLINE);
                    strcpy(d->str,buf);
                    printf("%s","String received from and sent to clients:");
                    puts(buf);
                    d->pid = getpid();
                    //End of critical section  
                    p->flag[READ] = true;
                }
            }

        }
    }
    pthread_exit(NULL);
}
