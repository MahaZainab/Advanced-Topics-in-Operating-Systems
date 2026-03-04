#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>

#define READ_END 0
#define WRITE_END 1
#define BUFFER_SIZE 4096

int count_words(const char *buf){
int count=0, in_word=0;

for (int i=0; buf[i]!= '\0'; i++){
if (isspace(buf[i])){
in_word=0;
}
else if (!in_word){
in_word=1;
count++;
}

}
return count;
}


int main(int argc, char *argv[]){
if (argc !=2){
printf("Please enter the file name \n");
printf("Usage: ./pwordcount <file_name>\n");
return 1;
}

int pipe1[2];
int pipe2[2];

pipe(pipe1);
pipe(pipe2);

pid_t pid = fork();

if (pid>0){
/* Parent Process*/
close(pipe1[READ_END]);
close(pipe2[WRITE_END]);
printf("Process 1 is reading file \"%s\" now \n", argv[1]);
FILE *fp = fopen(argv[1], "r");
if (! fp){
perror("File open failed");
exit(1);
}
char buffer[BUFFER_SIZE];
int bytes = fread(buffer, 1, BUFFER_SIZE-1, fp);
buffer[bytes]='\0';
fclose(fp);

printf("Process 1 starts sending data to process 2");
write(pipe1[WRITE_END], buffer, strlen(buffer)+1);
close(pipe1[WRITE_END]);

int result;
read(pipe2[READ_END], &result, sizeof(result));
close(pipe2[READ_END]);
printf("Process 1: the total  number of words is %d \n", result);

wait(NULL);
}
else{
close(pipe1[WRITE_END]);
close(pipe2[READ_END]);

char buffer[BUFFER_SIZE];
read(pipe1[READ_END], buffer, BUFFER_SIZE);
close(pipe1[READ_END]);

printf("Process 2 finishes receiving data from process 1 \n");
printf("Process 2 is counting words now \n");
int wc = count_words(buffer);
printf("Process 2 is sending the result back to process 1 \n");
write(pipe2[WRITE_END], &wc, sizeof(wc));
close(pipe2[WRITE_END]);

}
return 0;
}
