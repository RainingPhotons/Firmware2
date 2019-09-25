#include <arpa/inet.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const int kStrandCnt = 20;
const int kMaxStrands = 64;

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
  int set_accelerometer = 0;
  char* input_file = NULL;
  char* strand_command = "b100";
  int strand_count = kStrandCnt + 1;

  while ((c = getopt(argc, argv, "par:c:")) != -1) {
    switch (c) {
      case 'p':
        pause = 1;
        break;
      case 'r':
        input_file = optarg;
        break;
      case 'a':
        set_accelerometer = 1;
        break;
      case 'c':
        strand_command = optarg;
        break;
    }
  }

  int32_t ordering[kMaxStrands];
  for (int i = 0; i < kMaxStrands; ++i) {
      ordering[i] = i + 200;
  }

  if (input_file) {
    FILE *fp;
    fp = fopen(input_file, "r");

    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    int i = 0;

    while ((read = getline(&line, &len, fp)) != -1) {
      if (i >= kMaxStrands) {
        fprintf(stderr, "Too many strands (>%d)\n", i);
        exit(EXIT_FAILURE);
      }
      int strand_number = atoi(line);
      if (strand_number < 200 || strand_number > 256) {
        fprintf(stderr, "Bad file, number out of range (%d)\n", strand_number);
        exit(EXIT_FAILURE);
      }
      ordering[i++] = strand_number;
    }

    strand_count = i;

    fclose (fp);
  }

  char cmd_buffer[256];
  uint32_t sample_rate = 16;

  for (int i = 0; i < strand_count; ++i) {
    int sock;
    int strand_number = ordering[i];
    printf("%d\n", strand_number);
    createConnection(&sock, strand_number);

    // Identify the strand
    send_cmd(sock, strand_command);

    if (set_accelerometer) {
      // Set accelerometer on
      sprintf(cmd_buffer, "a1");
      send_cmd(sock, cmd_buffer);

      // Set sample rate
      sprintf(cmd_buffer, "s%d", sample_rate);
      send_cmd(sock, cmd_buffer);
    }

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

