#include "fx.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

void fadeToDefault(uint8_t *leds, int num, uint8_t fadeValue, uint8_t cR, uint8_t cG, uint8_t cB) 
{
    uint8_t r, g, b;

    r = leds[num * 3 + 0];
    g = leds[num * 3 + 1];
    b = leds[num * 3 + 2];

    r=(r<=10)? 0 : (uint8_t) r-(r*fadeValue/256);
    g=(g<=10)? 0 : (uint8_t) g-(g*fadeValue/256);
    b=(b<=10)? 0 : (uint8_t) b-(b*fadeValue/256);

    leds[(num * 3) + 0] = r;
    leds[(num * 3) + 1] = g;
    leds[(num * 3) + 2] = b;
}

int effectMeteor(int iSocket, uint8_t * matrix, uint8_t cR, uint8_t cG, uint8_t cB) 
{
    int meteorTrailDecay = 64;
    int meteorRandomDecay = 0;
    int meteorSize = 10;
    float rate = (rand() % 5 + 5.0) / 10;

    //for (o = 0; o < (3 * kLEDCnt) - (30*3); ++o) // minus 30 for slightly earlier finish
    for (int o = 0; o < kLEDCnt + meteorTrailDecay; ++o)
    {
        int j = (int)(rate * o);
        for (int k = 0; k < kLEDCnt; ++k) 
        {
            fadeToDefault(matrix, k, meteorTrailDecay, cR, cG, cB);
        }

        for (int k = 0; k < meteorSize; ++k) 
        {
            if ((j - k < kLEDCnt) && (j - k >= 0)) 
            {
                matrix[((kLEDCnt - (j - k) - 1) * 3) + 0] = 255;
                matrix[((kLEDCnt - (j - k) - 1) * 3) + 1] = 255;
                matrix[((kLEDCnt - (j - k) - 1) * 3 )+ 2] = 255;
            }
        }

        if (send(iSocket, matrix, (kLEDCnt*3) + 1, 0) < 0) 
        {
            fprintf(stderr, "Send failed");
            return -1;
        }
        usleep(13000);
    }
    return 1;
}

int effectThunder(int iSocket, uint8_t uR, uint8_t uG, uint8_t uB, uint8_t iFinalBrightness) 
{
        uint8_t matrix[kLEDCnt * 3 + 1]; //+1 for brightness
   for (int j = 0; j < kLEDCnt; ++j) 
    {
        matrix[j *3 + 0] = 0;
        matrix[j *3 + 1] = 0;
        matrix[j *3 + 2] = 0;
    }
    matrix[kLEDCnt * 3] = 100;
    if (send(iSocket, matrix, (kLEDCnt*3) + 1, 0) < 0) 
    {
        fprintf(stderr, "Send failed");
        return -1;
    }
        usleep(100000);
    for(int iIdx = 0; iIdx < 10; iIdx++)
    {
        for (int j = 0; j < kLEDCnt; ++j) 
        {
            matrix[j *3 + 0] = (iIdx * 25);
            matrix[j *3 + 1] = (iIdx * 25);
            matrix[j *3 + 2] = (iIdx * 25);
        }
            matrix[kLEDCnt * 3] = (iIdx * 25);

        if (send(iSocket, matrix, (kLEDCnt*3) + 1, 0) < 0) 
        {
            fprintf(stderr, "Send failed");
            return -1;
        }
        usleep(35000);
    }
        for (int j = 0; j < kLEDCnt; ++j) 
    {
        matrix[j *3 + 0] = 50;
        matrix[j *3 + 1] = 50;
        matrix[j *3 + 2] = 50;
    }
            matrix[kLEDCnt * 3] = 50;
    if (send(iSocket, matrix, (kLEDCnt*3) + 1, 0) < 0) 
    {
        fprintf(stderr, "Send failed");
        return -1;
    }
    usleep(200000);

    for(int iIdx = 0; iIdx < 10; iIdx++)
    {
        for (int j = 0; j < kLEDCnt; ++j) 
        {
            matrix[j *3 + 0] = 180 - (iIdx * 18);
            matrix[j *3 + 1] = 180 - (iIdx * 18);
            matrix[j *3 + 2] = 180 - (iIdx * 18);
        }
            matrix[kLEDCnt * 3] = 180 - (iIdx * 18);

        if (send(iSocket, matrix, (kLEDCnt*3) + 1, 0) < 0) 
        {
            fprintf(stderr, "Send failed");
            return -1;
        }
        usleep(20000);
    }
    for(int iIdx = 0; iIdx < 20; iIdx++)
    {
        for (int j = 0; j < kLEDCnt; ++j) 
        {
            matrix[j *3 + 0] = uR;
            matrix[j *3 + 1] = uG;
            matrix[j *3 + 2] = uB;
        }
            matrix[kLEDCnt * 3] = iIdx * (iFinalBrightness/20);

        if (send(iSocket, matrix, (kLEDCnt*3) + 1, 0) < 0) 
        {
            fprintf(stderr, "Send failed");
            return -1;
        }
        usleep(20000);
    }
}

int effectMeteorDown(int iSocket, uint8_t * matrix, uint8_t cR, uint8_t cG, uint8_t cB) 
{
  int meteorTrailDecay = 64;
  int meteorRandomDecay = 0;
  int meteorSize = 10;
   
  for (int j = 0; j < kLEDCnt + meteorTrailDecay; ++j) 
  {
      for (int k = 0; k < kLEDCnt; ++k) 
      {
          fadeToDefault(matrix, k, meteorTrailDecay, cR, cG, cB);
      }

      for (int k = 0; k < meteorSize; ++k) {
        if ((j - k < kLEDCnt) && (j - k >= 0)) {
          matrix[(j - k) * 3 + 0] = 255;
          matrix[(j - k) * 3 + 1] = 255;
          matrix[(j - k) * 3 + 2] = 255;
        }
      }
      if (send(iSocket, matrix,  (kLEDCnt*3) + 1, 0) < 0) {
        fprintf(stderr, "Send failed");
        return - 1;
      }
      usleep(13000);
  }
  return 1;
}

int effectMeteorPartial(int iSocket, uint8_t * matrix, int iRow, int iDropSize,  int iTrailSize, int iRandInt) 
{
      int iIdx = 0;
      int k;
      for (k = 0; k < iDropSize; ++k) 
      {
        if ((iRow - k < kLEDCnt) && (iRow - k >= 0)) 
        {
          matrix[(iRow - k) * 3 + 0] = 255;
          matrix[(iRow - k) * 3 + 1] = 255;//min(cG + 200, 255);
          matrix[(iRow - k) * 3 + 2] = 255;//min(cB + 200, 255);
        }
      }
      for (k = iDropSize; k < iDropSize + iTrailSize; ++k) 
      {
          if ((iRow - k < kLEDCnt) && (iRow - k >= 0))
          {
              matrix[(iRow - k) * 3 + 0] = max(0, 180 - (iIdx * 9));//min(cR + (140 - (47*iIdx)), 255);
              matrix[(iRow - k) * 3 + 1] = max(0, 180 - (iIdx * 9));//min(cG + (140 - (47*iIdx)), 255);
              matrix[(iRow - k) * 3 + 2] = max(0, 180 - (iIdx * 9));//min(cB + (140 - (47*iIdx)), 255);
              //printf("iRox, %d, iIdx, %d, iVal, %d\n", iRow, iIdx, max(0, 180 - (iIdx * 9)));
          }
          iIdx++;
      }
      iIdx = 0;
      for (k = (iDropSize + iTrailSize - 15) + (iRandInt %15); iIdx < 10 ; ++k)
      {              
          if ((iRow - k < kLEDCnt) && (iRow - k >= 0))
          {
              matrix[(iRow - k) * 3 + 0] = 0;//max(0, 180 - (iIdx * 9));//min(cR + (140 - (47*iIdx)), 255);
              matrix[(iRow - k) * 3 + 1] = 0;//max(0, 180 - (iIdx * 9));//min(cG + (140 - (47*iIdx)), 255);
              matrix[(iRow - k) * 3 + 2] = 0;//max(0, 180 - (iIdx * 9));//min(cB + (140 - (47*iIdx)), 255);
              //printf("iRox, %d, k, %d\n", iRow, k);
              
          }
          iIdx++;
      }
      // for (k = iDropSize + iTrailSize; k < iDropSize + iTrailSize + 10; ++k) 
      // {
          // if ((iRow - k < kLEDCnt) && (iRow - k >= 0))
          // {
              // matrix[(iRow - k) * 3 + 0] = cR;//min(cR + (140 - (7*iIdx)), 255);
              // matrix[(iRow - k) * 3 + 1] = cG;//min(cG + (140 - (7*iIdx)), 255);
              // matrix[(iRow - k) * 3 + 2] = cB;//min(cB + (140 - (7*iIdx)), 255);
          // }
      // }
      // for (; k < iDropSize + iTrailSize + 10; ++k) 
      // {
          // if ((iRow - k < kLEDCnt) && (iRow - k >= 0))
          // {
              // if(k < iDropSize + iTrailSize + 10)
              // {
                  // if(0 == (k + iRandInt)%30)
                  // {
                        // matrix[(iRow - k) * 3 + 0] = 100;//min(cR + (140 - (7*iIdx)), 255);
                        // matrix[(iRow - k) * 3 + 1] = 100;//min(cG + (140 - (7*iIdx)), 255);
                        // matrix[(iRow - k) * 3 + 2] = 100;//min(cB + (140 - (7*iIdx)), 255);
                        // printf("iRox, %d, k, %d\n", iRow, k);
                  // }
              // }
          // }
      // }

      for (k = iDropSize + iTrailSize; k < kLEDCnt; ++k) 
      {
          if ((iRow - k < kLEDCnt) && (iRow - k >= 0))
          {
              matrix[(iRow - k) * 3 + 0] = 0;//min(cR + (140 - (7*iIdx)), 255);
              matrix[(iRow - k) * 3 + 1] = 0;//min(cG + (140 - (7*iIdx)), 255);
              matrix[(iRow - k) * 3 + 2] = 0;//min(cB + (140 - (7*iIdx)), 255);
          }
      }

//       printf("j, %d, brightness, %d, %p\n",iRow,matrix[(kLEDCnt*3) - 3], &matrix[(kLEDCnt*3) - 3]);
      if (send(iSocket, matrix,  (kLEDCnt*3 + 1), 0) < 0) 
      {
        fprintf(stderr, "Send failed");
        return - 1;
      }
}
#define dropOffset 100
#define dropOffsetDecay 25
static void getPixel(uint8_t * aPixel, int iDropSize, int iTrailSize, uint8_t cR, uint8_t cG, uint8_t cB, int iRainStart, int iRandInt)
{
    switch (iRainStart)
    {
        case 6:
        {
        aPixel[0] = 255;
        aPixel[1] = 255;
        aPixel[2] = 255;
        break;
        }
        case 5 : 
        {
        aPixel[0] = min(cR + (dropOffset - (dropOffsetDecay*2)), 255);
        aPixel[1] = min(cG + (dropOffset - (dropOffsetDecay*2)), 255);
        aPixel[2] = min(cB + (dropOffset - (dropOffsetDecay*2)), 255);
        break;
        }
        case 4:
        {
        aPixel[0] = min(cR + (dropOffset - (dropOffsetDecay*3)), 255);
        aPixel[1] = min(cG + (dropOffset - (dropOffsetDecay*3)), 255);
        aPixel[2] = min(cB + (dropOffset - (dropOffsetDecay*3)), 255);
        break;
        }
        case 3:
        {
        aPixel[0] = min(cR + (dropOffset - (dropOffsetDecay*4)), 255);
        aPixel[1] = min(cG + (dropOffset - (dropOffsetDecay*4)), 255);
        aPixel[2] = min(cB + (dropOffset - (dropOffsetDecay*4)), 255);
        break;
        }
        case 2:
        {
        aPixel[0] = min(cR + (dropOffset - (dropOffsetDecay*4)), 255);
        aPixel[1] = min(cG + (dropOffset - (dropOffsetDecay*4)), 255);
        aPixel[2] = min(cB + (dropOffset - (dropOffsetDecay*4)), 255);
         break;
        }
        case 1:
        {
            if(iRandInt %2)
            {
            aPixel[0] = max(cR + (dropOffset - (dropOffsetDecay*5)), 0);
            aPixel[1] = max(cG + (dropOffset - (dropOffsetDecay*5)), 0);
            aPixel[2] = max(cB + (dropOffset - (dropOffsetDecay*5)), 0);
            }
            else
            {
                aPixel[0] = cR>>1;
                aPixel[1] = cG>>1;
                aPixel[2] = cB>>1;                
            }
   
        break;
        }
        default:
        aPixel[0] = cR;
        aPixel[1] = cG;
        aPixel[2] = cB;
        break;
    }
}
//Sparkly rain
int effectRainPartial(int iSocket, uint8_t * matrix, uint8_t cR, uint8_t cG, uint8_t cB, int iDropSize,  int iTrailSize, int iRainStart, int iRandInt) 
{
      int iIdx = 0;
      uint8_t aPixel[3];
      uint8_t aRandPixel[3];
      for (iIdx = kLEDCnt - 1; iIdx > 0; iIdx--) 
      {
          matrix[(iIdx * 3) + 0] = matrix[((iIdx - 1) * 3) + 0];
          matrix[(iIdx * 3) + 1] = matrix[((iIdx - 1) * 3) + 1];
          matrix[(iIdx * 3) + 2] = matrix[((iIdx - 1) * 3) + 2];
      }
      getPixel(aPixel, iDropSize, iTrailSize, cR, cG, cB, iRainStart, iRandInt);
      matrix[0] = aPixel[0];
      matrix[1] = aPixel[1];
      matrix[2] = aPixel[2];

      int iSparkleLoc = iRandInt % kLEDCnt;
      aRandPixel[0] = matrix[iSparkleLoc * 3 + 0];
      aRandPixel[1] = matrix[iSparkleLoc * 3 + 1];
      aRandPixel[2] = matrix[iSparkleLoc * 3 + 2];
      
      matrix[iSparkleLoc * 3 + 0] = aRandPixel[0]<<1;
      matrix[iSparkleLoc * 3 + 1] = aRandPixel[1]<<1;
      matrix[iSparkleLoc * 3 + 2] = aRandPixel[2]<<1;

      if (send(iSocket, matrix,  (kLEDCnt*3 + 1), 0) < 0) 
      {
        fprintf(stderr, "Send failed");
        return - 1;
      }
      usleep(3000);
      matrix[iSparkleLoc * 3 + 0] = aRandPixel[0];
      matrix[iSparkleLoc * 3 + 1] = aRandPixel[1];
      matrix[iSparkleLoc * 3 + 2] = aRandPixel[2];
//       printf("j, %d, brightness, %d, %p\n",iRow,matrix[(kLEDCnt*3) - 3], &matrix[(kLEDCnt*3) - 3]);
      if (send(iSocket, matrix,  (kLEDCnt*3 + 1), 0) < 0) 
      {
        fprintf(stderr, "Send failed");
        return - 1;
      }
}

