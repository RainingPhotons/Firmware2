#include "fx.h"

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <stdint.h>
#include <pthread.h>
#define MAXLINE 8 

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
    int iMinX;
    int iMaxX;
    int iMinY;
    int iMaxY;
    int iMinZ;
    int iMaxZ;
    int iDiffX;
    int iDiffY;
    int iDiffZ;
    int iXTolarance;
    int iYTolarance;
    int iZTolarance;
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
};

struct strand {
  int sock;
  int host;
};

#define ACTIVE_STRANDS 16
#define TOTAL_STRANDS 20
#define ADDR_PREFIX 200
#define EXTRA_TOL 400

int m_aiActiveStrands[ACTIVE_STRANDS] = {9,3,1,19, //[0,3]
                                                                    6,4,17,14,// [4,7]
                                                                    15,10, 16,8,//[8,11]
                                                                    7,0,13,11};//[12,15]

int m_aiStrandsToLoc[TOTAL_STRANDS] = {13, 2, -1, 1,  //[0,3]
                                                                     5, -1, 4,12 ,   //[4,7]
                                                                     11, 0, 9, 15, //[8,11]
                                                                     -1, 14, 7, 8,  //[12,15]
                                                                    10, 6, -1, 3};  //[16,19]
pthread_mutex_t m_alStrandLock[ACTIVE_STRANDS];
pthread_t m_atStrand[ACTIVE_STRANDS];
struct MovementDelta_t m_asMovementDelta[ACTIVE_STRANDS];

uint8_t m_aiHueColor[ACTIVE_STRANDS][3];//R,G,B hues
uint8_t m_aiBrightness[ACTIVE_STRANDS]; //Sync brighness patterns
//Returns socket
pthread_barrier_t m_tSync;
int createConnection(int iBoardAddr) 
{
    struct sockaddr_in sServer;
    int iHost = iBoardAddr + ADDR_PREFIX;
    int iSocket = socket(AF_INET, SOCK_DGRAM, 0);
    char caAccSet[] = "a1";
    char caSampleRateSet[] = "s16";
    char caColorSet[] = "b12";
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
    if (send(iSocket, caColorSet, sizeof(caColorSet), 0) < 0) 
    {
        fprintf(stderr, "Send failed");
        return -1;
    }
    if (send(iSocket, caAccSet, sizeof(caAccSet), 0) < 0) 
    {
        fprintf(stderr, "Send failed");
        return -1;
    }
    if (send(iSocket, caSampleRateSet, sizeof(caSampleRateSet), 0) < 0) 
    {
        fprintf(stderr, "Send failed");
        return -1;
    }
    sleep(2);
    return iSocket;
}

void *playRain( void *params ) 
{
    char command[256];
    sprintf( command, "aplay -c 1 -q -t wav 401277__inspectorj__rain_lowVolNoFade.wav");
    int iStatus = 0;
    while(0 == iStatus)
    {
        iStatus = system( command );
    }
    pthread_exit(NULL);
}

void *playThunder( void *params ) 
{
    char command[256];
    usleep(5500000);
    sprintf( command, "aplay -c 1 -q -t wav thunderShort.wav" );

    system( command );
    pthread_exit(NULL);
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
    
    struct MovementDelta_t * psStrandMove = &m_asMovementDelta[iBoardPhysicalLoc];
    pthread_mutex_t * psStrandMutex = &m_alStrandLock[iBoardPhysicalLoc];
    pthread_t stThunderSound;
    //printf("Strand address: %d created\n", iBoardAddr);
    int iSocket = createConnection(iBoardAddr);
    int iIdx;
    if (iSocket < 0)
    {
        printf("Socket connection failed for strand, %d", iBoardAddr);
        pthread_exit(NULL);
    }
    uint8_t uR = 0x10;
    uint8_t uG = 0;
    uint8_t uB = 0;
     //pthread_mutex_lock(psStrandMutex); 
     uR = m_aiHueColor[iBoardPhysicalLoc][0];
     uG = m_aiHueColor[iBoardPhysicalLoc][1];
     uB = m_aiHueColor[iBoardPhysicalLoc][2];
     //pthread_mutex_lock(psStrandMutex); 
    uint8_t matrix[kLEDCnt * 3 + 1];//+1 for brightness

    for (int j = 0; j < kLEDCnt; ++j) 
    {
        matrix[j *3 + 0] = uR;
        matrix[j *3 + 1] = uG;
        matrix[j *3 + 2] = uB;
    }
    matrix[kLEDCnt * 3 ] = 50;
    int iRow = 0;
    int iDropSize = 2;
    int iTrailSize = 4;
    int iRainStart = iDropSize + iTrailSize;
    int iRandInt = rand();
    int iMeteorSize = 10;
    int iMeteorTrailSize = 20;
    
    while(1)
    {
        pthread_mutex_lock(psStrandMutex); 
        
        if (1 == psStrandMove->iBoardState)        
        {
            if(effectMeteor(iSocket, matrix, uR, uG, uB) < 0)
            {
                printf("Error with meteor effect on strand, %d\n", iBoardAddr);
            }
            psStrandMove->fXDelta = 0.0f;
            psStrandMove->fYDelta = 0.0f;
            psStrandMove->fZDelta = 0.0f;
            psStrandMove->iBoardState = 2; // Set the board state to default after this
            //Start iterating through other boards and change their status
            pthread_mutex_unlock(psStrandMutex);
            
           int iThreadId = pthread_create(&stThunderSound, NULL, playThunder, NULL);
           if(iThreadId)
           {
                printf("Thunder sound thread create error, %d\n", iThreadId);
           }
            
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
                    matrix[j *3 + 0] = uR;
                    matrix[j *3 + 1] = uG;
                    matrix[j *3 + 2] = uB;
                }
            usleep(12000);

        }
        else if (2 == psStrandMove->iBoardState)
        {
            if(effectMeteorPartial(iSocket, matrix, iRow/3,iMeteorSize, iMeteorTrailSize, iRandInt) < 0) //TODO change it meteor down
            {
                printf("Error with meteor effect 2 on strand, %d\n", iBoardAddr);
            }
            psStrandMove->fXDelta = 0.0f;
            psStrandMove->fYDelta = 0.0f;
            psStrandMove->fZDelta = 0.0f;
            iRow++;
           iRandInt = rand();

           if((iRow/3)>=  kLEDCnt + iMeteorSize + iMeteorTrailSize)
           {
               //int iHalt = pthread_barrier_wait(&m_tSync);
               effectThunder(iSocket, uR, uG, uB, matrix[kLEDCnt * 3]);
                // if(iHalt == PTHREAD_BARRIER_SERIAL_THREAD) 
                // {

                      // printf("THUNDER, %d\n", iBoardPhysicalLoc);
                // }
                // else if(iHalt != 0) 
                // {
                    // printf("Board Loc, %d, barrier sync error\n", iBoardPhysicalLoc);
                // }
               iRow = 0;
               psStrandMove->iBoardState = 0; // Set the board state to default after this

               for (int j = 0; j < kLEDCnt; ++j) 
                {
                    matrix[j *3 + 0] = uR;
                    matrix[j *3 + 1] = uG;
                    matrix[j *3 + 2] = uB;
                }
           }
           pthread_mutex_unlock(psStrandMutex);
           usleep(12000);

        }
        else
        {
            int iRandInt = rand();
            if(effectRainPartial(iSocket, matrix, uR, uG, uB, iDropSize, iTrailSize, iRainStart, iRandInt) < 0)
            {
                printf("Error with meteor effect on strand, %d\n", iBoardAddr);
            }
            psStrandMove->fXDelta = 0.0f;
            psStrandMove->fYDelta = 0.0f;
            psStrandMove->fZDelta = 0.0f;
            pthread_mutex_unlock(psStrandMutex);
            usleep(((iRandInt % 5) + 1) * 5500);

           if(iRainStart > 0)
               iRainStart --;
           if(0 == (iRandInt % 8))
               iRainStart = iDropSize + iTrailSize;
        }
        //pthread_mutex_lock(psStrandMutex); 
        uR = m_aiHueColor[iBoardPhysicalLoc][0];
        uG = m_aiHueColor[iBoardPhysicalLoc][1];
        uB = m_aiHueColor[iBoardPhysicalLoc][2];
        //pthread_mutex_lock(psStrandMutex); 
    }
    close(iSocket);
    pthread_exit(NULL);
 }
 
int main() 
{ 
    int iSockfd; 
    char acBuffer[MAXLINE]; 
    struct sockaddr_in sServaddr; 
    int iIdx;
    int iThreadId;
    pthread_t stRainSound;
    srand(time(0));
    struct timespec stStart, stCurr;

    // Creating socket file descriptor 
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
    
    int iCounter = 0;
    const int iCallibrationTimeOut = 5;
    struct AvgPosition_t asDefaultPosition[ACTIVE_STRANDS];
    struct Position_t sTempPosition = {0};
    struct MovementDelta_t sMovementTemp = {0};
    memset(&asDefaultPosition, 0, sizeof(asDefaultPosition));
    //Initializing Board values
    for(iIdx = 0; iIdx<ACTIVE_STRANDS; iIdx++)
    {
          asDefaultPosition[iIdx].iXAvg = 0;
          asDefaultPosition[iIdx].iYAvg = 0;
          asDefaultPosition[iIdx].iZAvg = 0;
          asDefaultPosition[iIdx].iNumOfSamples = 0;
          asDefaultPosition[iIdx].iMinX = 0x0fffffff;          
          asDefaultPosition[iIdx].iMaxX = 0;
          asDefaultPosition[iIdx].iMinY = 0x0fffffff;          
          asDefaultPosition[iIdx].iMaxY = 0;
          asDefaultPosition[iIdx].iMinZ = 0x0fffffff;          
          asDefaultPosition[iIdx].iMaxZ = 0;
          asDefaultPosition[iIdx].iDiffX = 0;
          asDefaultPosition[iIdx].iDiffY = 0;
          asDefaultPosition[iIdx].iDiffZ = 0;
    }
     time_t tStartTime = time(NULL);
     time_t tCurrTime = time(NULL);
    printf("Initializing strand position. Wait %d seconds\n", iCallibrationTimeOut);
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
        asDefaultPosition[iBoardPhysicalLoc].iMinX = min(asDefaultPosition[iBoardPhysicalLoc].iMinX, abs(psStrand->iX));
        asDefaultPosition[iBoardPhysicalLoc].iMaxX = max(asDefaultPosition[iBoardPhysicalLoc].iMaxX, abs(psStrand->iX));
        asDefaultPosition[iBoardPhysicalLoc].iMinY = min(asDefaultPosition[iBoardPhysicalLoc].iMinY, abs(psStrand->iY));
        asDefaultPosition[iBoardPhysicalLoc].iMaxY = max(asDefaultPosition[iBoardPhysicalLoc].iMaxY, abs(psStrand->iY));
        asDefaultPosition[iBoardPhysicalLoc].iMinZ = min(asDefaultPosition[iBoardPhysicalLoc].iMinZ, abs(psStrand->iZ));
        asDefaultPosition[iBoardPhysicalLoc].iMaxZ = max(asDefaultPosition[iBoardPhysicalLoc].iMaxZ, abs(psStrand->iZ));        
    }

    for(iIdx = 0; iIdx<ACTIVE_STRANDS; iIdx++)
    {
        asDefaultPosition[iIdx].iXAvg = (int)((double)asDefaultPosition[iIdx].iXAvg / (double)max(1, asDefaultPosition[iIdx].iNumOfSamples));
        asDefaultPosition[iIdx].iYAvg = (int)((double)asDefaultPosition[iIdx].iYAvg / (double)max(1, asDefaultPosition[iIdx].iNumOfSamples));
        asDefaultPosition[iIdx].iZAvg = (int)((double)asDefaultPosition[iIdx].iZAvg / (double)max(1,asDefaultPosition[iIdx].iNumOfSamples));
        asDefaultPosition[iIdx].iDiffX = asDefaultPosition[iIdx].iMaxX - asDefaultPosition[iIdx].iMinX;
        asDefaultPosition[iIdx].iDiffY = asDefaultPosition[iIdx].iMaxY - asDefaultPosition[iIdx].iMinY;
        asDefaultPosition[iIdx].iDiffZ = asDefaultPosition[iIdx].iMaxZ - asDefaultPosition[iIdx].iMinZ;
        asDefaultPosition[iIdx].iXTolarance = (int)((float)asDefaultPosition[iIdx].iDiffX/2.0f) + EXTRA_TOL;
        asDefaultPosition[iIdx].iYTolarance = (int)((float)asDefaultPosition[iIdx].iDiffY/2.0f) + EXTRA_TOL;
        asDefaultPosition[iIdx].iZTolarance = (int)((float)asDefaultPosition[iIdx].iDiffZ/2.0f) + EXTRA_TOL;
        m_aiHueColor[iIdx][0] = 0x10;
        m_aiHueColor[iIdx][1] = 0x0;
        m_aiHueColor[iIdx][2] = 0x0;
        printf("Strand positions: Board, %d, x, %d, y, %d, z, %d, xDiff, %d, yDiff, %d, zDiff, %d, counter, %d, tolX, %d , tolY, %d , tolZ, %d \n", m_aiActiveStrands[iIdx], 
        asDefaultPosition[iIdx].iXAvg,
        asDefaultPosition[iIdx].iYAvg,
        asDefaultPosition[iIdx].iZAvg,
        asDefaultPosition[iIdx].iDiffX, 
        asDefaultPosition[iIdx].iDiffY,
        asDefaultPosition[iIdx].iDiffZ,
        asDefaultPosition[iIdx].iXTolarance,
        asDefaultPosition[iIdx].iYTolarance,
        asDefaultPosition[iIdx].iZTolarance,
        asDefaultPosition[iIdx].iNumOfSamples);
    }
    printf("Strand initilization sucessful\n");
    //Launch threads  and initialize mutex for each strand
    pthread_barrier_init(&m_tSync, NULL, ACTIVE_STRANDS);
    for(iIdx = 0; iIdx < ACTIVE_STRANDS; iIdx++)
    {
        struct StrandParam_t * psStrandParam = malloc(sizeof(struct StrandParam_t));
        psStrandParam->iBoardAddr = m_aiActiveStrands[iIdx];
        psStrandParam->iBoardPhysicalLoc = iIdx;
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
    iThreadId = pthread_create(&stRainSound, NULL, playRain, NULL);
    if(iThreadId)
    {
        printf("Rain sound thread create error, %d\n", iThreadId);
    }
    clock_gettime(CLOCK_MONOTONIC_RAW, &stStart);
    clock_gettime(CLOCK_MONOTONIC_RAW, &stCurr);
    uint8_t uR = 126;
    uint8_t uG = 0;
    uint8_t uB = 0;
    uint8_t cHueCountVector = 0;
    uint8_t cHueCountColor = 127;
    int aaiHueChanges[6][3] = {{0, 0,1}, {-1,0,0}, {0,1,0},
                                                {0,0,-1},{1,0,0},{0,-1,0}};
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
        
        if(abs(sMovementTemp.fXDelta) > asDefaultPosition[iBoardPhysicalLoc].iXTolarance ||
            abs(sMovementTemp.fYDelta) > asDefaultPosition[iBoardPhysicalLoc].iYTolarance || 
            abs(sMovementTemp.fZDelta) > asDefaultPosition[iBoardPhysicalLoc].iZTolarance)
        {
            // printf("Movement Outside mutex: Board, %d, x, %f, y, %f, z, %f, xToll, %d, yToll, %d, zToll, %d\n", iBoardAddr, 
            // sMovementTemp.fXDelta,
            // sMovementTemp.fYDelta,
            // sMovementTemp.fZDelta,
            // asDefaultPosition[iBoardPhysicalLoc].iXTolarance,
            // asDefaultPosition[iBoardPhysicalLoc].iYTolarance,
            // asDefaultPosition[iBoardPhysicalLoc].iZTolarance);
             // pthread_mutex_lock(&m_alStrandLock[iBoardPhysicalLoc]); 
            if( 0 == m_asMovementDelta[iBoardPhysicalLoc].iBoardState)
            {
                m_asMovementDelta[iBoardPhysicalLoc].fXDelta = sMovementTemp.fXDelta;
                m_asMovementDelta[iBoardPhysicalLoc].fYDelta = sMovementTemp.fYDelta;
                m_asMovementDelta[iBoardPhysicalLoc].fZDelta = sMovementTemp.fZDelta;
                m_asMovementDelta[iBoardPhysicalLoc].iBoardState = 1;
                //printf("Movement Inside mutes Board\n");
            }
            pthread_mutex_unlock(&m_alStrandLock[iBoardPhysicalLoc]);
        }
        clock_gettime(CLOCK_MONOTONIC_RAW, &stCurr);
        uint32_t  uDeltaTime = (abs(stCurr.tv_nsec - stStart.tv_nsec)/10000);
        //printf("uDeltaTime, %ld\n", uDeltaTime);

        if(uDeltaTime  > 1)
        {    
          clock_gettime(CLOCK_MONOTONIC_RAW, &stStart);
          
          if(cHueCountColor == 1)
           {
               cHueCountVector += 1;   
               cHueCountColor = 126;
               if(cHueCountVector > 5)
                   cHueCountVector = 0;
           }
           else
           {
               cHueCountColor --;
           }
            uR = uR + aaiHueChanges[cHueCountVector][0];
            uG = uG + aaiHueChanges[cHueCountVector][1];
            uB = uB + aaiHueChanges[cHueCountVector][2];
            
            for(iIdx = ACTIVE_STRANDS - 1; iIdx >=1; iIdx--) //Copy color from previous strand from last idx to current
            {
                m_aiHueColor[iIdx][0] = m_aiHueColor[iIdx - 1][0];
                m_aiHueColor[iIdx][1] = m_aiHueColor[iIdx - 1][1];
                m_aiHueColor[iIdx][2] = m_aiHueColor[iIdx - 1][2];
            }
            //Update 0th strand with current hue
            m_aiHueColor[iIdx][0] = uR;
            m_aiHueColor[iIdx][1] = uG;
            m_aiHueColor[iIdx][2] = uB;
            //printf("cHueCountVector %d, cHueCountColor %d, R %d, G %d, B %d\n",cHueCountVector, cHueCountColor,uR,uG,uB);
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
    return 0; 
} 
