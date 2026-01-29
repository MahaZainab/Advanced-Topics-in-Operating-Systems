#include "wordcount.h"
#include <ctype.h>

int count_words_chunk(const char *buf, ssize_t n, int *prev_in_word)
{
    int count = 0;
    int in_word = *prev_in_word;

    for (ssize_t i = 0; i < n; i++)
    {
        unsigned char c = (unsigned char)buf[i];
        if (isspace(c))
        {
            in_word = 0;
        }
        else if (!in_word)
        {
            in_word = 1;
            count++;
        }
    }

    *prev_in_word = in_word;
    return count;
}
