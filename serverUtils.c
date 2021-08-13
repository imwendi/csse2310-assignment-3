#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include "lineList.h"
#include "commands.h"
#include "serverUtils.h"

/* Uses fork and exec to create a new client child-process given a valid
 * configfile command stored in a LineList struct cmd. cmd is assumed to
 * be storing two strings, the clientProgram and clientScript where:
 *
 * clientProgram: path to the client program
 * (i.e. client, clientbot)
 *
 * clientScript: argument to be passed into the client program
 * (for clients this is the path to its chatscript whilst for clientbots it is
 * the path to its reponsefile)
 *
 * A new ClientInstance struct for this client is initialized and read and 
 * write pipes to this new process are created, connected to the calling
 * process, and saved to this struct as FILE * objects.
 *
 * Finally, a pointer to this struct is then returned.
 *
 */
ClientInstance *new_client_instance(LineList *cmd) {
    ClientInstance *newClient;

    /* Create two pipes, one for reading from the client, the other for
     * writing to it
     */
    int readPipe[2];
    int writePipe[2];
    pipe(readPipe);
    pipe(writePipe);

    if (fork()) {
        //Create new ClientInstance struct
        newClient = (ClientInstance *) malloc(sizeof(ClientInstance));
        newClient->name = NULL;
        newClient->isActive = true;

        /* Close unneeded pipe ends and open the read end of readPipe and
         * write end of writePipe as files.
         */
        close(readPipe[1]);
        close(writePipe[0]);
        newClient->readEnd = fdopen(readPipe[0], "r");
        newClient->writeEnd = fdopen(writePipe[1], "w");
    } else {
        /* Closed unneeded pipe ends and connected stdout/stdin to the
         * readPipe/writePipe respectively.
         */
        close(readPipe[0]);
        close(writePipe[1]);
        dup2(readPipe[1], 1);
        close(readPipe[1]);
        dup2(writePipe[0], 0);
        close(writePipe[0]);

        // dup2 stderr to the null device to suppress it
        int nullDevice = open("/dev/null", O_WRONLY);
        dup2(nullDevice, 2);
        
        char *clientProgram = cmd->lines[0];
        char *clientScript = cmd->lines[1];
        execlp(clientProgram, clientProgram, clientScript, NULL);

        // Free the cmd LineList if the execlp call failed
        free_line_list(cmd);
        exit(-1);
    }

    return newClient;
}

/* Frees memory allocated to a client. (a ClientInstance struct.)
 * Closes the readEnd and writeEnd FILE pointers as well as their underlying
 * file descriptors.
 *
 * Frees memory allocated to store the client's name.
 */
void free_client_instance(ClientInstance *client) {
    fclose(client->readEnd);
    fclose(client->writeEnd);
    free(client->name);
    free(client);
}

/* Sends a string msg to the stdin of the given client instance. */
void send_client(ClientInstance *client, char *msg) {
    fprintf(client->writeEnd, "%s", msg);
    fflush(client->writeEnd);
}

/* Reads a single line from stdout of a client and returns it as a string */
char *read_client_line(ClientInstance *client, bool *isLineEmpty) {
    return read_file_line(client->readEnd, isLineEmpty);
}

/* Sets the name of a ClientInstance struct client.
 * Allocates memory for and copies the given name into the name member of
 * client. It is assumed that the member was previously NULL, if not, there 
 * may be a memory leak.
 */
void set_client_name(ClientInstance *client, char *name) {
    client->name = calloc(strlen(name) + 1, sizeof(char));
    strcpy(client->name, name);
}

/* Finds the first client in a ClientList with a given name and returns its
 * index. Otherwise -1 is returned if such a client is not found
 */
int find_client_index(ClientList *chatMembers, char *name) {
    int foundIndex = -1;

    // Check the names of each client in chatMembers if it matches name
    for (int i = 0; i < chatMembers->numClients; ++i) {
        char *clientName = (chatMembers->clients[i])->name;
        // Do nothing if the client's name isn't set yet
        if (clientName == NULL) {
            ;
        } else if (clientName != NULL && !strcmp(clientName, name)) {
            foundIndex = i;
            break;
        }
    }

    return foundIndex;
}

/* Initializes and allocates memory for a new ClientList struct and returns
 * a pointer to it.
 */
ClientList *init_client_list() {
    ClientList *chatMembers = malloc(sizeof(ClientList));
    chatMembers->numClients = 0;
    chatMembers->clients = (ClientInstance **) malloc(0);

    return chatMembers;
}

/* Frees all memory allocated to a ClientList struct. */
void free_client_list(ClientList *chatMembers) {
    // Free memory allocated to each client in the ClientList
    for (int i = 0; i < chatMembers->numClients; ++i) {
        free_client_instance(chatMembers->clients[i]);
    }
    free(chatMembers->clients);
    free(chatMembers);
}

/* Adds a new ClientInstance * to an existing ClientList struct 
 * Allocates memory for the new pointer then adds it to the end of the 
 * clients array of the struct
 */
void add_client_instance(ClientList *chatMembers, ClientInstance *newClient) {
    chatMembers->clients = (ClientInstance **) realloc(chatMembers->clients,
            sizeof(ClientInstance *) * (chatMembers->numClients + 1));
    chatMembers->clients[chatMembers->numClients++] = newClient;
}

/* Counts and returns the number of active clients in a ClientList.
 */
int count_active_clients(ClientList *chatMembers) {
    int count = 0;

    for (int i = 0; i < chatMembers->numClients; ++i) {
        ClientInstance *client = chatMembers->clients[i];
        if (client->isActive) {
            count++;
        }
    }

    return count;
}

/* Sends a string msg to the stdin of all active clients in a given
 * ClientList except the client who's index is equal to excludedIndex.
 *
 * excludedIndex can be set to -1 to send msg to all active clients.
 */
void send_all(ClientList *chatMembers, char *msg, int excludedIndex) {
    for (int i = 0; i < chatMembers->numClients; ++i) {
        ClientInstance *currentClient = chatMembers->clients[i];
        if (currentClient->isActive && i != excludedIndex) {
            send_client(currentClient, msg);
        }
    }
}

/* Initializes a new ClientList given a configfile represented as a LineList
 * struct configLines.
 * (i.e. a LineList containing every individual line of the configfile)
 *
 * Checks if each line is in the format <program>:<arg> and if so, attempts to
 * start a new client process as per these arguments. Invalid or commented
 * lines are ignored.
 *
 * Returns a ClientList struct containing ClientInstance structs for each
 * client process started.
 */
ClientList *init_clients_from_lines(LineList *configLines) {
    ClientList *chatMembers = init_client_list();

    for (int i = 0; i < configLines->numLines; ++i) {
        LineList *cmd = get_cmd_str(configLines->lines[i], NULL);

        /* Check the current line is not a comment and has the correct number
         * of arguments.
         */
        if (!is_comment(configLines->lines[i]) && cmd->numLines == 2) {
            add_client_instance(chatMembers, new_client_instance(cmd));
        }
        free_line_list(cmd);
    }

    return chatMembers;
}