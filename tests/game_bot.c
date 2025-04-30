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
    
//    write(sockfd, &message, M_SIZE);
    return sockfd;
}

void throw_card_test(int sockfd)
{
    struct timespec duration, rem;
    char buf[] = "0";

    duration.tv_sec = 1;
    duration.tv_nsec = 0;

    for (int i = 0; i < NUMBER_OF_CARDS; i++) {
        write(sockfd, buf, strlen(buf));
        nanosleep(&duration, &rem);
    }
} 

int visual_test(int sockfd, char *buf)
{
    int count;

    count = read(sockfd, buf, BUF_SIZE);
    if (count == -1 && (errno != EAGAIN || errno != EWOULDBLOCK)) {
        return 0;
    }
    if (count == -1) {
        perror("read");
        return - 1;
    }
    printf("count = %d\n", count);
    write(STDOUT_FILENO, buf, count);
    return count;
}

int main()
{
    int sockfd;
    char buf[BUF_SIZE];

    if ((sockfd = connect_to_server()) == -1)
        return 1;
    
    close(sockfd);
    /*
    while (1) {
        //throw_card_test(sockfd);
        if (visual_test(sockfd, buf) == -1)
            break;
        sleep(1);
    }
    */
}
