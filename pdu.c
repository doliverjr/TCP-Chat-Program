#include "pdu.h"

#define MAXBUF 1024

int createPDU(char* msg, uint16_t msgLen)
{
    int i=msgLen;
    uint16_t netLen;
    
    msgLen = msgLen+2;
    //printf("creating PDU for: '%s'\n", msg); 
    
    for (i=(msgLen); i>0; i--)
    {
        msg[i] = msg[i-2];
    }
    
    //swap to network order and put in header
    netLen = htons(msgLen);
    memcpy(msg, &netLen, 2);
    
    return msgLen;
}

int sendPDU(char* pdu, int pduLen, int socketNum)
{
    int sent = send(socketNum, pdu, pduLen, 0);
    if (sent < 0)
    {
        perror("send call");
        exit(-1);
    }
    
    return sent;
}

int recvPDU(char* buf, int socketNum)
{
    uint16_t length = 0;
    int msgLen = 0;
  
    msgLen = recv(socketNum, buf, 2, MSG_WAITALL);  
    if (msgLen < 0)
    {
        perror("recv size call");
        exit(-1);
    }
    
    memcpy(&length, buf, 2);
    length = ntohs(length);
    
    msgLen = recv(socketNum, &(buf[2]), length-2, MSG_WAITALL);
    if (msgLen < 0)
    {
        perror("recv data call");
        exit(-1);
    }

    return length;
}

int createFlagPDU(char *buf, uint8_t flag)
{
    int len = 0;
    buf[1] = '\0';
    buf[0] = flag;
    
    len = createPDU(buf, 1);
    
    return len;
}



