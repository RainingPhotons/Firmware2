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
    int iBoardPhysicalLoc;
    int iBroadcast;
};

struct strand {
  int sock;
  int host;
};
#define ACTIVE_STRANDS 3
#define TOTAL_STRANDS 20
int m_aiActiveStrands[ACTIVE_STRANDS] = {18,5,12};//{9,3,1,19,6,4,17,12,14,15,18,5};
int m_aiStrandsToLoc[TOTAL_STRANDS] = {-1, -1, -1, -1,  //[0,3]
                                                                     -1, 1, -1, -1,   //[4,7]
                                                                     -1, -1, -1, -1, //[8,11]
                                                                     2, -1, -1, -1,  //[12,15]
                                                                    -1, -1, 0, -1};  //[16,19]
pthread_mutex_t m_alStrandLock[ACTIVE_STRANDS];//TODO, turn it into array
pthread_t m_atStrand[ACTIVE_STRANDS];
struct MovementDelta_t m_asMovementDelta[ACTIVE_STRANDS];

const int iMovementTolaranceXY = 200; //TODO: make it unique per board?
const int iMovementTolaranceZ = 400; //TODO: make it unique per board?
#define ADDR_PREFIX 200
//Returns socket
int createConnection(int iBoardAddr) 
{
    struct sockaddr_in sServer;
    int iHost = iBoardAddr + ADDR_PREFIX;
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
    int iBoardPhysicalLoc = psStrandParam->iBoardPhysicalLoc;
    free(params);
    if (m_aiStrandsToLoc[iBoardAddr] < 0 || m_aiStrandsToLoc[iBoardAddr] > TOTAL_STRANDS)
    {
        printf("Error! Board Physical location invalid: board number, %d, physical location, %d\n", iBoardAddr, m_aiStrandsToLoc[iBoardAddr]);
        pthread_exit(NULL);
    }
    
    struct MovementDelta_t * psStrandMove = &m_asMovementDelta[m_aiStrandsToLoc[iBoardAddr]];
    pthread_mutex_t * psStrandMutex = &m_alStrandLock[m_aiStrandsToLoc[iBoardAddr]];
    
    printf("Strand address: %d created\n", iBoardAddr);
    int iSocket = createConnection(iBoardAddr);
    int iIdx;
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
        pthread_mutex_lock(psStrandMutex); 
        
        if (1 == psStrandMove->iBoardState)        
        {
            if(effectMeteor(iSocket, 0, matrix) < 0)
            {
                printf("Error with meteor effect on strand, %d\n", iBoardAddr);
            }
            psStrandMove->fXDelta = 0.0f;
            psStrandMove->fYDelta = 0.0f;
            psStrandMove->fZDelta = 0.0f;
            psStrandMove->iBoardState = 0; // Set the board state to default after this
            //Start iterating through other boards and change their status
            pthread_mutex_unlock(psStrandMutex);
             for(iIdx = iBoardPhysicalLoc - 1; iIdx >= 0 ; iIdx--)
            {
                pthread_mutex_lock(&m_alStrandLock[iIdx]);
                printf("%d, A-Setting meteor down to strands, %d, boardNumber, %d\n", iBoardAddr,iIdx, m_aiActiveStrands[iIdx]);
                m_asMovementDelta[iIdx].iBoardState = (0 == m_asMovementDelta[iIdx].iBoardState) ? 2: m_asMovementDelta[iIdx].iBoardState;
                pthread_mutex_unlock(&m_alStrandLock[iIdx]);
            }
            for(iIdx = iBoardPhysicalLoc + 1; iIdx<ACTIVE_STRANDS;iIdx++)
            {
                printf("%d, B-Setting meteor down to strands, %d, boardNumber, %d\n",iBoardAddr, iIdx, m_aiActiveStrands[iIdx]);
                pthread_mutex_lock(&m_alStrandLock[iIdx]); 
                m_asMovementDelta[iIdx].iBoardState = (0 == m_asMovementDelta[iIdx].iBoardState) ? 2: m_asMovementDelta[iIdx].iBoardState;
                pthread_mutex_unlock(&m_alStrandLock[iIdx]);
            }
                for (int j = 0; j < kLEDCnt; ++j) 
                {
                    matrix[j *3 + 0] = 0x10;
                    matrix[j *3 + 1] = 0x0;
                    matrix[j *3 + 2] = 0x0;
                }

        }
        else if (2 == psStrandMove->iBoardState)
        {
            if(effectMeteorDown(iSocket, 0, matrix) < 0) //TODO change it meteor down
            {
                printf("Error with meteor effect 2 on strand, %d\n", iBoardAddr);
            }
            psStrandMove->fXDelta = 0.0f;
            psStrandMove->fYDelta = 0.0f;
            psStrandMove->fZDelta = 0.0f;
            psStrandMove->iBoardState = 0; // Set the board state to default after this
            pthread_mutex_unlock(psStrandMutex);
            for (int j = 0; j < kLEDCnt; ++j) 
            {
                matrix[j *3 + 0] = 0x10;
                matrix[j *3 + 1] = 0x0;
                matrix[j *3 + 2] = 0x0;
            }
        }
        else
        {
            if(effectDefault(iSocket, 0, matrix) < 0)
            {
                printf("Error with meteor effect on strand, %d\n", iBoardAddr);
            }
            psStrandMove->fXDelta = 0.0f;
            psStrandMove->fYDelta = 0.0f;
            psStrandMove->fZDelta = 0.0f;
            pthread_mutex_unlock(psStrandMutex);
        }
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
    struct AvgPosition_t asDefaultPosition[ACTIVE_STRANDS];
    struct Position_t sTempPosition = {0};
    struct MovementDelta_t sMovementTemp = {0};
    memset(&asDefaultPosition, 0, sizeof(asDefaultPosition));
    printf("Initializing strand position. Wait 10 seconds\n");
    while(iCounter < iCallibrationTimeOut)
    {
        if( 0 >read(iSockfd, (char *)acBuffer, MAXLINE))
            printf("error\n");
        tCurrTime = time(NULL);
        iCounter = (tCurrTime - tStartTime);
        int16_t iBoardAddr = (*((int16_t*)(&acBuffer[0]))) - ADDR_PREFIX;
        int16_t iBoardPhysicalLoc = m_aiStrandsToLoc[iBoardAddr];
        if(iBoardPhysicalLoc > ACTIVE_STRANDS || iBoardAddr < 0)
        {
            printf("Board Adress Error. Exiting\n");
            return -1;
        }
        struct Position_t * psStrand = &sTempPosition;
        memcpy((void*)psStrand, (void*)&acBuffer[2], sizeof(struct Position_t));
        asDefaultPosition[iBoardPhysicalLoc].iXAvg += psStrand->iX;
        asDefaultPosition[iBoardPhysicalLoc].iYAvg += psStrand->iY;
        asDefaultPosition[iBoardPhysicalLoc].iZAvg += psStrand->iZ;
        asDefaultPosition[iBoardPhysicalLoc].iNumOfSamples += 1;
    }

    for(iIdx = 0; iIdx<ACTIVE_STRANDS; iIdx++)
    {
        asDefaultPosition[iIdx].iXAvg = (int)((double)asDefaultPosition[iIdx].iXAvg / (double)max(1, asDefaultPosition[iIdx].iNumOfSamples));
        asDefaultPosition[iIdx].iYAvg = (int)((double)asDefaultPosition[iIdx].iYAvg / (double)max(1, asDefaultPosition[iIdx].iNumOfSamples));
        asDefaultPosition[iIdx].iZAvg = (int)((double)asDefaultPosition[iIdx].iZAvg / (double)max(1,asDefaultPosition[iIdx].iNumOfSamples));
        printf("Strand positions: Board, %d, x, %d, y, %d, z, %d\n", m_aiActiveStrands[iIdx], 
        asDefaultPosition[iIdx].iXAvg,
        asDefaultPosition[iIdx].iYAvg,
        asDefaultPosition[iIdx].iZAvg);
    }
    printf("Strand initilization sucessful\n");
    //Launch threads  and initialize mutex for each strand
    for(iIdx = 0; iIdx < ACTIVE_STRANDS; iIdx++)
    {
        struct StrandParam_t * psStrandParam = malloc(sizeof(struct StrandParam_t));
        psStrandParam->iBoardAddr = m_aiActiveStrands[iIdx];
        psStrandParam->iBoardPhysicalLoc = iIdx;
        psStrandParam->iBroadcast = iBroadcast;
        if (pthread_mutex_init(&m_alStrandLock[iIdx], NULL) != 0)
        {
            printf("\n mutex init failed\n");
            return -1;
        }
        
        iThreadId = pthread_create(&m_atStrand[iIdx], NULL, strand, psStrandParam);
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
        int iBoardPhysicalLoc = m_aiStrandsToLoc[iBoardAddr];
        if(iBoardPhysicalLoc > ACTIVE_STRANDS || iBoardAddr < 0)
        {
            printf("Board Adress Error, board ID %d, physical loc, %d. Exiting\n", iBoardAddr, iBoardPhysicalLoc);
            return -1;
        }
        struct Position_t * psStrand = &sTempPosition;
        memcpy((void*)psStrand, (void*)&acBuffer[2], sizeof(struct Position_t));
        sMovementTemp.fXDelta = (float)(asDefaultPosition[iBoardPhysicalLoc].iXAvg - psStrand->iX);
        sMovementTemp.fYDelta = (float)(asDefaultPosition[iBoardPhysicalLoc].iYAvg - psStrand->iY);
        sMovementTemp.fZDelta = (float)(asDefaultPosition[iBoardPhysicalLoc].iZAvg - psStrand->iZ);
        
        if(abs(sMovementTemp.fXDelta) > iMovementTolaranceXY ||
            abs(sMovementTemp.fYDelta) > iMovementTolaranceXY || 
            abs(sMovementTemp.fZDelta) > iMovementTolaranceZ)
        {
            // printf("Movement Outside mutex: Board, %d, x, %f, y, %f, z, %f\n", iBoardAddr, 
            // sMovement.fXDelta,
            // sMovement.fYDelta,
            // sMovement.fZDelta);
             pthread_mutex_lock(&m_alStrandLock[iBoardPhysicalLoc]); 
            if( 1 != m_asMovementDelta[iBoardPhysicalLoc].iBoardState)
            {
                m_asMovementDelta[iBoardPhysicalLoc].fXDelta = sMovementTemp.fXDelta;
                m_asMovementDelta[iBoardPhysicalLoc].fYDelta = sMovementTemp.fYDelta;
                m_asMovementDelta[iBoardPhysicalLoc].fZDelta = sMovementTemp.fZDelta;
                m_asMovementDelta[iBoardPhysicalLoc].iBoardState = 1;
                //printf("Movement Inside mutes Board\n");
            }
            pthread_mutex_unlock(&m_alStrandLock[iBoardPhysicalLoc]);
        }
        else
        {
            //Default
        }
    }
    close(iSockfd);
    {    
        pthread_exit(NULL);
        for(iIdx = 0; iIdx < ACTIVE_STRANDS; iIdx++)
        {
            pthread_mutex_destroy(&m_alStrandLock[iIdx]);
        }
    }
    close(iBroadcast);
    return 0; 
} 
