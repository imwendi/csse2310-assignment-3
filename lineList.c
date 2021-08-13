#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lineList.h"

/*
 * "Read" in "ReadLine" is past tense :)
 * ReadLine stores lines read from a file with the get_line function.
 * Stores the line read and a bool value representing if the line is the last 
 * line of the file.
 *
 * (This struct was previously written for my A1 submission)
 */
typedef struct {
    /* The line read from a file */
    char *line;
    /* Whether or not the line was the last line in the file */
    bool isLastLine;
} ReadLine;

static ReadLine *get_line(FILE *dict, bool *isLineEmpty);
static void free_read_line(ReadLine *targetLine);

/* Initializes, allocates memory for and returns a pointer to a new,
 * empty LineList.
 *
 * (Adapted from code from my A1 submission)
 */
LineList *init_line_list() {
    LineList *output = (LineList *) malloc(sizeof(LineList));
    output->numLines = 0;
    output->lines = (char **) malloc(0);

    return output;
}

/* Allocates memory for a new line in an existing LineList and stores a given 
 * string to to end of that LineList */
void add_to_lines(LineList *target, char *line) {
    int numLines = ++(target->numLines);
    
    // Allocate memory for the given string and strcpy it into the LineList
    target->lines = (char **) realloc(target->lines,
            numLines * sizeof(char *));
    target->lines[numLines - 1] = (char *) calloc(strlen(line) + 1,
            sizeof(char));
    strcpy(target->lines[numLines - 1], line);
}

/* Frees memory associated to a LineList at a pointer
 *
 * (Adapted from code from my A1 submission)
 */
void free_line_list(LineList *linesToFree) {
    //Free all structure members
    for (int i = 0; i < (linesToFree->numLines); ++i) {
        // Free each line stored in the LineList
        free(linesToFree->lines[i]);
    }
    free(linesToFree->lines);
    free(linesToFree);
}

/* Reads a line from a file to a string and returns that string */
char *read_file_line(FILE *doc, bool *isLineEmpty) {
    ReadLine *tempLine = get_line(doc, isLineEmpty);
    // Keep the line but free the temporary ReadLine structure
    char *line = tempLine->line;
    free(tempLine);

    return line;
}

/* Wrapper for read_file_line to read a line from stdin to a string and returns
 * that string.
 *
 * It also sets a bool flag isLineEmpty to true if the line read from stdin is
 * completely empty, i.e. it only contains EOF and not even '\n' etc.
 */
char *read_line_stdin(bool *isLineEmpty) {
    return read_file_line(stdin, isLineEmpty);
}

/* Reads a line from a file to a ReadLine struct and returns it.
 * Also sets the boolean flag isLineEmpty to true if the line only contains
 * the EOF character.
 * This is only done if the pointer to this flag is not null.
 *
 * (Adapted from code from my A1 submission)
 */
static ReadLine *get_line(FILE *dict, bool *isLineEmpty) {
    int index = 0;
    char *line = (char *) calloc(1, sizeof(char));
    // Initialize the first character to '\0' in case of an empty file.
    line[0] = '\0';
    int nextChar = fgetc(dict);

    /* Set the below bool flag to true if the line just contains EOF, else
     * it's default value is true.
     */
    bool isLastLine = false;
    if (nextChar == EOF) {
        isLastLine = true;
        if (isLineEmpty != NULL) {
            *isLineEmpty = true;
        }
    }

    while (((char) nextChar != '\n') && !isLastLine) {
        /* Place the last read character into the output string
         * +2 accounts for '\0' and new character allocated
         */
        line = (char *) realloc(line, (index + 2) * sizeof(char));
        line[index++] = (char) nextChar;
     
        // Read the next character
        nextChar = fgetc(dict);

        // Break out of loop if EOF is found
        if (nextChar == EOF) {
            isLastLine = true;
        }
    }

    /* Add end of string character '\0' if the end character currently
     * isn't already '\0'
     */
    if (index > 0 && line[index - 1] != '\0') {
        line = (char *) realloc(line, (index + 2) * sizeof(char));
        line[index] = '\0';
    }

    // Create and return the ReadLine struct
    ReadLine *readLine = malloc(sizeof(ReadLine));
    readLine->line = line;
    readLine->isLastLine = isLastLine;

    return readLine;
}

/* Frees memory allocated to a ReadLine at a given pointer
 *
 * (Adapted from code from my A1 submission)
 */
static void free_read_line(ReadLine *targetLine) {
    free(targetLine->line);
    free(targetLine);
}

/*
 * Reads a file and return a LineList containing the lines in the file.
 *
 * (Adapted from code from my A1 submission)
 */
LineList *file_to_line_list(FILE *script) {

    int numLines = 0;
    char **lines = (char **) malloc(0);
    ReadLine *nextLine = get_line(script, NULL);

    while (1) { 
        // Append the nextLine.line to lines
        numLines++;
        lines = (char **) realloc(lines, numLines * sizeof(char *));
        
        /* 
         * Allocate memory for the word and then strcpy it into the
         * correct index in lines
         */
        lines[numLines - 1] = \
                (char *) calloc(strlen(nextLine->line) + 1, sizeof(char));
        strcpy(lines[numLines - 1], nextLine->line);
       
        if (nextLine->isLastLine) {
            break;
        }

        // Free memory allocated to nextLine before reading the next line to it
        free_read_line(nextLine);

        nextLine = get_line(script, NULL);
    }

    /* Remove last line if it just contains EOF or '\0'.
     * This is useful as vim adds an extra '\n' to the last line of a file
     * by default.
     */
    if (lines[numLines - 1][0] == '\0' || lines[numLines - 1][0] == EOF) {
        free(lines[numLines - 1]);
        numLines--;
    }

    // Create the output LineList struct
    LineList *outputList = malloc(sizeof(LineList));
    outputList->lines = lines;
    outputList->numLines = numLines;

    free_read_line(nextLine);

    return outputList;
}

/*
 * Finds the first occurrence of a string (line) in an array of strings (lines)
 * and returns its index or -1 if it wasn't found.
 * len is the the length of the array of strings.
 */
int find_word(char *line, char **lines, int len) {
    int index = -1;

    for (int i = 0; i < len; ++i) {
        if (!strcmp(line, lines[i])) {
            index = i;
            break;
        }
    }

    return index;
}

/* A naive implementation of string pattern matching.
 * Returns 1 if the pattern appears anywhere in a given target string.
 * Returns 0 if not.
 * Matching is CASE INSENSITIVE.
 */
static int pattern_match_string(char *pattern, char *target) {

    // Default return val is 0
    int matched = 0;

    /* Offset is an integer iterating from 0 to the length of the target
     * string.
     *
     * Check if target[offset : offset + length of pattern] matches the given
     * pattern, and if so, sets the output value as true and breaks from the
     * loop.
     */
    for (int offset = 0; offset < strlen(target); ++offset) {
        if (strncasecmp(pattern, target + offset,
                strlen(pattern) * sizeof(char)) == 0) {
            matched = 1;
            break;
        }
    }

    return matched;
}

/* Returns the index of the first entry in a LineList of patterns
 * which is contained in the given target string anywhere.
 * Pattern matching is case insensitive.
 *
 * If the pattern is not found, -1 is returned instead
 */
int pattern_match_lines(char *target, LineList *patternList) {
    int foundIndex = -1;
    for (int i = 0; i < patternList->numLines; ++i) {
        if (pattern_match_string(patternList->lines[i], target)) {
            foundIndex = i;
            break;
        }
    }

    return foundIndex;
}
