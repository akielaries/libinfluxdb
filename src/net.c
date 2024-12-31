#include <stdint.h>
#include "../lib/net.h"
#include <sys/errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>


int init_socket(char *influx_ip, uint32_t influx_port) {
    int i;
    static char buffer[4096];
    static struct sockaddr_in serv_addr;
    int sockfd = 0;

    fprintf(stderr,
            "socket: trying to connect to \"%s\":%ld\n",
            influx_ip,
            influx_port);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket() call failed");
        return 0;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(influx_ip);
    serv_addr.sin_port = htons(influx_port);

    /* connect to the socket offered by the web server */
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, " connect() call failed errno=%d\n", errno);
        return 0;
    }
    // TODO populate socket in a struct to use accross the functions that need 
    // it OR return the socket and pass by parameter
    return 1;
}

