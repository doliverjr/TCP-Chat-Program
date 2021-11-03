
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "networks.h"
#include "pdu.h"
#include "pollLib.h"
#include "handleTable.h"

#define DEBUG_FLAG 1
#define MAXBUF 1024

struct handleTable *Handles = NULL; //initialize hash table


int findHandle(char *handle)
{
    struct handleTable *s = NULL;
    
    HASH_FIND_STR(Handles, handle, s);
    if (s == NULL)
    {
        return 0;
    } else {
        return s->handleSocket;
    }
}

void getHandle(int socket, char* handle)
{
    char buf[MAXBUF];
    int flags = 0;
    int handleSize = 0;

    recvPDU(buf, socket);
    flags = buf[2];
    memcpy(&handleSize, &buf[3], 1);
    
    if (flags==1)
    {
        memcpy(handle, &buf[4], handleSize);
        memset(&handle[handleSize], 0, 1);
        //printf("handle: %s size: %d\n", handle, handleSize);
    }
}

int checkHandle(char *handle)
{
    int valid = 1;
    
    if (strlen(handle) > 100){
        valid = 0;
    } else if(findHandle(handle))
        valid = 0;
    else if (isdigit(handle[0]))
    {
        valid = 0;
    }
    
    return valid;
}

void addHandle(int serverSocket)
{
    struct handleTable *s = NULL;
    char handle[100];
    int flag = 2;
    char pdu[4];
    int pduLen = 0;
    int socket = tcpAccept(serverSocket, DEBUG_FLAG);
    
    pdu[1] = '\0';
    memset(handle, 0, 100);
    getHandle(socket, handle);

    
    
    if (checkHandle(handle))
    {
        s = (struct handleTable *)malloc(sizeof *s);
        //printf("%s || %d\n", handle, strlen(handle));

        memcpy(s->handleName, handle, strlen(handle)+1);
        
        s->handleSocket = socket;
        HASH_ADD_STR(Handles, handleName, s);
        addToPollSet(socket);
    } else {
        flag = 3;
        removeFromPollSet(socket);
    }
    
    pduLen = createFlagPDU(pdu, flag);
    sendPDU(pdu, pduLen, socket);
}

void removeHandle(int socket)
{
    struct handleTable *s = NULL;
    
    for (s = Handles; s != NULL; s = s->hh.next)
    {
        if (socket == s->handleSocket)
        {
            HASH_DEL(Handles, s);
            free(s);
            removeFromPollSet(socket);
        }
    }
}

void printHandles() {
    struct handleTable *s;
    for (s = Handles; s != NULL; s = s->hh.next) {
        printf("handle %s: socket %d\n", s->handleName, s->handleSocket);
    }
}

int getAllSockets(int *sockets, int sender)
{
    struct handleTable *s;
    int i=0;
    
    for (s = Handles; s != NULL; s = s->hh.next)  {
        if (s->handleSocket != sender)
        {
            sockets[i] = s->handleSocket;
            i++;
        }
    }
    
    return HASH_COUNT(Handles)-1;
}

uint32_t getNumHandles()
{
    return HASH_COUNT(Handles);
}

uint32_t getAllHandles(char *tempHandles[])
{
    struct handleTable *s;
    int i=0;
    
    for (s = Handles; s != NULL; s = s->hh.next)  {
        tempHandles[i] = s->handleName;
        i++;
    }
    
    return HASH_COUNT(Handles);
}



