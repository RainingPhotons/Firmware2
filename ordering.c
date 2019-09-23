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

volatile int loop = 1;

/* Signal Handler for SIGINT */
void sigintHandler(int sig_num) {
    signal(SIGINT, sigintHandler);
    loop = 0;
}

int main(int c, char **v) {
    int sockfd;
    struct sockaddr_in serv_addr;

    signal(SIGINT, sigintHandler); 

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

    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(kServerSocket); 
    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect to server failed");
        exit(EXIT_FAILURE);
    }
    printf("Connection to server sucessful!\n");

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
            printf("board = %4d : %4d, %4d, %4d\n",
                data[0], data[1], data[2], data[3]);
            fflush(stdout);
        }
    }
}
