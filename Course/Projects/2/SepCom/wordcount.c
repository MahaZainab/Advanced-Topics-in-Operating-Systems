#include "wordcount.h"
#include <ctype.h>

/*
 * A "word" here is: a sequence of non-whitespace characters.
 * Whitespace includes space, tab, newline, etc. (isspace()).
 *
 * Example:
 *   "hi  there\nfriend"  => 3 words
 *
 * The tricky part:
 *   if "friend" is split across two reads (e.g., "fr" + "iend"),
 *   we must NOT count it twice.
 */
int count_words_in_buffer(const unsigned char *buf, size_t n, int *prev_in_word)
{
    int count = 0;

    /* Start in whatever state the previous chunk ended in */
    int in_word = (*prev_in_word != 0);

    for (size_t i = 0; i < n; i++) {
        unsigned char c = buf[i];

        /* Always cast to unsigned char before isspace() */
        if (isspace((unsigned char)c)) {
            /* Any whitespace means we are NOT inside a word anymore */
            in_word = 0;
        } else {
            /*
             * Non-whitespace character:
             * if we were not already in a word, this is the START of a new word.
             */
            if (!in_word) {
                count++;
                in_word = 1;
            }
        }
    }

    /* Save state for next chunk */
    *prev_in_word = in_word;
    return count;
}
