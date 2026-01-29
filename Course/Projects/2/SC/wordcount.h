#ifndef WORDCOUNT_H
#define WORDCOUNT_H

#include <sys/types.h>

// Counts words in a chunk of bytes.
// prev_in_word keeps state between chunks (0 = no, 1 = yes).
int count_words_chunk(const char *buf, ssize_t n, int *prev_in_word);

#endif
