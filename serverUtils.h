#ifndef SERVERUTILS_H
#define SERVERUTILS_H

#include <stdio.h>
#include <stdbool.h>
#include "lineList.h"

/* Struct for storing information pertaining to a client child process of the
 * current server.
 *
 * readEnd and writeEnd are file object wrappers for the file descriptors of
 * the pipe ends which allow the server to read from and write to the client 
 * process respectively.
 *
 * name is the name of the client process (i.e. client, clientbot0 etc..).
 *
 * isActive is a bool flag that is initialized to true and set to false only
 * if the client has left the server.
 */
typedef struct {
    /* File pointer to the pipe end the server uses to read the client's stdout
     */
    FILE *readEnd;
    /* File pointer to the pipe end the server uses to write to the client's
     * stdin */
    FILE *writeEnd;
    /* Name of the client */
    char *name;
    /* Whether or not the client is active */
    bool isActive;
} ClientInstance;

/* Struct for storing information pertaining to every client that was in the
 * server when the server was started.
 *
 * clients is an array of ClientInstance structs storing info of each client.
 * numClients is the total number of clients in the server at the start of the
 * server.
 */
typedef struct {
    /* Array of all ClientInstances for each client in the server */
    ClientInstance **clients;
    /* Number of clients in the server */
    int numClients;
} ClientList;

ClientInstance *new_client_instance(LineList *cmd);
void free_client_instance(ClientInstance *client);
void send_client(ClientInstance *client, char *msg);
char *read_client_line(ClientInstance *client, bool *isLineEmpty);
void set_client_name(ClientInstance *client, char *name);
int find_client_index(ClientList *chatMembers, char *name);
ClientList *init_client_list();
void free_client_list(ClientList *chatMembers);
void add_client_instance(ClientList *chatMembers, ClientInstance *newClient);
int count_active_clients(ClientList *chatMembers);
void send_all(ClientList *chatMembers, char *msg, int excludedIndex);
ClientList *init_clients_from_lines(LineList *configLines);

#endif
