#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lineList.h"
#include "commands.h"
#include "clientbotUtils.h"

/* Initializes a new ResponseDict struct and returns a pointer to it */
static ResponseDict *init_dict() {
    ResponseDict *dict = malloc(sizeof(ResponseDict));
    dict->stimuli = init_line_list();
    dict->responses = init_line_list();

    return dict;
}

/* Frees memory allocated to a ResponseDict struct. */
void free_dict(ResponseDict *dict) {
    free_line_list(dict->stimuli);
    free_line_list(dict->responses);
    free(dict);
}

/* Creates a ResponseDict structure given a LineList containing lines
 * from a responsefile and returns pointer to it.
 *
 * For every line saved in LineList->lines:
 * if the line is a valid responsefile line in the format stimulus:response,
 * stimulus is appended to dict->stimuli and response is appended to
 * dict->responses.
 *
 * If the line is commented or is of invalid format, it is
 * simply ignored and the next line processed.
 */
ResponseDict *lines_to_dict(LineList *responseList) {
    ResponseDict *dict = init_dict();

    for (int i = 0; i < responseList->numLines; ++i) {
       /* Read current line of responsefile LineList to a LineList
        * representation of a command as per get_cmd_str()
        */
        LineList *currentLine = get_cmd_str(responseList->lines[i], 0);
        /* Check for valid <stimulus>:<response> format, i.e. 
         * - the line's first non-space char isn't '#'
         * - the line when delimited by ':' contains exactly two strings.
         * - the response string is not empty (i.e. length = 0)
         */
        if ((currentLine->numLines == 2) &&
                (!is_comment(currentLine->lines[0]) &&
                strlen(currentLine->lines[1]) > 0)) {
            // Add stimulus and response to the dict
            add_to_lines(dict->stimuli, currentLine->lines[0]);
            add_to_lines(dict->responses, currentLine->lines[1]);
        }

        free_line_list(currentLine);
    }

    return dict;
}

/* Initializes a new ResponseBuffer struct and returns a pointer to it */
ResponseBuffer *init_buffer() {
    ResponseBuffer *newBuff = malloc(sizeof(ResponseBuffer));
    newBuff->responses = calloc(0, sizeof(int));
    newBuff->bufferLen = 0;

    return newBuff;
}

/* Allocates memory for an additional response index in a ResponseBuffer
 * and appends a given index to that ResponseBuffer.
 */
void append_buffer(int newIndex, ResponseBuffer *buff) {
    buff->bufferLen++;
    buff->responses = realloc(buff->responses, sizeof(int) * buff->bufferLen);
    buff->responses[buff->bufferLen - 1] = newIndex;
}

/* Frees memory allocated to a ResponseBuffer structure */
void free_buffer(ResponseBuffer *buff) {
    free(buff->responses);
    free(buff);
}
