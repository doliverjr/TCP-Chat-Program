#ifndef PDU_H_
    #define PDU_H_
    
    #include <stdio.h>
    #include <stdlib.h>
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
    
    int createPDU(char* msg, uint16_t msgLen);
    
    int sendPDU(char* pdu, int pduLen, int socketNum);
    int createFlagPDU(char *buf, uint8_t flag);
    int recvPDU(char* buf, int socketNum);
    
#endif
