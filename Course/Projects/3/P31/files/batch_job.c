
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <seconds>\n", argv[0]);
        return 1;
    }

    int secs = atoi(argv[1]);
    if (secs <= 0)
    {
        fprintf(stderr, "seconds must be > 0\n");
        return 1;
    }

    sleep(secs);
    return 0;
}
