#include <stdio.h>
#include <string.h>
#include "commands.h"
#include "clientData.h"
#include "genericClient.h"

static void client_yt_handler();

int main(int argc, char **argv) {
    // Set up client config
    ClientConfig config;
    config.defaultName = "client";
    config.msgAndLeftHandler = NULL;
    config.ytHandler = client_yt_handler;

    // Declare the ClientData struct data to be used by the client
    ClientData *data = init_client_data(config);

    setup_client(data, argc, argv);
    run_client(data);

    return 0;
}

/* Given data of the current client, handles the YT: command.
 * Emits the next lines from the chatscript until DONE: is reached or the
 * end of the chatscript is reached.
 *
 * Any line containing invalid commands are ignored.
 */
static void client_yt_handler(ClientData *data) {
    /* If DONE: was reached in the script on this turn of YT */
    bool done = false;
    /* If QUIT: was reached in the script on this turn of YT */
    bool normalExit = false;

    while(!done) {
        
        // Get the current line of the chatscript
        char *currentLine = get_script_line(data, data->scriptIndex);

        /* Read current command to LineList and set the invalidCmd flag if
           the command is invalid as per get_cmd_str() */
        bool invalidCmd = false;
        LineList *currentScriptCmd = get_cmd_str(currentLine, &invalidCmd);

        const int numValidCmds = 5;
        char *validCmds[] = {"NAME", "CHAT", "DONE", "KICK", "QUIT"};
        const int numCmdArgs[] = {2, 2, 1, 2, 1};
        char *cmdName = currentScriptCmd->lines[0];
        int cmdIndex = find_word(cmdName, validCmds, numValidCmds);

        // Check if current line is a valid command and emit it to stdout if so
        if (!invalidCmd && (cmdIndex > -1) &&
            (currentScriptCmd->numLines == numCmdArgs[cmdIndex])) {
            printf("%s\n", currentLine);
            fflush(stdout);
        } else {
            invalidCmd = true;
        }

        // Check if YT is done and/or client should exit
        if ((data -> scriptIndex + 1 == get_script_len(data)) ||
            (!strcmp(cmdName, "QUIT") && !invalidCmd)) {
            done = true;
            normalExit = true;
        } else if (!strcmp(cmdName, "DONE") && !invalidCmd) {
            done = true;
            next_script_index(data);
        } else {
            next_script_index(data);
        }

        free_line_list(currentScriptCmd);
    }

    // Make client exit if needed
    if (normalExit) {
        client_exit(data, NORMAL_EXIT, "no message");
    }
}
