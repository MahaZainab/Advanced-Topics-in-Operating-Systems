/*
 * pwordcount: A Pipe-based WordCount Tool
 *
 * Two processes:
 *   - Process 1 (parent):
 *       1) reads the input file
 *       2) sends file bytes to Process 2 via pipe #1
 *       3) receives the final word count via pipe #2
 *       4) prints the answer
 *
 *   - Process 2 (child):
 *       1) receives file bytes from pipe #1
 *       2) counts words
 *       3) sends the integer result back via pipe #2
 *
 * We use TWO pipes because:
 *   - pipe #1 is parent -> child (file content)
 *   - pipe #2 is child  -> parent (result integer)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

#include "wordcount.h"

#define READ_END  0
#define WRITE_END 1

/* A chunk size of 4096 is common and works well */
#define BUF_SIZE 4096

/* Simple helper: print OS error message and exit */
static void die_perror(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

/*
 * write_all:
 * write() is allowed to write fewer bytes than requested.
 * This function keeps writing until everything is sent.
 *
 * This is one of the biggest “robustness” improvements for pipe code.
 */
static void write_all(int fd, const void *buf, size_t n)
{
    const unsigned char *p = (const unsigned char *)buf;
    size_t sent = 0;

    while (sent < n) {
        ssize_t w = write(fd, p + sent, n - sent);

        if (w < 0) {
            if (errno == EINTR) {
                /* Interrupted by a signal; try again */
                continue;
            }
            die_perror("write");
        }

        /* write() succeeded, w bytes were written */
        sent += (size_t)w;
    }
}

/*
 * read_all:
 * reads up to n bytes unless EOF occurs.
 * Returns how many bytes were actually read.
 *
 * We use this mainly to read the final integer result safely.
 */
static size_t read_all(int fd, void *buf, size_t n)
{
    unsigned char *p = (unsigned char *)buf;
    size_t total = 0;

    while (total < n) {
        ssize_t r = read(fd, p + total, n - total);

        if (r < 0) {
            if (errno == EINTR) {
                /* Interrupted by a signal; try again */
                continue;
            }
            die_perror("read");
        }

        if (r == 0) {
            /* EOF */
            break;
        }

        total += (size_t)r;
    }

    return total;
}

int main(int argc, char *argv[])
{
    /* ----- Usability requirement: handle missing filename nicely ----- */
    if (argc < 2) {
        printf("Please enter a file name.\n");
        printf("Usage: ./pwordcount <file_name>\n");
        return EXIT_FAILURE;
    }

    const char *filename = argv[1];

    int pipe1[2]; /* parent -> child: raw file bytes */
    int pipe2[2]; /* child -> parent: wordcount integer */

    /* ----- Create the two pipes (required by the project spec) ----- */
    if (pipe(pipe1) == -1) die_perror("pipe(pipe1)");
    if (pipe(pipe2) == -1) die_perror("pipe(pipe2)");

    /* ----- Fork a child process (Process 2) ----- */
    pid_t pid = fork();
    if (pid < 0) die_perror("fork");

    if (pid > 0) {
        /* =========================
         * Process 1 (Parent)
         * ========================= */

        /* Close ends we won't use in the parent */
        close(pipe1[READ_END]);    /* parent never reads from pipe1 */
        close(pipe2[WRITE_END]);   /* parent never writes to pipe2 */

        printf("Process 1 is reading file \"%s\" now ...\n", filename);

        /* Open the file safely */
        FILE *fp = fopen(filename, "r");
        if (!fp) {
            fprintf(stderr, "Error: cannot open file \"%s\": %s\n", filename, strerror(errno));

            /* Clean up pipes before exiting */
            close(pipe1[WRITE_END]);
            close(pipe2[READ_END]);
            return EXIT_FAILURE;
        }

        printf("Process 1 starts sending data to Process 2 ...\n");

        /*
         * Read file in chunks and stream them into pipe1.
         * This is correct for both small files and large files.
         */
        unsigned char buf[BUF_SIZE];
        size_t nread;

        while ((nread = fread(buf, 1, sizeof(buf), fp)) > 0) {
            write_all(pipe1[WRITE_END], buf, nread);
        }

        /* Check if fread stopped because of an error */
        if (ferror(fp)) {
            fprintf(stderr, "Error: failed while reading \"%s\".\n", filename);
            fclose(fp);
            close(pipe1[WRITE_END]);
            close(pipe2[READ_END]);
            return EXIT_FAILURE;
        }

        fclose(fp);

        /*
         * IMPORTANT: Closing the write end tells the child “no more data”.
         * If we forget this close(), the child may block forever waiting for EOF.
         */
        close(pipe1[WRITE_END]);

        /* Now receive the integer result from the child via pipe2 */
        int result = 0;
        size_t got = read_all(pipe2[READ_END], &result, sizeof(result));
        close(pipe2[READ_END]);

        if (got != sizeof(result)) {
            fprintf(stderr, "Error: did not receive wordcount result from Process 2.\n");
            waitpid(pid, NULL, 0);
            return EXIT_FAILURE;
        }

        /* Wait for child to finish cleanly (avoid zombie process) */
        waitpid(pid, NULL, 0);

        printf("Process 1: The total number of words is %d.\n", result);
        return EXIT_SUCCESS;

    } else {
        /* =========================
         * Process 2 (Child)
         * ========================= */

        /* Close ends we won't use in the child */
        close(pipe1[WRITE_END]);   /* child never writes to pipe1 */
        close(pipe2[READ_END]);    /* child never reads from pipe2 */

        /*
         * Read all incoming bytes from the parent until EOF,
         * and count words as we go.
         */
        unsigned char buf[BUF_SIZE];
        int total_words = 0;
        int prev_in_word = 0; /* tracks word split between chunks */

        while (1) {
            ssize_t r = read(pipe1[READ_END], buf, sizeof(buf));

            if (r < 0) {
                if (errno == EINTR) continue;
                die_perror("read(pipe1)");
            }

            if (r == 0) {
                /* EOF: parent closed its write end */
                break;
            }

            total_words += count_words_in_buffer(buf, (size_t)r, &prev_in_word);
        }

        close(pipe1[READ_END]);

        /*
         * These messages are printed AFTER receiving completes,
         * which matches the meaning of the sample output.
         */
        printf("Process 2 finishes receiving data from Process 1 ...\n");
        printf("Process 2 is counting words now ...\n");
        printf("Process 2 is sending the result back to Process 1 ...\n");

        /* Send the result back to parent via pipe2 */
        write_all(pipe2[WRITE_END], &total_words, sizeof(total_words));
        close(pipe2[WRITE_END]);

        return EXIT_SUCCESS;
    }
}
