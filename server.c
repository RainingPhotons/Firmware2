// Client side implementation of UDP client-server model 
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
    int16_t iBoardAddr;
};

const int iAddrPrefix = 200;

int main() 
{ 
    int iSockfd; 
    char acBuffer[MAXLINE]; 
    struct sockaddr_in     sServaddr; 
    int iIdx;
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
    int n, len; 
    
    time_t tStartTime = time(NULL);
    time_t tCurrTime = time(NULL);
    int iCounter = 0;
    const int iTotalStrands = 20;
    const int iCallibrationTimeOut = 10;
    const int iMovementTolaranceXY = 200; //TODO: make it unique per board?
    const int iMovementTolaranceZ = 400; //TODO: make it unique per board?
    struct AvgPosition_t asDefaultPosition[iTotalStrands];
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
        int16_t iBoardAddr = (*((int16_t*)(&acBuffer[0]))) - iAddrPrefix;
        if(iBoardAddr > 20 || iBoardAddr < 0)
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

    for(iIdx = 0; iIdx<iTotalStrands; iIdx++)
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
    while(1)
    {
        if( 0 >read(iSockfd, (char *)acBuffer, MAXLINE))
            printf("error\n");
        int16_t iBoardAddr = (*((int16_t*)(&acBuffer[0]))) - iAddrPrefix;
        if(iBoardAddr > 20 || iBoardAddr < 0)
        {
            printf("Board Adress Error. Exiting\n");
            return -1;
        }
        struct Position_t * psStrand = &sTempPosition;
        memcpy((void*)psStrand, (void*)&acBuffer[2], sizeof(struct Position_t));
        sMovement.iBoardAddr = iBoardAddr;
        sMovement.fXDelta = (float)(asDefaultPosition[iBoardAddr].iXAvg - psStrand->iX);
        sMovement.fYDelta = (float)(asDefaultPosition[iBoardAddr].iYAvg - psStrand->iY);
        sMovement.fZDelta = (float)(asDefaultPosition[iBoardAddr].iZAvg - psStrand->iZ);
        
        if(abs(sMovement.fXDelta) > iMovementTolaranceXY ||
            abs(sMovement.fYDelta) > iMovementTolaranceXY || 
            abs(sMovement.fZDelta) > iMovementTolaranceZ)
        {
            printf("Movement: Board, %d, x, %f, y, %f, z, %f\n", iBoardAddr, 
            sMovement.fXDelta,
            sMovement.fYDelta,
            sMovement.fZDelta);
        }
    }
    close(iSockfd); 
    return 0; 
} 
