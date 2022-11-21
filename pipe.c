#include<stdio.h>
#include<unistd.h>

int main() {
   int pipefds1[2], pipefds2[2];
   int returnstatus1, returnstatus2;
   int pid;
   char pipe1writemessage[20] = "Hi";
   char pipe2writemessage[20] = "Hello";
   char readmessage[20];
   returnstatus1 = pipe(pipefds1);

   int lpipefds1[2], lpipefds2[2];
   int lreturnstatus1, lreturnstatus2;
   int lpid;
   char lpipe1writemessage[20] = "LHi";
   char lpipe2writemessage[20] = "LHello";
   char lreadmessage[20];
   lreturnstatus1 = pipe(lpipefds1);

   int p1[2], p2[2];
   int r1, r2;
   int bpid;
   char p1message[20] = "BHi";
   char p2message[20] = "BHello";
   char breadmessage[20];
   r1 = pipe(p1);
   
   if (returnstatus1 == -1) {
      printf("Unable to create pipe 1 \n");
      return 1;
   }
   returnstatus2 = pipe(pipefds2);
   
   if (returnstatus2 == -1) {
      printf("Unable to create pipe 2 \n");
      return 1;
   }


   if (lreturnstatus1 == -1) {
      printf("Unable to create pipe 1 \n");
      return 1;
   }
   lreturnstatus2 = pipe(lpipefds2);
   
   if (lreturnstatus2 == -1) {
      printf("Unable to create pipe 2 \n");
      return 1;
   }

   if (r1 == -1) {
      printf("Unable to create pipe 1 \n");
      return 1;
   }
   r2 = pipe(p2);
   
   if (r2 == -1) {
      printf("Unable to create pipe 2 \n");
      return 1;
   }

   pid = fork(); printf("\n#####PID: %d\n", pid);

   
   
   if (pid != 0) {
      close(pipefds1[0]); // Close the unwanted pipe1 read side
      close(pipefds2[1]); // Close the unwanted pipe2 write side
      printf("In Parent: Writing to pipe 1 – Message is %s\n", pipe1writemessage);
      write(pipefds1[1], pipe1writemessage, sizeof(pipe1writemessage));
      read(pipefds2[0], readmessage, sizeof(readmessage));
      printf("In Parent: Reading from pipe 2 – Message is %s\n", readmessage);
      
      lpid = fork(); printf("\n#####PID: %d\n", lpid);

      	if (lpid != 0) {
      	close(lpipefds1[0]); // Close the unwanted pipe1 read side
      	close(lpipefds2[1]); // Close the unwanted pipe2 write side
      	printf("In Parent: Writing to pipe 1 – Message is %s\n", lpipe1writemessage);
      	write(lpipefds1[1], lpipe1writemessage, sizeof(lpipe1writemessage));
      	read(lpipefds2[0], lreadmessage, sizeof(lreadmessage));
      	printf("In Parent: Reading from pipe 2 – Message is %s\n", lreadmessage);
   	} else { 
     	 close(lpipefds1[1]); // Close the unwanted pipe1 write side
      	close(lpipefds2[0]); // Close the unwanted pipe2 read side
      	read(lpipefds1[0], lreadmessage, sizeof(lreadmessage));
      	printf("In Child: Reading from pipe 1 – Message is %s\n", lreadmessage);
      	printf("In Child: Writing to pipe 2 – Message is %s\n", lpipe2writemessage);
      	write(lpipefds2[1], lpipe2writemessage, sizeof(lpipe2writemessage));
        write(p2[1], p2message, sizeof(p2message));
   	}

   } else { 
      close(pipefds1[1]); // Close the unwanted pipe1 write side
      close(pipefds2[0]); // Close the unwanted pipe2 read side
      read(pipefds1[0], readmessage, sizeof(readmessage));
      printf("In Child: Reading from pipe 1 – Message is %s\n", readmessage);
      printf("In Child: Writing to pipe 2 – Message is %s\n", pipe2writemessage);
      write(pipefds2[1], pipe2writemessage, sizeof(pipe2writemessage));
      read(p2[0], breadmessage, sizeof(breadmessage));
      printf("@@@@In Child: Reading from pipe 2 – Message is %s\n", breadmessage);

   }
   return 0;
}