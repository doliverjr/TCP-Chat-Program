/******************************************************************************
* tcp_server.c
*
* CPE 464 - Program 1
*****************************************************************************/

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
#include "pdu.h"
#include "pollLib.h"
#include "handleTable.h"

#define MAXBUF 1400
#define MAXHANDLELENGTH 100


void recvFromClient(int clientSocket);
int checkArgs(int argc, char *argv[]);
void polling(int serverSocket);
void broadcast(int sender, char *pdu, int pduLen);
void getListHandles(int sender);
void distMessage(char *buf, int senderSocket);
void sendMessage(char *destHandle, int destSize, char *pdu, int pduLen, int senderSocket, int destSocket);

int main(int argc, char *argv[])
{
	int serverSocket = 0;   //socket descriptor for the server socket
	int portNumber = 0;
	
	portNumber = checkArgs(argc, argv);
	
	//create the server socket
	serverSocket = tcpServerSetup(portNumber);
        polling(serverSocket);
	
	/* close the sockets */
	close(serverSocket);

	return 0;
}

void polling(int serverSocket)
{
    int clientSocket = 0;
    setupPollSet();
    addToPollSet(serverSocket);
    
    while(13)
    {
        clientSocket = pollCall(-1);
        
        if (clientSocket == serverSocket)
        {
            addHandle(serverSocket);
        } else {
            recvFromClient(clientSocket);
        }
    }

}

void closedClient(int clientSocket)
{
    printf("Client Socket Number %d Closed\n", clientSocket);
    removeHandle(clientSocket);
}

void recvFromClient(int clientSocket)
{
    char buf[MAXBUF];
    int msgLen = 0;
    int flag = 0;

    //now get the data from the client_socket
    msgLen = recvPDU(buf, clientSocket);
    flag = buf[2];

    if (!msgLen)
    {
        closedClient(clientSocket);
    } else if (flag == 8) {
        msgLen = createFlagPDU(buf, 9);
        sendPDU(buf, msgLen, clientSocket);
        closedClient(clientSocket);
    } else if (flag == 4) {
        broadcast(clientSocket, buf, msgLen);
    } else if (flag == 10) {    
        getListHandles(clientSocket);
    } else if (flag == 5) {
        distMessage(&buf[3], clientSocket);
    } else {
        closedClient(clientSocket);
    } 
        
    memset(buf, 0, MAXBUF);//clear buf
}

void distMessage(char *buf, int senderSocket)
{
   int senderSize = buf[0];
   int destSize = 0;
   int numHandles = 0;
   int offset = 1;
   int pduLen = 0;
   int i=0;
   int flag = 5;
   int destSocket = 0;

   char senderHandle[MAXHANDLELENGTH];
   char destHandle[MAXHANDLELENGTH];
   char message[200];
   char pdu[MAXBUF];

   //setup strings for memcpy
   memset(senderHandle, 0, MAXHANDLELENGTH);
   memset(destHandle, 0, MAXHANDLELENGTH);
   memset(message, 0, 200);
   memset(pdu, 0, MAXBUF);

 
   //get handle name and number of destinations
   memcpy(senderHandle, &buf[offset], senderSize);
   offset += senderSize;
   memcpy(&numHandles, &buf[offset], 1);
   offset ++;
   
   //getting message
   for (i=0; i<numHandles; i++)
   {
       memcpy(&destSize, &buf[offset], 1);
       offset ++;
       offset += destSize;
   }
   
   //create message pdu
   strcpy(message, &buf[offset]);
   memcpy(pdu, &flag, 1);
   memcpy(&pdu[1], &senderSize, 1);
   memcpy(&pdu[2], senderHandle, senderSize);
   strcat(pdu, message);
   pduLen = strlen(pdu)+1;
   pduLen = createPDU(pdu, pduLen);
   
   //loop through handles sending pdu created above if the handle exists
   offset = 2 + senderSize;
   for (i=0; i<numHandles; i++)
   {
       memcpy(&destSize, &buf[offset], 1);
       offset++;
       memcpy(destHandle, &buf[offset], destSize);
 
       destHandle[destSize] = '\0';
       offset += destSize;
   
       destSocket = findHandle(destHandle);
       sendMessage(destHandle, destSize, pdu, pduLen, senderSocket, destSocket);
   }
}

void sendMessage(char *destHandle, int destSize, char *pdu, int pduLen, int senderSocket, int destSocket)
{
   int errPDULen = 0;
   char errPDU[MAXBUF];
   int flag = 7;
   
   if (destSocket)
   {
       sendPDU(pdu, pduLen, destSocket);
   } else {
       //create error pdu with flag = 7
       memset(errPDU, 0, MAXBUF);
       memcpy(errPDU, &flag, 1);
       memcpy(&errPDU[1], &destSize, 1);
       strcat(errPDU, destHandle);
       errPDULen = strlen(errPDU);
       errPDULen = createPDU(errPDU, errPDULen);
       sendPDU(errPDU, errPDULen, senderSocket);
   }
}

void getListHandles(int sender)
{
    char *handleNames[getNumHandles()]; //creates array of strings for handles to be copied into
    uint32_t numHandles = 0;
    uint32_t numNET = 0;
    char buf[MAXBUF];
    int pduLen = 0;
    int i=0;
    int sent;
    int handleSize = 0;
    
    numHandles = getAllHandles(handleNames);
    buf[0] = 11;

    //convert host to net order
    numNET = htonl(numHandles);
    memcpy(&buf[1], &numNET, 4);

    pduLen = createPDU(buf, 5);
    sendPDU(buf, pduLen, sender);
    
    for (i=0; i<numHandles; i++)
    {
        buf[0] = 12;
        handleSize = strlen(handleNames[i]);
        buf[1] = handleSize;
        strcpy(&buf[2], handleNames[i]);
        pduLen = createPDU(buf, strlen(buf)+2);
        sendPDU(buf, pduLen, sender);
    }
    
    pduLen = createFlagPDU(buf, 13);
    sent = sendPDU(buf, pduLen, sender);
    
    if (sent == 0)
    {
        closedClient(sender);
    }
}

void broadcast(int sender, char *pdu, int pduLen)
{
    int numClients = 0;
    int sockets[10];
    int i=0;
    int sent=0;
    
    numClients = getAllSockets(sockets, sender);
    
    for (i=0; i<numClients; i++)
    {
        sent = sendPDU(pdu, pduLen, sockets[i]);
        if (sent==0)
        {
            closedClient(sockets[i]);
        }
    }
}

int checkArgs(int argc, char *argv[])
{
	// Checks args and returns port number
	int portNumber = 0;

	if (argc > 2)
	{
		fprintf(stderr, "Usage %s [optional port number]\n", argv[0]);
		exit(-1);
	}
	
	if (argc == 2)
	{
		portNumber = atoi(argv[1]);
	}
	
	return portNumber;
}
    

