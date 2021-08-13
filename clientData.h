#ifndef CLIENTDATA_H
#define CLIENTDATA_H

#include "lineList.h"
#include "clientbotUtils.h"

typedef struct ClientConfig ClientConfig;
typedef struct ClientData ClientData;

/* Configuration and options for client like programs, i.e. client.c and
 * clientbot.h
 */
struct ClientConfig {
    /* Default/base name of the client, i.e. client for client.c and clientbot
     * for clientbot.c
     */
    char *defaultName;
    /* File path of chatscript for clients and responsefile for clientbots */
    void (*msgAndLeftHandler)(ClientData *, LineList *);
    /* Function pointer to function for handling the YT: command given client
     * data
     */
    void (*ytHandler)(ClientData *data);
};

struct ClientData {
    /* Configuration info of the current client */
    ClientConfig config;
    /* Number to appended to name of a client to differentiate from other
     * clients in the server with the same base name.
     * starts at -1 (no number) and increments to 0, 1, 2... each time the
     * client receives a NAME_TAKEN command.
     */
    int clientNo;
    /* LineList representation of either a chatscript or responsefile */
    LineList *script;
    /* Current line processed of a chatscript, not used by clientBot */
    int scriptIndex;
    /* used by clientbot ONLY:
     * ResponseDict struct containing the current clientbot's stimuli and
     * responses as given in its responsefile.
     */
    ResponseDict *dict;
    /* used by clientbot ONLY:
     * ResponseBuffer struct containing ResponseDict indices of all lines for
     * a clientbot to output on the next YT: commands it receives.
     */
    ResponseBuffer *buff;
};

ClientData *init_client_data(struct ClientConfig givenConfig);
char *get_script_line(ClientData *data, int index);
int get_script_len(ClientData *data);
void set_client_script(LineList *script, ClientData *data);
void next_client_no(ClientData *data);
void next_script_index(ClientData *data);
void free_client_data(ClientData *data);

#endif
