#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lineList.h"
#include "clientbotUtils.h"
#include "genericClient.h"

static void clientbot_handle_msg_n_left(ClientData *data, LineList *cmd);
static void clientbot_yt_handler(ClientData *data);

int main(int argc, char **argv) {
    // Set up client config
    ClientConfig config;
    config.defaultName = "clientbot";
    config.msgAndLeftHandler = clientbot_handle_msg_n_left;
    config.ytHandler = clientbot_yt_handler;

    // Declare the ClientData struct data to be used by the client
    ClientData *data = init_client_data(config);

    LineList *responsefile = setup_client(data, argc, argv);
    /* Initialize dict to a ResponseDict struct representation of the given
     * responsefile.
     */
    data->dict = lines_to_dict(responsefile);
    
    // Initialize buff
    data->buff = init_buffer();

    run_client(data);

    return 0;
}

/* Handler for the MSG: and LEFT: commands received from stdin given data
 * of the clientbot.
 * These are handled identically to the regular client (see handle_msg_and_left
 * in genericClient.c) except for MSG:, the message is also checked if it
 * contains a stimulus in the clientbot's responsefile and the corresponding
 * response is added to a ResponseBuffer and queued to be emitted to stdout
 * when the next YT: command is handled
 */
static void clientbot_handle_msg_n_left(ClientData *data, LineList *cmd) {
    /* If the command was MSG, check if the message is in the clientbot's
     * accepted stimuli, and if so, append the dict index of the corresponding
     * reply to buff. Clientbots should not reply to themselves, i.e. it does
     * not response to any clients with the same name as itself
     */
    
    char *botName = get_name(data);

    if (!strcmp(cmd->lines[0], "MSG") && strcmp(botName, cmd->lines[1])) {
        int responseIndex;
        if ((responseIndex = pattern_match_lines(cmd->lines[2],
                data->dict->stimuli)) > -1) {
            append_buffer(responseIndex, data->buff);
        }
    }

    free(botName);

    /* Finally handle MSG and LEFT as usual, i.e. emitting a message or left
     * message to stderr. This also frees the LineList cmd.
     */
    handle_msg_and_left(cmd);
}

/* Prints responses at all indices stored in the clientbot's response buffer
 * (buff) to stdout. Then frees buff and freshly reinitializes it.
 */
static void clientbot_yt_handler(ClientData *data) {
    for (int i = 0; i < data->buff->bufferLen; ++i) {
        printf("CHAT:%s\n",
                data->dict->responses->lines[data->buff->responses[i]]);
        fflush(stdout);
    }
    printf("DONE:\n");
    fflush(stdout);
    free_buffer(data->buff);
    data->buff = init_buffer();
}
