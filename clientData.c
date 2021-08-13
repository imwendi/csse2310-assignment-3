#include <stdio.h>
#include <stdlib.h>
#include "lineList.h"
#include "clientbotUtils.h"
#include "clientData.h"

/* Initializes a new clientData structure given the name of the client and
 * returns a pointer to it.
 *
 * scriptIndex starts at 0 and is incremented by the client's YT command
 * handler each time a new line from the script is handled.
 */
ClientData *init_client_data(ClientConfig givenConfig) {
    ClientData *data = (ClientData *) malloc(sizeof(ClientData));
    data->config = givenConfig;
    data->clientNo = -1;
    data->scriptIndex = 0;
    data->script = NULL;
    data->dict = NULL;
    data->buff = NULL;

    return data;
}

/* 
 * Returns the string stored at the index specified of the script member
 * (a LineList) of a given ClientData struct.
 */
char *get_script_line(ClientData *data, int index) {
    return data->script->lines[index];
}

/* Returns the number of strings stored in the script member of a given 
 * ClientData struct
 */
int get_script_len(ClientData *data) {
    return data->script->numLines;
}

/* Sets the script of a ClientData struct to a given LineList */
void set_client_script(LineList *script, ClientData *data) {
    data->script = script;
}

/* Increments the clientNo of a ClientData struct */
void next_client_no(ClientData *data) {
    data->clientNo++;
}

/* Increments the scriptIndex of a ClientData struct */
void next_script_index(ClientData *data) {
    data->scriptIndex++;
}

/* Frees memory allocated to a clientData structure */
void free_client_data(ClientData *data) {
    // Free each of the script, dict and buff that aren't null
    if (data->script != NULL) {
        free_line_list(data->script);
    }
    if (data->dict != NULL) {
        free_dict(data->dict);
    }
    if (data->buff != NULL) {
        free_buffer(data->buff);
    }

    free(data);
}
