#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "commands.h"
#include "clientData.h"
#include "genericClient.h"

/* Enumerated enum values corresponding to valid commands a client
 * can receive.
 * These values are equal to the index of the respective command
 * as a string in clientCmdWords in commands.c, or as returned by
 * the get_cmd_master function
 */
typedef enum {
    WHO,
    NAME_TAKEN,
    YT,
    KICK,
    MSG,
    LEFT
} ClientCmds;

static FILE *get_script(ClientData *data, char *path);
static void handle_cmd(ClientData *data, LineList *cmdLines, bool invalidCmd);
static void handle_who(ClientData *data);
static void handle_name_taken(ClientData *data);
static void handle_usage_error(ClientData *data);
static void handle_kick(ClientData *data);
static void handle_comms_error(ClientData *data);

/* Loads the script into the client's data based on command-line arguments
 * passed to the client which specify the path of the chatscript/responsefile.
 * Attempts to load the script from the file specified at this path and throws
 * usage error if the file does not exist/cannot be opened.
 *
 * Throws usage error if the number of command-line arguments are incorrect
 * (!= 2).
 *
 * Returns a pointer to the LineList representation of the script.
 */
LineList *setup_client(ClientData *data,
        int argc, char **argv) {

    // Quit with usage error if incorrect no. of commandline args given
    if (argc != 2) {
        handle_usage_error(data);
    }

    FILE *document = get_script(data, argv[1]);
    LineList *script = file_to_line_list(document);
    set_client_script(script, data);
    fclose(document);

    return script; 
}

/* Handles client functionality after a client has been set up correctly.
 * Repeatedly reads commands from stdin and handles them.
 * Loop is broken only when the client exits, i.e. when a KICK:, QUIT: or 
 * invalid command is handled, or if the client has reached the end of its 
 * chatscript.
 * (see handle_cmd(...))
 */
void run_client(ClientData *data) {
    bool invalidCmd;
    bool isLineEmpty;

    while (1) {
        invalidCmd = false;
        isLineEmpty = false;
        LineList *currentCmd = get_cmd_stdin(&invalidCmd, &isLineEmpty);

        /* The line read from stdin is checked if it is empty (contains only
         * EOF) to ensure that the client does not attempt to handle a command
         * in-between a server writing to stdin of the client.
         */
        if (!isLineEmpty) {
            handle_cmd(data, currentCmd, invalidCmd);
        }
    }
}

/* Increments the scriptIndex in the data of the current client */
void client_next_script_index(ClientData *data) {
    next_script_index(data);
}

/*
 * Retrieves a script document to a file pointer from a given path and returns 
 * that file pointer if the document was opened successfully.
 *
 * If the document could not be opened properly/path was incorrect, the
 * function does not return and instead makes the client exit with
 * a usage error.
 */
static FILE *get_script(ClientData *data, char *path) {
    FILE *script;
    if ((script = fopen(path, "r")) == NULL) {
        handle_usage_error(data);       
    }

    return script;
}

/*
 * Handles a command from stdin in LineList representation as returned
 * by get_cmd_stdin(bool *invalidCmd)
 *
 * Takes the invalidCmd bool flag set by get_cmd_stdin as an input
 * in order to handle invalid commands given.
 *
 * Frees the command LineList after the command has been handled.
 */
static void handle_cmd(ClientData *data, LineList *cmdLines, bool invalidCmd) {
    /* Number of arguments, including command name, expected for WHO,
     * NAME_TAKEN, YT, KICK, MSG and LEFT respectively */
    const int numCmdArgs[] = {1, 1, 1, 1, 3, 2};

    int cmdNum = get_cmd(cmdLines->lines[0], CLIENT);

    // Check cmd is not invalid and number of arguments for the cmd is correct
    if (invalidCmd || cmdNum < 0 ||
            numCmdArgs[cmdNum] != cmdLines->numLines) {
        // Handle incorrect command
        free_line_list(cmdLines);
        handle_comms_error(data);
    } else if (cmdNum <= KICK) {
        free_line_list(cmdLines);
        // Handle server -> client commands
        void (*cmdFuncs[])(ClientData *) = {handle_who, handle_name_taken,
                data->config.ytHandler, handle_kick};
        cmdFuncs[cmdNum](data);
    } else if (cmdNum > KICK) {
        /* Run function specified in config to handle MSG: and LEFT: commands.
         * cmdLines is freed by the below function too.
         */
        if (data->config.msgAndLeftHandler == NULL) {
            handle_msg_and_left(cmdLines);
        } else {
            data->config.msgAndLeftHandler(data, cmdLines);
        }
    }
}

/* Returns the name of the client as a string given data of the client.
 * If clientNo in data is <0, the default name of the client is returned.
 * Else the returned name is the default name with the clientNo appened to the
 * end of it, i.e. client0, clientbot1
 */
char *get_name(ClientData *data) {
    char *defaultName = data->config.defaultName;
    int clientNo = data->clientNo;
    // Allocate memory for the name and 9 digits for the clientNo 
    char *name = calloc(strlen(defaultName) + 10, sizeof(char));

    if (clientNo < 0) {
        strcpy(name, defaultName);
    } else {
        sprintf(name, "%s%d", defaultName, clientNo);
    }

    return name;
}

/* Handler for the WHO: command. Emits the name of the client to stdout,
 * including the number of the client, i.e. client0, client1 when the client
 * has received NAME_TAKEN: once or more from the server */
static void handle_who(ClientData *data) {
    char *name = get_name(data);
    printf("NAME:%s\n", name);
    fflush(stdout);
    free(name);
}

/* Handler for the NAME_TAKEN: command. Increments the number of the client.
 * i.e. client with name client0 would change its name to client1
 */
static void handle_name_taken(ClientData *data) {
    next_client_no(data);
}

/*
 * Free client data and exit with given exit code and emit given exit message
 * to stderr
 */
void client_exit(ClientData *data, int exitCode, char *msg) {
    /* Free the client's data on exit */
    free_client_data(data);

    if (exitCode != NORMAL_EXIT) {
        fprintf(stderr, msg);
        fflush(stderr);
    }
    exit(exitCode);
}

/* Handler for usage error.
 * Emits the corresponding the usage error specific to client or clientbot
 * to stderr then exits with the specified error code for usage errors
 */
static void handle_usage_error(ClientData *data) {
    /* Possible client types, matches defaultName in the config struct */
    char *clientTypes[] = {"client", "clientbot"};
    // Usage error messages for client and clientbot respectively
    char *exitMsgs[] = {"Usage: client chatscript\n",
            "Usage: clientbot responsefile\n"};
    int index = find_word(data->config.defaultName, clientTypes, 2);

    client_exit(data, INCORRECT_ARGS, exitMsgs[index]);
}

/* client_exit() wrapper for handling exit on comms error */
static void handle_comms_error(ClientData *data) {
    client_exit(data, COMMS_ERROR, "Communications error\n");
}

/* client_exit() wrapper for handling exiting on a KICK: command */
static void handle_kick(ClientData *data) {
    client_exit(data, KICKED, "Kicked\n");
}

/* Handler for the MSG: and LEFT: commands, the given command cmd as a LineList
 * is freed at the end. Emits the appropriate message or left message to
 * stderr.
 * Assumes number of arguments given are already checked to be correct.
 */
void handle_msg_and_left(LineList *cmd) {
    // Name of command
    char *cmdName = cmd->lines[0];
    // Name of client messaging or leaving
    char *name = cmd->lines[1];

    if (!strcmp(cmdName, "MSG")) {
        char *msg = cmd->lines[2];
        fprintf(stderr, "(%s) %s\n", name, msg);
    } else if (!strcmp(cmdName, "LEFT")) {
        fprintf(stderr, "(%s has left the chat)\n", name);
    }
    fflush(stderr);

    free_line_list(cmd);
}
