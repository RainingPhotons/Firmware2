#include <arpa/inet.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const int kStrandCnt = 20;

int createConnection(int *sock, int host) {
  struct sockaddr_in server;

  *sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (*sock == -1) {
    fprintf(stderr, "Could not create socket");
    return 0;
  }

  char addr[32];
  sprintf(addr, "192.168.1.%d", host);
  server.sin_addr.s_addr = inet_addr(addr);
  server.sin_family = AF_INET;
  server.sin_port = htons(5000);

  if (connect(*sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
    perror("connect failed. Error");
    return 0;
  }

  return 1;
}

void send_cmd(int sock, char *buffer) {
  if (send(sock, buffer, strlen(buffer), 0) < 0) {
    fprintf(stderr, "Send failed");
  }
  usleep(10000);
}

int main(int argc, char **argv) {
  int c;
  int pause = 0;

  while ((c = getopt(argc, argv, "p")) != -1) {
    switch (c) {
      case 'p':
        pause = 1;
        break;
    }
  }

  char cmd_buffer[256];
  uint32_t sample_rate = 16;

  for (int i = 0; i <= kStrandCnt; ++i) {
    int sock;
    printf("%d\n", i);
    createConnection(&sock, i + 200);

    // Identify the strand
    sprintf(cmd_buffer, "b100");
    send_cmd(sock, cmd_buffer);

    // Set acceleromter on
    sprintf(cmd_buffer, "a1");
    send_cmd(sock, cmd_buffer);

    // Set sample rate
    sprintf(cmd_buffer, "s%d", sample_rate);
    send_cmd(sock, cmd_buffer);

    // pause
    if (pause)
      getchar();
    else
      sleep(1);

    // Clear the strand
    sprintf(cmd_buffer, "r0");
    send_cmd(sock, cmd_buffer);

    close(sock);
  }

  return 0;
}

