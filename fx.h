#ifndef FX_H
#define FX_H
#include <stdint.h>

 #define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })
 #define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

static const int kLEDCnt = 120;
int effectMeteor(int iSocket, uint8_t * matrix, uint8_t cR, uint8_t cG, uint8_t cB);
int effectThunder(int iSocket, uint8_t uR, uint8_t uG, uint8_t uB, uint8_t iFinalBrightness);
int effectMeteorDown(int iSocket, uint8_t * matrix, uint8_t cR, uint8_t cG, uint8_t cB);
int effectMeteorPartial(int iSocket, uint8_t * matrix, int iRow, int iDropSize,  int iTrailSize, int iRandInt);
int effectRainPartial(int iSocket, uint8_t * matrix, uint8_t cR, uint8_t cG, uint8_t cB, int iDropSize,  int iTrailSize, int iRainStart, int iRandInt);

#endif /* FX_H */

