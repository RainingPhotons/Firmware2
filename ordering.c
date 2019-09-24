#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

const int kServerSocket = 5002;
const int kMaxLine = 8;
const int kMaxStrands = 64;
const float kDivisor = 819.0;

volatile int loop = 1;

/* Signal Handler for SIGINT */
void sigintHandler(int sig_num) {
    signal(SIGINT, sigintHandler);
    loop = 0;
}

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

void send_cmd(int sock, char const *buffer) {
  if (send(sock, buffer, strlen(buffer), 0) < 0) {
    fprintf(stderr, "Send failed");
  }
  usleep(10000);
}


int main(int argc, char **argv) {
    int c;
    int float_display = 0;
    int accelerometer_output = 0;

    while ((c = getopt(argc, argv, "fa")) != -1) {
        switch (c) {
            case 'f':
                accelerometer_output = 1;
                float_display = 1;
                break;
            case 'a':
                accelerometer_output = 1;
                break;
        }
    }

    signal(SIGINT, sigintHandler); 

    int sockfd;
    // Open connection to listen to the strands
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 

    // set to non blocking i/o
    int flags = fcntl(sockfd, F_GETFL);
    if (flags < 0) {
        int errsv = errno;
        printf("flag read failed: %s", strerror(errsv));
        exit(EXIT_FAILURE);
    }

    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0) {
        int errsv = errno;
        printf("flag set failed: %s", strerror(errsv));
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(kServerSocket); 
    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect to server failed");
        exit(EXIT_FAILURE);
    }
    printf("Connection to server sucessful!\n");

    int32_t last_board = -1;
    int32_t ordering[kMaxStrands];
    int32_t cnt = 1;
    for (int i = 0; i < kMaxStrands; ++i) {
        ordering[i] = -1;
    }

    while (loop == 1) {
        char buffer[kMaxLine];
        int fd_read = read(sockfd, buffer, kMaxLine);
        if (fd_read < 0) {
            if (errno == EWOULDBLOCK) {
                usleep(1);
            } else {
                perror("connection problem\n");
            }
        } else {
            int16_t *data = (int16_t*)buffer;
            // TODO(fritz) : do some error checking to make sure this
            // is a valid board
            int board = data[0];

            if (accelerometer_output) {
                if (float_display) {
                    printf("board = %4d : %lf, %lf, %lf\n",
                        board, data[1] / kDivisor, data[2] / kDivisor,
                        data[3] / kDivisor);
                } else {
                    printf("board = %4d : %4d, %4d, %4d\n",
                        board, data[1], data[2], data[3]);
                }
                fflush(stdout);
            }
            // Y is pointing down, X and Z are to the side
            // if Y goes close to 0, then the board is horizontal
            int32_t y = data[2];
            if (y < 256 && y > -256) {
                last_board = board;
                int sock;
                createConnection(&sock, board);
                send_cmd(sock, "b100");
                close(sock);
                if (ordering[cnt - 1] != board) {
                    printf("board %d is detected\n", board);
                    ordering[cnt] = board;
                    cnt++;
                }
            } else if (last_board == board) {
                last_board = -1;
                int sock;
                createConnection(&sock, board);
                send_cmd(sock, "b0");
                close(sock);
            }
        }
    }

    for (int i = 1; i < kMaxStrands; i++) {
        if (ordering[i] == -1)
            break;
        printf("%d\n", ordering[i]);
    }
}
