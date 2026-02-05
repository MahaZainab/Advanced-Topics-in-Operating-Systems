/*
 * pwordcount: A Pipe-based WordCount Tool (Project 2)
 *
 * What this program does:
 *   - Process 1 (parent) reads a text file and sends its bytes to Process 2 using Pipe #1.
 *   - Process 2 (child) reads those bytes, counts how many words are in the file,
 *     then sends the integer result back to Process 1 using Pipe #2.
 *   - Process 1 prints the final word count.
 *
 * Key requirements covered:
 *   - TWO pipes (parent->child for data, child->parent for result)
 *   - fork() creates TWO cooperating processes
 *   - file is read in a loop (supports large files)
 *   - word counting works even when words are split across chunks (handled in wordcount.c)
 *   - error checking + clean termination (no weird extra prints on error)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

#include "wordcount.h"

#define READ_END 0
#define WRITE_END 1
#define BUF_SIZE 4096

/* Print system error message and exit */
static void die_perror(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

/*
 * write_all:
 * Pipes do NOT guarantee that write(fd, buf, n) writes all n bytes in one call.
 * This function keeps writing until every byte is sent (or a real error happens).
 */
static void write_all(int fd, const void *buf, size_t n)
{
    const unsigned char *p = (const unsigned char *)buf;
    size_t sent = 0;

    while (sent < n)
    {
        ssize_t w = write(fd, p + sent, n - sent);
        if (w < 0)
        {
            if (errno == EINTR)
                continue; /* interrupted: try again */
            die_perror("write");
        }
        sent += (size_t)w;
    }
}

/*
 * read_all:
 * Reads up to n bytes unless EOF occurs first.
 * We mainly use it to safely read the final integer result from pipe2.
 */
static size_t read_all(int fd, void *buf, size_t n)
{
    unsigned char *p = (unsigned char *)buf;
    size_t total = 0;

    while (total < n)
    {
        ssize_t r = read(fd, p + total, n - total);
        if (r < 0)
        {
            if (errno == EINTR)
                continue; /* interrupted: try again */
            die_perror("read");
        }
        if (r == 0)
            break; /* EOF */
        total += (size_t)r;
    }

    return total;
}

int main(int argc, char *argv[])
{
    /* Make stdout unbuffered so prints from parent/child show up immediately */
    setvbuf(stdout, NULL, _IONBF, 0);

    /* If user didn't give a file name, print the required usage message */
    if (argc < 2)
    {
        printf("Please enter a file name.\n");
        printf("Usage: ./pwordcount <file_name>\n");
        return EXIT_FAILURE;
    }

    const char *filename = argv[1];

    int pipe1[2]; /* parent -> child: file bytes */
    int pipe2[2]; /* child -> parent: word count integer */

    if (pipe(pipe1) == -1)
        die_perror("pipe(pipe1)");
    if (pipe(pipe2) == -1)
        die_perror("pipe(pipe2)");

    pid_t pid = fork();
    if (pid < 0)
        die_perror("fork");

    if (pid > 0)
    {
        /* =========================
         * Process 1 (Parent)
         * ========================= */

        /* Parent only WRITES to pipe1 and READS from pipe2 */
        close(pipe1[READ_END]);
        close(pipe2[WRITE_END]);

        printf("Process 1 is reading file \"%s\" now ...\n", filename);

        FILE *fp = fopen(filename, "r");
        if (!fp)
        {
            /*
             * IMPORTANT FIX:
             * If we fail to open the file, we must shut down cleanly.
             * We close the write-end of pipe1 so the child sees EOF and exits quietly.
             * We also wait for the child so we don't leave a zombie process behind.
             */
            fprintf(stderr, "Error: cannot open file \"%s\": %s\n", filename, strerror(errno));

            close(pipe1[WRITE_END]); /* child will get EOF immediately */
            close(pipe2[READ_END]);  /* we won't receive anything */

            waitpid(pid, NULL, 0); /* clean up child process */
            return EXIT_FAILURE;
        }

        printf("Process 1 starts sending data to Process 2 ...\n");

        /* Stream the file into pipe1 in chunks */
        unsigned char buf[BUF_SIZE];
        size_t nread;

        while ((nread = fread(buf, 1, sizeof(buf), fp)) > 0)
        {
            write_all(pipe1[WRITE_END], buf, nread);
        }

        /* If fread stopped due to an error, handle it */
        if (ferror(fp))
        {
            fprintf(stderr, "Error: failed while reading \"%s\".\n", filename);
            fclose(fp);

            close(pipe1[WRITE_END]);
            close(pipe2[READ_END]);

            waitpid(pid, NULL, 0);
            return EXIT_FAILURE;
        }

        fclose(fp);

        /* Closing this signals EOF to the child (very important!) */
        close(pipe1[WRITE_END]);

        /* Receive the result (an int) from pipe2 */
        int result = 0;
        size_t got = read_all(pipe2[READ_END], &result, sizeof(result));
        close(pipe2[READ_END]);

        if (got != sizeof(result))
        {
            fprintf(stderr, "Error: did not receive wordcount result from Process 2.\n");
            waitpid(pid, NULL, 0);
            return EXIT_FAILURE;
        }

        waitpid(pid, NULL, 0);

        printf("Process 1: The total number of words is %d.\n", result);
        return EXIT_SUCCESS;
    }
    else
    {
        /* =========================
         * Process 2 (Child)
         * ========================= */

        /* Child only READS from pipe1 and WRITES to pipe2 */
        close(pipe1[WRITE_END]);
        close(pipe2[READ_END]);

        unsigned char buf[BUF_SIZE];
        int total_words = 0;
        int prev_in_word = 0;

        /*
         * Read from pipe1 until EOF.
         * EOF happens when parent closes pipe1[WRITE_END].
         */
        int received_anything = 0;
        while (1)
        {
            ssize_t r = read(pipe1[READ_END], buf, sizeof(buf));
            if (r < 0)
            {
                if (errno == EINTR)
                    continue;
                die_perror("read(pipe1)");
            }
            if (r == 0)
                break; /* EOF */

            received_anything = 1;
            total_words += count_words_in_buffer(buf, (size_t)r, &prev_in_word);
        }

        close(pipe1[READ_END]);

        /*
         * If parent couldn't open the file, it closes pipe1 immediately.
         * In that case, we received nothing and should exit quietly
         * (so we don't print confusing "Process 2..." messages).
         */
        if (!received_anything)
        {
            close(pipe2[WRITE_END]);
            return EXIT_FAILURE;
        }

        /* Normal successful case: print required status messages */
        printf("Process 2 finishes receiving data from Process 1 ...\n");
        printf("Process 2 is counting words now ...\n");
        printf("Process 2 is sending the result back to Process 1 ...\n");

        /* Send result back to parent */
        write_all(pipe2[WRITE_END], &total_words, sizeof(total_words));
        close(pipe2[WRITE_END]);

        return EXIT_SUCCESS;
    }
}
