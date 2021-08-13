#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "lineList.h"
#include "commands.h"
#include "serverUtils.h"

/* Enumerated enum values corresponding to valid commands a 
 * server can receive from clients.
 *
 * These values are equal to the index of the respective command
 * as a string in serverCmdWords in commands.c, or as returned by
 * the get_cmd_master function.
 */
typedef enum {
    CHAT,
    KICK,
    DONE,
    QUIT
} ServerCmds;

void handle_clients(ClientList *chatMembers);
int handle_client_cmd(ClientList *chatMembers, ClientInstance *client, 
        char *cmdLine);
void handle_client_quit(ClientList *chatMembers,
        ClientInstance *leavingClient);
void handle_client_kick(ClientList *chatMembers, char *kickedClientName);
void handle_client_chat(ClientList *chatMembers, ClientInstance *client,
        char *msg);
void send_and_handle_yt(ClientList *chatMembers, ClientInstance *client);
void negotiate_all_names(ClientList *chatMembers);
int negotiate_name(int clientIndex, ClientList *chatMembers);
ClientList *setup_server(int argc, char **argv);
static void suppress_sigpipe();

int main(int argc, char **argv) {
    /* Suppress the SIGPIPE signal so that the server does not exit if it
     * tries to communicate with bad clients, i.e. clients that have quit
     */
    suppress_sigpipe();

    // Setup all client processes
    ClientList *chatMembers = setup_server(argc, argv);
    // Perform name negotiation with every client
    negotiate_all_names(chatMembers);
   
    /* Communicate with clients as per the spec while there are active clients
     * left in the chat
     */
    while (count_active_clients(chatMembers) > 0) {
        handle_clients(chatMembers);
    }

    free_client_list(chatMembers);

    return 0;
}

/* Creates a new sigaction struct whose handler is SIG_IGN and sets this as
 * the handler for SIGPIPE to suppress it*/
static void suppress_sigpipe() {
    struct sigaction ignoreSignal;
    memset(&ignoreSignal, 0, sizeof(struct sigaction));
    ignoreSignal.sa_handler = SIG_IGN;
    ignoreSignal.sa_flags = SA_RESTART;
    sigaction(SIGPIPE, &ignoreSignal, 0);
}

/*
 * Handles one round of communications with each client in a given ClientList
 * chatMembers. Only communicates with clients that are active.
 *
 * If a client's name isn't set, name negotation is performed with the client,
 * else the command YT: is sent to the client and the client's reply handled
 */
void handle_clients(ClientList *chatMembers) {
    int numClients = chatMembers->numClients;

    for (int i = 0; i < numClients; ++i) {
        // Get ith client
        ClientInstance *currentClient = chatMembers->clients[i];

        // Communicate with currentClient if it is active
        if (currentClient->isActive) {
            // Send YT to client and handle the client's reply
            send_and_handle_yt(chatMembers, currentClient);
        }
    }   
}

/* Sends the command YT: to a specified client and handles its reply, i.e.
 * executes commands the client might give to the server, makes server stop
 * communicating with the client if it gives a invalid command.
 */
void send_and_handle_yt(ClientList *chatMembers, ClientInstance *client) {
    // Sent YT to the client
    send_client(client, "YT:\n");
    int clientStatus;
    
    while (1) {
        // Read the next line of the client's reply
        char *reply = read_client_line(client, NULL);

        clientStatus = handle_client_cmd(chatMembers, client, reply);
        if (clientStatus != 0) {
            // clientStatus = -1 is an invalid command, so deactivate client
            if (clientStatus < 0) {
                //client->isActive = false;
                handle_client_quit(chatMembers, client);
            }
            /* clientStatus indicates client sent DONE: or QUIT:, just break
             * the loop
             */
            free(reply);
            break;
        }
        
        free(reply);
    }
}

/* Handles a single command (given as the string cmdLine) from the reply of a 
 * client to the client being sent YT: by the server.
 *
 * The arguments are:
 * chatMembers: clients currently in the server
 * client: the client that sent the command
 * cmdLine: the given command
 *
 * -1 is returned if the command is invalid or the command was empty.
 *  1 is returned if the command was DONE or QUIT.
 *  Else 0 is returned.
 */
int handle_client_cmd(ClientList *chatMembers, ClientInstance *client,
        char *cmdLine) {

    // Create LineList representation of cmd 
    bool invalidCmd = false;
    LineList *cmd = get_cmd_str(cmdLine, &invalidCmd);

    // Check if the command is empty
    if (cmd->numLines < 1) {
        free_line_list(cmd);
        return -1;
    }

    /* Number of arguments expected, including the command's name, for the
     * commands CHAT, KICK, DONE and QUIT respectively
     */
    const int numCmdArgs[] = {2, 2, 1, 1};
    int cmdNum = get_cmd(cmd->lines[0], SERVER);


    int returnFlag = 0;
    // Check cmd is not invalid and number of arguments for the cmd is correct
    if (invalidCmd || cmdNum < 0 || numCmdArgs[cmdNum] != cmd->numLines) {
        returnFlag = -1;
    } else if (cmdNum == CHAT) {
        handle_client_chat(chatMembers, client, cmd->lines[1]);
    } else if (cmdNum == KICK) {
        handle_client_kick(chatMembers, cmd->lines[1]);
    } else if (cmdNum == DONE) {
        returnFlag = 1;
    } else if (cmdNum == QUIT) {
        handle_client_quit(chatMembers, client);
        returnFlag = 1;
    }

    free_line_list(cmd);

    return returnFlag;
}

/* Sets a client to inactive and sends the message 
 * LEFT:<name of client that quit> to all other active clients in 
 * the ClientList chatMembers.
 *
 * Also emits (<name> has left the chat) to stdout of the server
 */
void handle_client_quit(ClientList *chatMembers,
        ClientInstance *leavingClient) {
    leavingClient->isActive = false;
    char *msg = calloc(strlen("LEFT:\n") + strlen(leavingClient->name) + 1,
            sizeof(char));
    sprintf(msg, "LEFT:%s\n", leavingClient->name);
    send_all(chatMembers, msg, -1);
    free(msg);

    printf("(%s has left the chat)\n", leavingClient->name);
}

/* Sends the command KICK: to the client in chatMembers who's name is equal to 
 * the given string kickedClientName. If such a client does not exist, this
 * function does nothing.
 */
void handle_client_kick(ClientList *chatMembers, char *kickedClientName) {
    for (int i = 0; i < chatMembers->numClients; ++i) {
        ClientInstance *client = chatMembers->clients[i];
        if (client->isActive && client->name != NULL) {
            if (!strcmp(client->name, kickedClientName)) {
                send_client(client, "KICK:\n");
            }
        }
    }
}

/* Sends the message CHAT:<name>:msg to all active clients in chatMembers.
 * Also emits (<name>) message to stdout of the server
 *
 * Note that <name> is the name of client
 */
void handle_client_chat(ClientList *chatMembers, ClientInstance *client,
        char *msg) {
    char *serverMsg = calloc(
            strlen("MSG::\n") + strlen(client->name) + strlen(msg) + 1,
            sizeof(char));
    sprintf(serverMsg, "MSG:%s:%s\n", client->name, msg);
    send_all(chatMembers, serverMsg, -1);
    free(serverMsg);

    printf("(%s) %s\n", client->name, msg);

}

/* Command line arguments to a server are passed to this function to set up
 * all clients specified in its configfile.
 * 
 * Function exits with code 1 if an incorrect number of commandline args 
 * are given or if the configfile could not be opened.
 *
 * Otherwise, for each valid line in its configfile, a client process is
 * created as per the given arguments in that line and a ClientInstance struct
 * corresponding to that client is created.
 *
 * Each ClientInstance struct created like so is added to a ClientList struct 
 * that is returned by this function.
 */
ClientList *setup_server(int argc, char **argv) {
    FILE *configFile;
    if (argc != 2 || (configFile = fopen(argv[1], "r")) == NULL) {
        fprintf(stderr, "Usage: server configfile\n");
        fflush(stderr);
        exit(1);
    }

    LineList *configLines = file_to_line_list(configFile);
    ClientList *chatMembers = init_clients_from_lines(configLines);
    free_line_list(configLines);
    fclose(configFile);

    return chatMembers;
}

/* Performs name negotiation on each client in a ClientList to set all their
 * names. If a client replies with an invalid command during name negotiation,
 * that client is deactived and name negotiation performed on the next client
 * in the ClientList.
 */
void negotiate_all_names(ClientList *chatMembers) {
    for (int i = 0; i < chatMembers->numClients; ++i) {
        ClientInstance *client = chatMembers->clients[i];
        /* Name negotiate with the client until its name is set or it is
         * deactivated. (i.e. if it sends an invalid command as its reply)
         */
        while (client->isActive && client->name == NULL) {
            negotiate_name(i, chatMembers);
        }
    }
}

/* Performs name negotiation on a client (a ClientInstance struct) in an 
 * existing ClientList struct chatMembers. 
 * clientIndex is the index of the client in chatMembers.
 *
 * If the name member of the client is not null, the function returns -1, else
 * the command WHO: is send to the client and a reply from the client with its
 * name is listened for.
 *
 * If there is already a client with the same name in the 
 * chatMembers, NAME_TAKEN: is sent to the client else the client's name is set
 * as the name it gave.
 *
 * In both of these cases, this function returns 0 instead.
 */
int negotiate_name(int clientIndex, ClientList *chatMembers) {
    
    ClientInstance *client = chatMembers->clients[clientIndex];
    // Check if the client's name is already set.
    if (client->name != NULL) {
        return -1;
    }

    // Send WHO: and wait for the client to reply with its name
    send_client(client, "WHO:\n");
    char *reply = read_client_line(client, NULL);
    LineList *cmd = get_cmd_str(reply, NULL);

    char *clientName;
    
    if ((cmd->numLines != 2) || strcmp(cmd->lines[0], "NAME")) {
        client->isActive = false;
    } else if (find_client_index(chatMembers,
            clientName = cmd->lines[1]) < 0) {
        // Set the clients name if there isn't another client with that name
        set_client_name(client, clientName);
        printf("(%s has entered the chat)\n", clientName);
        fflush(stdout);
    } else {
        // else send NAME_TAKEN:
        send_client(client, "NAME_TAKEN:\n");
    }

    free(reply);
    free_line_list(cmd);

    return 0;
}
