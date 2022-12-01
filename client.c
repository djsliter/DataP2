#include <arpa/inet.h>
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

#define MAXLINE 4196 /*max text line length*/
#define SERV_PORT 3000 /*port*/

int sockfd;
char sendline[MAXLINE], recvline[MAXLINE];
bool canrun = true;

void *do_read(void *arg);
void *do_write(void *arg);

int main(int argc, char **argv) {
    
    pthread_t thread1;     //thread for reader
    pthread_t thread2;     //thread for writer
    int thr1;
    int thr2;
    struct sockaddr_in servaddr;
    
        
    //basic check of the arguments
    //additional checks can be inserted
    if (argc !=2) {
        perror("Usage: TCPClient <IP address of the server"); 
        exit(1);
    }
        
    //Create a socket for the client
    //If sockfd<0 there was an error in the creation of the socket
    if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) <0) {
        perror("Problem in creating the socket");
        exit(2);
    }
        
    //Creation of the socket
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr= inet_addr(argv[1]);
    servaddr.sin_port =  htons(SERV_PORT); //convert to big-endian order
        
    //Connection of the client to the socket 
    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))<0) {
        perror("Problem in connecting to the server");
        exit(3);
    }
        
    
    if ((thr1 = pthread_create(&thread1, NULL, do_read, NULL)) != 0) {
        fprintf(stderr, "Thread create error %d: %s\n", thr1, strerror(thr1));
        exit(1);
    }
    pthread_detach(thread1);
    if ((thr2 = pthread_create(&thread2, NULL, do_write, NULL)) != 0) {
        fprintf(stderr, "Thread create error %d: %s\n", thr2, strerror(thr2));
        exit(1);
    }

    //wait for thread two to complete
    pthread_join(thread2,NULL);

    exit(0);
}

//read takes data from server and prints
void *do_read(void *arg){
    while(canrun){
        if (recv(sockfd, recvline, MAXLINE,0) == 0) {
            if(canrun){
                //error: server terminated prematurely
                perror("The server terminated prematurely"); 
                exit(4);
            }
        }

        printf("%s", "From server: ");
        fputs(recvline, stdout);
        //clear string
        memset(recvline, 0 , MAXLINE);
    }
    pthread_exit(NULL);
}

//write takes data from user and sends to sever
void *do_write(void *arg){ 
    while(canrun){ 
        while (fgets(sendline, MAXLINE, stdin)!=NULL) {
            if(strcmp(sendline,"quit\n")==0){
                canrun = false;
            }
            send(sockfd, sendline, strlen(sendline), 0);
            memset(sendline , 0 , MAXLINE);
        }   
    }
    pthread_exit(NULL);
}
