#include<stdio.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<sys/types.h>
#include<string.h>
#include<errno.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<stdbool.h>
#include<pthread.h>
#include<stdarg.h>

#define READ 0
#define WRITE 1

typedef struct{
    int flag[2];
    int turn;
    int client_count;
    int current_count;
}permissions;

typedef struct{
    char str[1000];
    pid_t pid;
}data;

void *do_read(void *arg);
void *do_write(void *arg);

bool canrun = true;
permissions *p;
data *d;

int main(){

    bool justmade = false;
    pthread_t thread1;     //thread for reader
    pthread_t thread2;     //thread for writer
    int thr1;
    int thr2;
	int data_id;
    int perm_id;

    // ftok to generate unique key
    key_t key1 = ftok("chatserv.c",1);
    key_t key2 = ftok("chatserv.c",2);
  
    // Attempt to create shared memory
    data_id = shmget(key1,sizeof(data), 0660);
    // Check if mem already exists
    if(data_id == -1){
        data_id = shmget(key1,sizeof(data), 0660 | IPC_CREAT);
        perm_id = shmget(key2,sizeof(permissions), 0660 | IPC_CREAT);
        justmade = true;
    }
    else{
        perm_id = shmget(key2,sizeof(permissions),0660);
    }

    if (data_id == -1 || perm_id == -1) {
        perror("Shared memory");
        exit(1);
    }

    p = shmat(data_id, NULL, 0);
    d = shmat(perm_id, NULL, 0);

    if (p == (void *) -1 || d == (void *) -1) {
        perror("Shared memory attach");
        exit(1);
    }

    if(justmade){
        justmade = false;
        p->flag[READ] = false;
        p->flag[WRITE] = true;
        p->turn = 0;
        p->client_count = 1;
        p->current_count = 1;
        d->pid = 0;
    }
    
    if ((thr2 = pthread_create(&thread2, NULL, do_write, NULL)) != 0) {
        fprintf(stderr, "Thread create error %d: %s\n", thr2, strerror(thr2));
        exit(1);
    }
    pthread_detach(thread2);
    if ((thr1 = pthread_create(&thread1, NULL, do_read, NULL)) != 0) {
        fprintf(stderr, "Thread create error %d: %s\n", thr1, strerror(thr1));
        exit(1);
    }
    
    pthread_join(thread1,NULL);
    
    if (shmdt(d) == -1 || shmdt(p) == -1) {
      perror("shmdt");
      return 1;
   }

   if (shmctl(data_id, IPC_RMID, 0) == -1 || shmctl(perm_id, IPC_RMID, 0) == -1) {
      perror("shmctl");
      return 1;
   }
    return 0;
}

//read takes data from server and prints
//write takes data from user and writes to sever
void *do_read(void *arg){
    while(canrun){
        if(p->flag[READ]){
            //critical section 
            if(d->pid != getpid()){
                printf("User %d: %s\n", d->pid, d->str);
            }       
            ++p->current_count;
            while(p->current_count < p->client_count){
                sleep(1);
            }
            --p->turn;
            p->flag[READ] = false;
            if(p->turn == 0){
                p->flag[WRITE] = true;
            }
        }
    }
    pthread_exit(NULL);
}

void *do_write(void *arg){
    while(canrun){
        if(p->flag[WRITE]){
            //critical section 
            int turn;
            char* message = malloc(1000 * sizeof(char));
            bzero(message, 1000);
            //printf("Enter a message:\n");
            if(fgets(message, 1000, stdin) != NULL){
                p->flag[WRITE] = false;
                if(strcmp(message,"quit\n")==0){
                    canrun = false;
                    pthread_exit(NULL);
                }
                printf("Sending...\n");
                ++p->turn;
                turn = p->turn;

                while(p->turn!=turn){
                    sleep(1);
                }
                printf("Sent\n");
                p->current_count = 0;
                strcpy(d->str,message);
                d->pid = getpid();  
                p->flag[READ] = true;
            }
            free(message);
        }
    }
    pthread_exit(NULL);
}
