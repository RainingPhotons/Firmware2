static const int kLEDCnt = 120;

struct rgb {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

void setPixel(char *led, struct rgb color) {
    led[0] = color.r;
    led[1] = color.g;
    led[2] = color.b;
}

void fadeToBlack(char *leds, int num, char fadeValue) {
    uint8_t r, g, b;

    r = leds[num * 3 + 0];
    g = leds[num * 3 + 1];
    b = leds[num * 3 + 2];

    r=(r<=10)? 0 : (int) r-(r*fadeValue/256);
    g=(g<=10)? 0 : (int) g-(g*fadeValue/256);
    b=(b<=10)? 0 : (int) b-(b*fadeValue/256);

    leds[num * 3 + 0] = r;
    leds[num * 3 + 1] = g;
    leds[num * 3 + 2] = b;
}

int effectMeteor(int iSocket, int broadcast, char * matrix) {
    int meteorTrailDecay = 64;
    int meteorRandomDecay = 1;
    int meteorSize;
    float rate;
    struct rgb color;
    int o,i, j, k;
    for (j = 0; j < kLEDCnt; ++j) 
    {
        int pixel = j * 3;
        matrix[pixel + 0] = 0x0;
        matrix[pixel + 1] = 0x0;
        matrix[pixel + 2] = 0x0;
    }
    rate = (rand() % 5 + 5.0) / 10;
    meteorSize = rand() % 10 + 5;
    color.r = rand() % 255;
    color.g = rand() % 255;
    color.b = rand() % 255;

    for (o = 0; o < (3 * kLEDCnt) - (30*3); ++o) // minus 10 for slightly earlier finish
    {
        int j = (int)(rate * o);
        for (int k = 0; k < kLEDCnt; ++k) 
        {
            if ((!meteorRandomDecay) || ((rand() % 10) > 5)) 
            {
                fadeToBlack(matrix, k, meteorTrailDecay);
            }
        }

        for (k = 0; k < meteorSize; ++k) 
        {
            if ((j - k < kLEDCnt) && (j - k >= 0)) 
            {
                int pixel = (kLEDCnt - (j - k) - 1) * 3;
                setPixel(&matrix[pixel], color);
            }
        }

        if (send(iSocket, matrix, kLEDCnt*3, 0) < 0) 
        {
            fprintf(stderr, "Send failed");
            return -1;
        }
        usleep(8000);
    }
    return 1;
}

int effectMeteorDown(int iSocket, int broadcast, char * matrix) {
  int meteorTrailDecay = 64;
  int meteorRandomDecay = 1;
  int meteorSize = 10;

    for (int j = 0; j < kLEDCnt; ++j) {
      matrix[j *3 + 0] = 0x0;
      matrix[j *3 + 1] = 0x0;
      matrix[j *3 + 2] = 0x0;
    }

  for (int j = 0; j < kLEDCnt + kLEDCnt; ++j) {
      for (int k = 0; k < kLEDCnt; ++k) {
        if ((!meteorRandomDecay) || ((rand() % 10) > 5)) {
          fadeToBlack(matrix, k, meteorTrailDecay);
        }
      }

      for (int k = 0; k < meteorSize; ++k) {
        if ((j - k < kLEDCnt) && (j - k >= 0)) {
          matrix[(j - k) * 3 + 0] = 0xff;
          matrix[(j - k) * 3 + 1] = 0xff;
          matrix[(j - k) * 3 + 2] = 0xff;
        }
      }

      if (send(iSocket, matrix, kLEDCnt*3, 0) < 0) {
        fprintf(stderr, "Send failed");
        return - 1;
      }
      usleep(8000);
  }
  return 1;
}

int effectDefault(int iSocket, int broadcast, char * matrix) 
{
    int pixel = rand() % kLEDCnt;
    matrix[pixel * 3 + 0] = 0xff;
    matrix[pixel * 3 + 1] = 0x00;
    matrix[pixel * 3 + 2] = 0x00;
    if (send(iSocket, matrix, kLEDCnt*3, 0) < 0) 
    {
        fprintf(stderr, "Send failed");
        return -1;
    }

    usleep(2000);
    matrix[pixel * 3 + 0] = 0x10;
    matrix[pixel * 3 + 1] = 0x00;
    matrix[pixel * 3 + 2] = 0x00;
    if (send(iSocket, matrix, kLEDCnt*3, 0) < 0) 
    {
        fprintf(stderr, "Send failed");
        return -1;
    }
}