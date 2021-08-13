#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdio.h>
#include "lineList.h"
#include "stdbool.h"

/* Enum respresenting possible locations to which commands are sent, 
 * for use as the sentTo variable passed into get_cmd_master.
 * 0 corresponds to client programs, 1 corresponds to server
 */
typedef enum {
    CLIENT, SERVER
} CmdSentTo;

int get_cmd(char *cmd, int sentTo);
LineList *get_cmd_str(char *cmd, bool *invalidCmd);
LineList *get_cmd_stdin(bool *invalidCmd, bool *isLineEmpty);
bool is_comment(char *line);

#endif
