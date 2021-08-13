#ifndef CLIENTBOTUTILS_H
#define CLIENTBOTUTILS_H

#include "lineList.h"

/* Structure which stores clientBot stimuli and responses.
 * Acts like a dictionary between the stimuli and responses.
 *
 * Stimuli and responses are stored in their respective WordLists, with
 * each matching stimulus and response sharing the same index in their
 * respective WordLists.
 */
typedef struct {
    /* LineList storing a clientbot's stimuli from a responsefile */
    LineList *stimuli;
    /* LineList storing a clientbot's responses from a responsefile */
    LineList *responses;
} ResponseDict;

/* Structure which stores the responses a clientbot should print
 * to stdout on the next YT: command given.
 *
 * responses: Integer array corresponding to the index of a particular response
 * in the response LineList of a clientbots response dictionary. (ResponseDict)
 *
 * bufferLen: Number of elements in responses
 */
typedef struct {
    int *responses;
    int bufferLen;
} ResponseBuffer;

void free_dict(ResponseDict *dict);
ResponseDict *lines_to_dict(LineList *responseList);
ResponseBuffer *init_buffer();
void append_buffer(int newIndex, ResponseBuffer *buff);
void free_buffer(ResponseBuffer *buff);

#endif
