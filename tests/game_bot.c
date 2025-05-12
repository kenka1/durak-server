#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>

#include "../constants.h"

#define M_SIZE 3

int connect_to_server()
{
    int ok, sockfd, opt, flags;
    struct sockaddr_in addr;
    char message[M_SIZE] = "\r\n";

    addr.sin_family = AF_INET;
    ok = inet_aton("127.0.0.1", &addr.sin_addr);
    if (!ok) {
        fprintf(stderr, "Invalied address\n");
        return -1;
    }
    addr.sin_port = htons(PORT);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return -1;
    }

    opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        close(sockfd);
        return -1;
    }

    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(sockfd);
        return -1;
    }
    printf("connected\n");

    flags = fcntl(sockfd, F_GETFL);
    if (flags == -1 ) {
        fprintf(stderr, "fcntl F_GETFL");
        close(sockfd);
        return -1;
    }

    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        fprintf(stderr, "fcntl F_SETFL");
        close(sockfd);
        return -1;
    }
    
    write(sockfd, &message, M_SIZE);
    return sockfd;
}

void attack_test(int sockfd, char *card)
{
    char buffer[32];
    snprintf(buffer, 32, "%s\n", card);
    write(sockfd, buffer, strlen(buffer));
} 

void defense_test(int sockfd, char *card0, char *card1)
{
    char buffer[32];
    snprintf(buffer, 32, "%s %s\n", card0, card1);
    write(sockfd, buffer, strlen(buffer));
} 

int visual_test(int sockfd, char *buf)
{
    int count = read(sockfd, buf, BUF_SIZE);
    if (count == -1) {
        return 0;
    }
    write(STDOUT_FILENO, buf, count);
    return count;
}

int main()
{
    int sockfd, seed;
    char buf[BUF_SIZE];
    struct timespec dur0, dur1, rem;
    dur0.tv_sec = 0;
    dur0.tv_nsec = 100000000;
    dur0.tv_sec = 0;
    dur0.tv_nsec = 500000000;
    char *cards[] = {
        "0", "1", "2", "3", "4", "5", 
        "6", "7", "8", "9", "10", "11",
        "12", "13", "14", "15", "16", "17",
        "18", "19", "20", "21", "22", "23", 
        "24", "25", "26", "27", "28", "29",
        "30", "31", "32", "33", "34", "35"};

    seed = time(NULL);
    srand(seed);

    if ((sockfd = connect_to_server()) == -1)
        return 1;
    
    while (1) {
        if (visual_test(sockfd, buf) == -1)
            break;
        attack_test(sockfd, cards[rand() % NUMBER_OF_CARDS]);
        nanosleep(&dur0, &rem);
        defense_test(sockfd, cards[rand() % DESK_SIZE], cards[rand() % NUMBER_OF_CARDS]);
        nanosleep(&dur1, &rem);
    }
}
