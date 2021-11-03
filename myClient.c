/******************************************************************************
* myClient.c
*
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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
#include <ctype.h>

#include "networks.h"
#include "pdu.h"
#include "pollLib.h"

#define MAXBUF 1400
#define DEBUG_FLAG 1
#define MAXMSG 200

void sendToServer(int socketNum);
int readFromStdin(char * buffer);
void checkArgs(int argc, char * argv[]);
void sendHandle(int socketNum, char* handle); //sends handle to server
void setupHandle(int socketNum, char *handle); //gets handle from argv and checks if its valid
void sendBrodcast(char *sendBuf, int sendLen, int socketNum); //sends brodcast message
void readFlags(char *readBuf, int recvLen, int socketNum); //reads flags in packets recieved from the server
void sendMessage(char *sendBuf, int sendLen, int socketNum); //sends direct message
void printMessage(char *buf); //prints handle and message recieved
void checkSent(int sent); //make sure something was sent
void sendBrokenUp(char pdu[MAXBUF], int headerLen, char *msg, int socketNum); //breaks message into MAXMSG blocks and sends each

char *myHandle = NULL;
int myHandleLen = 0;

int main(int argc, char * argv[])
{
	int socketNum = 0;         //socket descriptor
	
	checkArgs(argc, argv);

	/* set up the TCP Client socket  */
	socketNum = tcpClientSetup(argv[2], argv[3], DEBUG_FLAG);
	setupHandle(socketNum, argv[1]);
	sendToServer(socketNum);
	
	return 0;
}

void handleFlag(int flag, char *handle)
{
    if (flag==2)
    {
        myHandle = handle;
        myHandleLen = strlen(handle);
    } else if (flag ==3) {
        printf("Handle already in use: %s\n", handle);
        exit(1);
    } else {
        printf("Unexpected handle flag %d\n", flag);
        exit(1);
    }
}

void setupHandle(int socketNum, char *handle)
{
    if (strlen(handle) > 100)
    {
        printf("Invalid handle, handle longer than 100 characters: %s\n", handle);
        exit(1);
    }else if (isdigit(handle[0]))
    {
        printf("Invalid handle, handle starts with a number\n");
        exit(1);
    } else {
        sendHandle(socketNum, handle);
    }
}

void sendHandle(int socketNum, char* handle)
{
    char initPDU[256];
    char recvBuf[MAXBUF];
    int pduLen = 0;
    int i = 0;
    int flag = 1;
    int handleLen = strlen(handle);
    
    initPDU[0] = flag;
    initPDU[1] = handleLen;
    
    for (i=0; i<handleLen; i++)
    {
        initPDU[i+2] = handle[i];
    }
    
    handleLen += 2;
    pduLen = createPDU(initPDU, handleLen);
    sendPDU(initPDU, pduLen, socketNum);
    recvPDU(recvBuf, socketNum);

    handleFlag(recvBuf[2], handle);
}

void commands(char *sendBuf, int sendLen, int socketNum)
{
    char pdu[MAXBUF];
    char cmd[3];
    int pduLen = 0;
    pdu[1] = '\0';

    memcpy(cmd, sendBuf, 2);
    cmd[2] = '\0';
    
    if(!strcmp(cmd, "%E") || !strcmp(cmd, "%e"))
    {
        pduLen = createFlagPDU(pdu, 8);
        sendPDU(pdu, pduLen, socketNum);
        recvPDU(sendBuf, socketNum);
        close(socketNum);
        printf("Exiting...\n");
        exit(0);
        
    } else if(!strcmp(cmd, "%B") || !strcmp(cmd, "%b")) {
        sendBrodcast(sendBuf, sendLen, socketNum);
    } else if(!strcmp(cmd, "%L") || !strcmp(cmd, "%l")) {
        pduLen = createFlagPDU(pdu, 10);
        checkSent(sendPDU(pdu, pduLen, socketNum));
    } else if(!strcmp(cmd, "%M") || !strcmp(cmd, "%m")) {
        sendMessage(sendBuf, sendLen, socketNum);
    } else {
        printf("Invalid command\n");
    }
    
}

void checkSent(int sent)
{
    if (sent == 0)
    {
        printf("Server Terminated\n");
        exit(1);
    }
}

void messagePDU(int numHandles, char *sendBuf, int socketNum)
{
    char pdu[MAXBUF];
    char msg[MAXBUF];
    int oldLen = 0;
    int newLen = 0;
    int i=0;
    int flag = 5;
    int valid = 1;
    
    memset(pdu, 0, MAXBUF);
    memset(msg, 0, MAXBUF);
    oldLen = myHandleLen;

    //put flag into pdu
    memcpy(pdu, &flag, 1);
        
    //put sender handle size into pdu
    memcpy(&pdu[1], &oldLen, 1);
        
    //put sender in pdu
    strcat(pdu, myHandle);
    oldLen += 2;
    memcpy(&pdu[oldLen], &numHandles, 1);
        
    sendBuf = strtok (&sendBuf[4], " ");
    while (i<numHandles && valid)
    {
        if (sendBuf == NULL)
        {
            printf("Not enough recipients\n");
            valid = 0;
        }
            
        if (strlen(sendBuf) > 100)
        {
            printf("Invalid handle, handle longer than 100 characters: %s\n", sendBuf);
            valid = 0;
        }
        newLen = strlen(sendBuf);
        oldLen += 1;
            
        //put length of destination name into pdu
        memcpy(&pdu[oldLen], &newLen, 1);
            
        // copy destination name into pdu
        strcat(pdu, sendBuf);
            
        //increase offset and number of handles put into pdu
        oldLen += newLen; 
        i++;
        sendBuf = strtok (NULL, " ");
    }
    
    while (sendBuf != NULL && valid)
    {
        strcat(msg, sendBuf);
        strcat(msg, " ");
        sendBuf = strtok (NULL, " ");
    }

    msg[strlen(msg)-1] = '\0';
    
    if (valid)
    {
        sendBrokenUp(pdu, oldLen+1, msg, socketNum);
    }
}

void sendMessage(char *sendBuf, int sendLen, int socketNum)
{
    int numHandles = 0;
    numHandles = atoi(&sendBuf[3]);
    
    if (numHandles > 9)
    {
        printf("To many recipients (9 max)\n");
    } else if (numHandles == 0) {
        printf("Please include number of recipients\n");
    } else {
        messagePDU(numHandles, sendBuf, socketNum);
    }
}

void sendBrokenUp(char pdu[MAXBUF], int headerLen, char *msg, int socketNum)
{
    int i=0;
    int pduLen = 0;
    int numLeft = strlen(msg);
    char currPDU[MAXBUF];

    //printf("Number of chars: %d\n", numLeft);

    for (i=0; i<strlen(msg)+1; i+=MAXMSG)
    {
        memcpy(currPDU, pdu, MAXBUF);
        if (numLeft > MAXMSG)
        {
            memcpy(&currPDU[headerLen], &msg[i], MAXMSG);
            currPDU[headerLen+MAXMSG] = '\0';
        } else {
            memcpy(&currPDU[headerLen], &msg[i], strlen(&msg[i])+1);
        }

        pduLen = strlen(currPDU);
        pduLen = createPDU(currPDU, pduLen+1);
        checkSent(sendPDU(currPDU, pduLen, socketNum));
        numLeft -= MAXMSG;
    }
}

void sendBrodcast(char *sendBuf, int sendLen, int socketNum)
{
    int flag = 4;
    char pdu[MAXBUF];
    
    memset(pdu, 0, MAXBUF);
    pdu[0] = flag;
    pdu[1] = myHandleLen;
    memcpy(&pdu[2], myHandle, myHandleLen);
   
    sendBrokenUp(pdu, 2+myHandleLen, &sendBuf[3], socketNum);
}

void sendToServer(int socketNum)
{
	char sendBuf[MAXBUF];   //data buffer
	int sendLen = 0;        //amount of data to send
	int readySocket;
	char readBuf[MAXBUF];
	int recvLen = 0;
	
        setupPollSet();
        addToPollSet(socketNum);
        addToPollSet(STDIN_FILENO);
        
        while(13)
        {    
            printf("$: ");
            fflush(STDIN_FILENO);
            readySocket = pollCall(-1);
            
            if (readySocket == STDIN_FILENO)
            {           
                sendLen = readFromStdin(sendBuf);
                if (sendLen > 1400)
                {
                    printf("Command exceeds 1400 character limit\n");
                } else {
                    commands(sendBuf, sendLen, socketNum);
                }

            } else if (readySocket == socketNum) {
                printf("\n");
                recvLen = recvPDU(readBuf, socketNum);
                readFlags(readBuf, recvLen, socketNum);
            }
            memset(readBuf, 0, MAXBUF);
        }
        
}

void readHandle(char *buf, int handleLen)
{
    int i=0;
    
    printf("\t");
    
    for (i=0; i<handleLen; i++)
    {
        printf("%c", buf[i]);
    }
    printf("\n");
}

void readFlags(char *readBuf, int recvLen, int socketNum)
{
    int flag = readBuf[2];
    int numHandles = 0;
    int i=0;
    
    if (!recvLen) {
        printf("Server Terminated\n");
        exit(1);
    } else if (flag == 11)
    {
        //convert from net order to host
        memcpy(&numHandles, &readBuf[3], 4);
        numHandles = ntohl(numHandles);

        printf("Number of clients: %d\n", numHandles); //MAKE NTOHS
        
        //flag = 12 packets
        for (i=0; i<numHandles; i++)
        {
            recvLen = recvPDU(readBuf, socketNum);
            readHandle(&readBuf[4], readBuf[3]);
        }
        
        //flag = 13 packet
        recvLen = recvPDU(readBuf, socketNum);

    } else if ((flag == 4) || (flag == 5)) {
        printMessage(&readBuf[3]);
    } else if (flag == 7) {
        printf("Client with handle ");
        for (i=0; i<readBuf[3]; i++)
        {
            printf("%c", readBuf[4+i]);
        }
        printf(" does not exist.\n");
    }
    
}

//takes in string pointing to the string of {1byte name length, name, message}
void printMessage(char *buf)
{
    int handleLen = buf[0];
    int i=0;
    
    for (i=0; i<handleLen; i++)
    {
        printf("%c", buf[i+1]);
    }
    
    printf(": %s\n", &buf[handleLen+1]);
}

int readFromStdin(char * buffer)
{
	char aChar = 0;
	int inputLen = 0;  
	      
	// Important you don't input more characters than you have space 
	buffer[0] = '\0';

	while (inputLen < (MAXBUF - 1) && aChar != '\n')
	{
		aChar = getchar();
		if (aChar != '\n')
		{
			buffer[inputLen] = aChar;
			inputLen++;
		}
	}
	
	// Null terminate the string
	buffer[inputLen] = '\0';
	inputLen++;
	
	return inputLen;
}

void checkArgs(int argc, char * argv[])
{
	/* check command line arguments  */
	if (argc != 4)
	{
		printf("usage: %s handle host-name port-number \n", argv[0]);
		exit(1);
	}
}
