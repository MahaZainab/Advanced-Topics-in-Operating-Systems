#ifndef WORDCOUNT_H
#define WORDCOUNT_H

#include <stddef.h>

/*
 * Count words in a chunk of bytes.
 *
 * IMPORTANT: This function supports streaming.
 * That means if a word gets split across two chunks,
 * we still count it correctly by using prev_in_word.
 *
 * prev_in_word:
 *   - input:  0 if the previous chunk ended outside a word,
 *             1 if the previous chunk ended inside a word.
 *   - output: updated state after scanning this chunk.
 */
int count_words_in_buffer(const unsigned char *buf, size_t n, int *prev_in_word);

#endif
