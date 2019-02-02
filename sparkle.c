#include <arpa/inet.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fx.c"

struct strand {
  int sock;
  int host;
};

int createConnection(struct strand *s) {
  struct sockaddr_in server;

  s->sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (s->sock == -1) {
    fprintf(stderr, "Could not create socket");
    return 0;
  }

  char addr[32];
  sprintf(addr, "192.168.1.%d", s->host);
  server.sin_addr.s_addr = inet_addr(addr);
  server.sin_family = AF_INET;
  server.sin_port = htons(5000);

  if (connect(s->sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
    perror("connect failed. Error");
    return 0;
  }

  return 1;
}
int main(int c, char **v) {
    
    int w = 0, h = 0;
    struct strand strands;
    strands.host = 218;
    uint8_t uR = 127;
    uint8_t uG = 0;
    uint8_t uB = 0;
    uint8_t cHueCountVector = 0;
    uint8_t cHueCountColor = 127;
    int aaiHueChanges[6][3] = {{0, 0,1}, {-1,0,0}, {0,1,0},
                                                {0,0,-1},{1,0,0},{0,-1,0}};
    
    createConnection(&strands);
    uint8_t matrix[kLEDCnt * 3 + 1]; //+1 for brightness
   for (int j = 0; j < kLEDCnt; ++j) 
    {
        matrix[j *3 + 0] = uR;
        matrix[j *3 + 1] = uG;
        matrix[j *3 + 2] = uB;
    }
    matrix[kLEDCnt * 3] = 100;
    //effectMeteor(strands.sock,0,matrix, 0x10, 0, 0);
    //effectMeteorDown(strands.sock,0,matrix, 0x10, 0, 0);
    int iRow = 0;
    int iDropSize = 2;
    int iTrailSize = 4;
    int iRainStart = iDropSize + iTrailSize;
    int iRandInt = rand();
    int iMeteorSize = 10;
    int iMeteorTrailSize = 20;
    time_t secondsStart = time(NULL);
   while(1)
   {
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

       uR = 0;//uR + aaiHueChanges[cHueCountVector][0];
       uG = 0;//uG + aaiHueChanges[cHueCountVector][1];
       uB = 0;//uB + aaiHueChanges[cHueCountVector][2];
       // printf("cHueCountVector %d, cHueCountColor %d, R %d, G %d, B %d\n",cHueCountVector, cHueCountColor,uR,uG,uB);
       
//       effectRainPartial(strands.sock,matrix, uR, uG, uB, iDropSize, iTrailSize, iRainStart, iRandInt);
              effectMeteorPartial(strands.sock,matrix, uR, uG, uB,iRow/3, iMeteorSize, iMeteorTrailSize, iRandInt);
       iRow++;
       iRandInt = rand();
       if(iRainStart > 0)
           iRainStart --;
       if(0 == (iRandInt % 20))
           iRainStart = iDropSize + iTrailSize;
       if((iRow/3)>=  kLEDCnt + iMeteorSize + iMeteorTrailSize)
       {
           effectThunder(strands.sock);
           iRow = 0;
           for (int j = 0; j < kLEDCnt; ++j) 
            {
                matrix[j *3 + 0] = uR;
                matrix[j *3 + 1] = uG;
                matrix[j *3 + 2] = uB;
            }
       }
       //printf("%d, %d\n", iRow,35000 - min(15000,(iRow * iRow)));
       usleep(8000);// - min(15000,(iRow * iRow)));
   }
   
  close(strands.sock);

  return 0;
}

