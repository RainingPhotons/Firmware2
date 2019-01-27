#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <time.h>
#include <stdint.h>
#include <pthread.h>
#include "fx.c"
#define MAXLINE 8 

 #define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })
#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })     

// Driver code 
struct Position_t 
{
    int16_t iX;
    int16_t iY;
    int16_t iZ;
};

struct AvgPosition_t 
{
    int iXAvg;
    int iYAvg;
    int iZAvg;
    int iNumOfSamples;
};

struct MovementDelta_t 
{
    float fXDelta;
    float fYDelta;
    float fZDelta;
    int16_t iBoardState;//0Default, 1 = instigator, 2 = propogator
    char r, g, b;
};

struct StrandParam_t
{
    int iBoardAddr;
    int iBroadcast;
};

struct strand {
  int sock;
  int host;
};

pthread_mutex_t lStrandLock4;//TODO, turn it into array
pthread_t lStrand4;
struct MovementDelta_t sMovementDelta4;
const int iMovementTolaranceXY = 300; //TODO: make it unique per board?
const int iMovementTolaranceZ = 500; //TODO: make it unique per board?
#define TOTAL_STRANDS 20
#define ADDR_PREFIX 200

//Returns socket
int createConnection(struct StrandParam_t * psStrandParam) 
{
    struct sockaddr_in sServer;
    int iHost = psStrandParam->iBoardAddr + ADDR_PREFIX;
    int iSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (iSocket == -1) 
    {
        fprintf(stderr, "Could not create socket");
        return -1;
    }

    char acAddr[32];
    sprintf(acAddr, "192.168.1.%d", iHost);
    printf("Strand Addr, %s\n", acAddr);
    sServer.sin_addr.s_addr = inet_addr(acAddr);
    sServer.sin_family = AF_INET;
    sServer.sin_port = htons(5000);

    if (connect(iSocket, (struct sockaddr*)&sServer, sizeof(sServer)) < 0) 
    {
        perror("connect failed. Error");
        return -1;
    }
    return iSocket;
}

 void *strand(void *params)
 {
    struct StrandParam_t * psStrandParam = (struct StrandParam_t *) params; 
    int iBoardAddr = psStrandParam->iBoardAddr;
    printf("Strand address: %d created\n", iBoardAddr);
    int iSocket = createConnection(psStrandParam);
    if (iSocket < 0)
    {
        printf("Socket connection failed for strand, %d", iBoardAddr);
        pthread_exit(NULL);
    }
    char matrix[kLEDCnt * 3];

    for (int j = 0; j < kLEDCnt; ++j) 
    {
        matrix[j *3 + 0] = 0x10;
        matrix[j *3 + 1] = 0x0;
        matrix[j *3 + 2] = 0x0;
    }
    while(1)
    {
        pthread_mutex_lock(&lStrandLock4); 
        
        // if(abs(sMovementDelta4.fXDelta) > iMovementTolaranceXY ||
                // abs(sMovementDelta4.fYDelta) > iMovementTolaranceXY || 
                // abs(sMovementDelta4.fZDelta) > iMovementTolaranceZ)
        if (1 == sMovementDelta4.iBoardState)        
        {
            if(effectMeteor(iSocket, psStrandParam->iBroadcast, matrix) < 0)
            {
                printf("Error with meteor effect on strand, %d\n", iBoardAddr);
            }
            sMovementDelta4.fXDelta = 0.0f;
            sMovementDelta4.fYDelta = 0.0f;
            sMovementDelta4.fZDelta = 0.0f;
            sMovementDelta4.iBoardState = 0; // Set the board state to default after this
            //Start iterating through other boards and change their status
        }
        else if (2 == sMovementDelta4.iBoardState)
        {
            if(effectMeteor(iSocket, psStrandParam->iBroadcast, matrix) < 0) //TODO change it meteor down
            {
                printf("Error with meteor effect on strand, %d\n", iBoardAddr);
            }
            sMovementDelta4.fXDelta = 0.0f;
            sMovementDelta4.fYDelta = 0.0f;
            sMovementDelta4.fZDelta = 0.0f;
            sMovementDelta4.iBoardState = 0; // Set the board state to default after this
        }
        else
        {
            if(effectDefault(iSocket, psStrandParam->iBroadcast, matrix) < 0)
            {
                printf("Error with meteor effect on strand, %d\n", iBoardAddr);
            }
            sMovementDelta4.fXDelta = 0.0f;
            sMovementDelta4.fYDelta = 0.0f;
            sMovementDelta4.fZDelta = 0.0f;
        }
        pthread_mutex_unlock(&lStrandLock4);
        usleep(10000);
    }
    close(iSocket);
    pthread_exit(NULL);
 }

 int createBroadcast() 
 {
    struct sockaddr_in sServer;

    int iSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (iSock == -1) 
    {
        fprintf(stderr, "Could not create socket");
        return -1;
    }

    int iBroadcast = 1;
    if (setsockopt(iSock, SOL_SOCKET, SO_BROADCAST,
                 &iBroadcast, sizeof(iBroadcast)) == -1) 
    {
        perror("unable to broadcast");
        return -1;
    }

    sServer.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    sServer.sin_family = AF_INET;
    sServer.sin_port = htons(5000);

    if (connect(iSock, (struct sockaddr*)&sServer, sizeof(sServer)) < 0) 
    {
        perror("connect failed. Error");
        return -1;
    }

    return iSock;
}
 
int main() 
{ 
    int iSockfd; 
    char acBuffer[MAXLINE]; 
    struct sockaddr_in sServaddr; 
    int iIdx;
    int iThreadId;
    int iBroadcast;
    // Creating socket file descriptor 
    iBroadcast = createBroadcast();
    if ((iSockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) 
    { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
    
    // Filling server information 
    sServaddr.sin_addr.s_addr = INADDR_ANY;
    sServaddr.sin_family = AF_INET; 
    sServaddr.sin_port = htons(5002); 
    if (bind(iSockfd, (struct sockaddr*)&sServaddr, sizeof(sServaddr)) < 0) 
    {
        perror("connect to server failed");
        return 0;
    }
    printf("Connection to server sucessful\n");
    int n, len; 
    
    time_t tStartTime = time(NULL);
    time_t tCurrTime = time(NULL);
    int iCounter = 0;
    const int iCallibrationTimeOut = 10;
    struct AvgPosition_t asDefaultPosition[TOTAL_STRANDS];
    struct Position_t sTempPosition = {0};
    struct MovementDelta_t sMovement = {0};
    memset(&asDefaultPosition, 0, sizeof(asDefaultPosition));
    printf("Initializing strand position. Wait 10 seconds\n");
    while(iCounter < iCallibrationTimeOut)
    {
        if( 0 >read(iSockfd, (char *)acBuffer, MAXLINE))
            printf("error\n");
        tCurrTime = time(NULL);
        iCounter = (tCurrTime - tStartTime);
        int16_t iBoardAddr = (*((int16_t*)(&acBuffer[0]))) - ADDR_PREFIX;
        if(iBoardAddr >= TOTAL_STRANDS || iBoardAddr < 0)
        {
            printf("Board Adress Error. Exiting\n");
            return -1;
        }
        struct Position_t * psStrand = &sTempPosition;
        memcpy((void*)psStrand, (void*)&acBuffer[2], sizeof(struct Position_t));
        asDefaultPosition[iBoardAddr].iXAvg += psStrand->iX;
        asDefaultPosition[iBoardAddr].iYAvg += psStrand->iY;
        asDefaultPosition[iBoardAddr].iZAvg += psStrand->iZ;
        asDefaultPosition[iBoardAddr].iNumOfSamples += 1;
    }

    for(iIdx = 0; iIdx<TOTAL_STRANDS; iIdx++)
    {
        asDefaultPosition[iIdx].iXAvg = (int)((double)asDefaultPosition[iIdx].iXAvg / (double)max(1, asDefaultPosition[iIdx].iNumOfSamples));
        asDefaultPosition[iIdx].iYAvg = (int)((double)asDefaultPosition[iIdx].iYAvg / (double)max(1, asDefaultPosition[iIdx].iNumOfSamples));
        asDefaultPosition[iIdx].iZAvg = (int)((double)asDefaultPosition[iIdx].iZAvg / (double)max(1,asDefaultPosition[iIdx].iNumOfSamples));
        printf("Strand positions: Board, %d, x, %d, y, %d, z, %d\n", iIdx, 
        asDefaultPosition[iIdx].iXAvg,
        asDefaultPosition[iIdx].iYAvg,
        asDefaultPosition[iIdx].iZAvg);
    }
    printf("Strand initilization sucessful\n");
    //TODO: launch thread per strand
    {
        struct StrandParam_t sStrandParam;//TODO: remove this after done
        sStrandParam.iBoardAddr = 4;
        sStrandParam.iBroadcast = iBroadcast;
        if (pthread_mutex_init(&lStrandLock4, NULL) != 0)
        {
            printf("\n mutex init failed\n");
            return -1;
        }
        iThreadId = pthread_create(&lStrand4, NULL, strand, &sStrandParam);
        if(iThreadId)
        {
            printf("Thread create error, %d\n", iThreadId);
            return -1;
        }
    }
    while(1)
    {
        if( 0 >read(iSockfd, (char *)acBuffer, MAXLINE))
            printf("error\n");
        int16_t iBoardAddr = (*((int16_t*)(&acBuffer[0]))) - ADDR_PREFIX;
        if(iBoardAddr >= TOTAL_STRANDS || iBoardAddr < 0)
        {
            printf("Board Adress Error. Exiting\n");
            return -1;
        }
        struct Position_t * psStrand = &sTempPosition;
        memcpy((void*)psStrand, (void*)&acBuffer[2], sizeof(struct Position_t));
        sMovement.fXDelta = (float)(asDefaultPosition[iBoardAddr].iXAvg - psStrand->iX);
        sMovement.fYDelta = (float)(asDefaultPosition[iBoardAddr].iYAvg - psStrand->iY);
        sMovement.fZDelta = (float)(asDefaultPosition[iBoardAddr].iZAvg - psStrand->iZ);
        
        if(abs(sMovement.fXDelta) > iMovementTolaranceXY ||
            abs(sMovement.fYDelta) > iMovementTolaranceXY || 
            abs(sMovement.fZDelta) > iMovementTolaranceZ)
        {
            // printf("Movement: Board, %d, x, %f, y, %f, z, %f\n", iBoardAddr, 
            // sMovement.fXDelta,
            // sMovement.fYDelta,
            // sMovement.fZDelta);
            pthread_mutex_lock(&lStrandLock4); 
            sMovementDelta4.fXDelta = sMovement.fXDelta;
            sMovementDelta4.fYDelta = sMovement.fYDelta;
            sMovementDelta4.fZDelta = sMovement.fZDelta;
            sMovementDelta4.iBoardState = 1;
            pthread_mutex_unlock(&lStrandLock4);
        }
        else
        {
            //Default
        }
    }
    close(iSockfd);
    {    
        pthread_exit(NULL);
        pthread_mutex_destroy(&lStrandLock4);
    }
    close(iBroadcast);
    return 0; 
} 
