#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "commands.h"

/* Strings corresponding to commands that can be sent to a client */
static const char *clientCmdWords[] = {
        "WHO",
        "NAME_TAKEN",
        "YT",
        "KICK",
        "MSG",
        "LEFT"
        };

/* Strings corresponding to commands that can be sent to a server.
 * "NAME" is not included here as name negotiation is handled separately
 * from the other commands. (see server.c)*/
static const char *serverCmdWords[] = {
        "CHAT",
        "KICK",
        "DONE",
        "QUIT"
        };

/* Number of possible commands for client and server respectively*/
static const int cmdCount[] = {6, 5};

/* List of list of commands that can be sent to client and server respectively
 */
static const char **cmds[] = {clientCmdWords, serverCmdWords};

/* Converts command cmd given as a string to the index of that string
 * in either clientCmdWords or serverCmdWords if it is in the array.
 * If sentTo is 0, the string is looked for in clientCmdWords whilst if it 
 * is 1 it is looked for in serverCmdWords.
 *
 * If the index is found it is returned, else -1 is returned.
 */
int get_cmd(char *cmd, int sentTo) {
    // Get list of commands
    const char **cmdWords = cmds[sentTo];
    // Default value is -1, no command matched
    int matchedCmd = -1;
    
    for (int i = 0; i < cmdCount[sentTo]; ++i) {
        if (!strcmp(cmd, cmdWords[i])) {
            matchedCmd = i;
        }
    }

    return matchedCmd;
}

/*
 * Reads a command from a single line of stdin and returns it represented
 * as a LineList struct using the function get_cmd_str().
 *
 * Sets a boolean flag invalidCmd to true if the command has invalid syntax
 * as per get_cmd_str().
 *
 * Sets another boolean flag isLineEmpty to true if the command is empty
 * (i.e. it contains just EOF)
 */
LineList *get_cmd_stdin(bool *invalidCmd, bool *isLineEmpty) {
    char *stdinLine = read_line_stdin(isLineEmpty);
    LineList *cmdLines = get_cmd_str(stdinLine, invalidCmd);
    free(stdinLine);

    return cmdLines;
}

/*
 * Returns a LineList representation of a command given as a string, where each
 * argument of the command delimited by ':' is saved as a separate entry in
 * the LineList.
 *
 * If a pointer to a bool flag is given (i.e. not NULL), then
 * sets the flag (*invalidCmd) to true if the command has invalid
 * formatting (if it is one word long and not terminated with a ':')
 */
LineList *get_cmd_str(char *cmd, bool *invalidCmd) {
    // Make a copy of cmd for strtok to work on as strtok modifies strings
    char *cmdCopy = calloc(strlen(cmd) + 1, sizeof(char));
    strcpy(cmdCopy, cmd);

    LineList *cmdLines = init_line_list();

    char *token = strtok(cmdCopy, ":");
    while (token != NULL) {
        add_to_lines(cmdLines, token);
        token = strtok(NULL, ":");
    }

    free(cmdCopy);

    // Check the command has valid format
    if (invalidCmd != NULL) {
        if ((cmdLines->numLines == 1) &&
                (cmd[strlen(cmd) - 1] != ':')) {
        // Command is invalid if it is one word long and doesn't end in ":"
            *invalidCmd = true;
        } else {
            *invalidCmd = false;
        }
    }

    return cmdLines;
}

/* Checks if a line from a clientbot responsefile or server configfile is
 * a comment.
 *
 * If the first non-whitespace character of line is '#', then this function
 * returns true; else false is returned.
 */
bool is_comment(char *line) {
    bool isComment = false;

    for (int i = 0; i < strlen(line); ++i) {
        // Ignore whitespaces
        if (isspace(line[i])) {
            ;
        } else if (line[i] == '#') {
            isComment = true;
            break;
        } else {
            break;
        }
    }

    return isComment;
}
