#ifndef HANDLETABLE_H_
    #define HANDLETABLE_H_
    
    #include "uthash.h" 
    #include "pollLib.h"  

    struct handleTable {
        char handleName[100];
        char name[100];
        int handleSocket;
        UT_hash_handle hh;
    } handleTable;
    
    int findHandle(char *handle);
    void addHandle(int serverSocket);
    void printHandles();
    void removeHandle(int socket);
    int getAllSockets(int *tempSock, int sender);
    uint32_t getAllHandles(char *tempHandles[]);
    uint32_t getNumHandles();
    
#endif
