#ifndef GENERICCLIENT_H
#define GENERICCLIENT_H

#include <stdio.h>
#include "clientData.h"
#include "lineList.h"

/* Possible client exit codes */
typedef enum {
    NORMAL_EXIT,
    INCORRECT_ARGS,
    COMMS_ERROR,
    KICKED
} ClientExitCodes;

LineList *setup_client(ClientData *data, int argc,
        char **argv);
char *get_name(ClientData *data);
void run_client();
void client_exit(ClientData *data, int exitCode, char *msg);
void handle_msg_and_left(LineList *cmd);

#endif
