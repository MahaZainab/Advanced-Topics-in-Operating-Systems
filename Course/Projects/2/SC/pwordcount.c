/*
 * pwordcount.c
 * Project 2: pWordcount - A Pipe-based WordCount Tool
 *
 * Name: <YOUR NAME>
 * AU ID: <YOUR AU ID>
 *
 * Process 1 (parent): reads file, sends content to Process 2 via pipe1
 * Process 2 (child): counts words, sends result back via pipe2
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "wordcount.h"

#define READ_END 0
#define WRITE_END 1
#define BUFFER_SIZE 4096

static void die(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Please enter a file name.\n");
        printf("Usage: ./pwordcount <file_name>\n");
        return 1;
    }

    int pipe1[2]; // Process 1 -> Process 2
    int pipe2[2]; // Process 2 -> Process 1

    if (pipe(pipe1) == -1) die("pipe1 failed");
    if (pipe(pipe2) == -1) die("pipe2 failed");

    pid_t pid = fork();
    if (pid < 0) die("fork failed");

    if (pid > 0) {
        /* -------- Parent (Process 1) -------- */
        close(pipe1[READ_END]);
        close(pipe2[WRITE_END]);

        printf("Process 1 is reading file \"%s\" now ...\n", argv[1]);

        FILE *fp = fopen(argv[1], "r");
        if (!fp) die("File open failed");

        printf("Process 1 starts sending data to Process 2 ...\n");

        char buffer[BUFFER_SIZE];
        size_t nread;
        while ((nread = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
            ssize_t nw = write(pipe1[WRITE_END], buffer, (ssize_t)nread);
            if (nw < 0) die("write to pipe1 failed");
        }

        fclose(fp);
        close(pipe1[WRITE_END]); // EOF to child

        int result = 0;
        ssize_t rr = read(pipe2[READ_END], &result, sizeof(result));
        if (rr != sizeof(result)) die("read from pipe2 failed");
        close(pipe2[READ_END]);

        printf("Process 1: The total number of words is %d.\n", result);
        wait(NULL);

    } else {
        /* -------- Child (Process 2) -------- */
        close(pipe1[WRITE_END]);
        close(pipe2[READ_END]);

        printf("Process 2 finishes receiving data from Process 1 ...\n");
        printf("Process 2 is counting words now ...\n");

        char buffer[BUFFER_SIZE];
        int total = 0;
        int prev_in_word = 0;

        ssize_t nr;
        while ((nr = read(pipe1[READ_END], buffer, sizeof(buffer))) > 0) {
            total += count_words_chunk(buffer, nr, &prev_in_word);
        }

        close(pipe1[READ_END]);

        printf("Process 2 is sending the result back to Process 1 ...\n");
        if (write(pipe2[WRITE_END], &total, sizeof(total)) != sizeof(total)) {
            die("write to pipe2 failed");
        }
        close(pipe2[WRITE_END]);
    }

    return 0;
}
