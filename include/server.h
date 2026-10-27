#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>

#include "client.h"
#define MAX_EVENTS 64
#define BUF_SIZE 4096


typedef struct {
    int fd;
    uint32_t event;

    char in_buf[BUF_SIZE];
    size_t in_offset;

    char out_buf[BUF_SIZE];
    size_t out_size;
    size_t out_offset;
} Connection_t;


int set_non_blocking(int fd);

void worker_process(int server_fd);
int handle_client(Connection_t* conn);

int conn_send(Connection_t* conn);


#endif
