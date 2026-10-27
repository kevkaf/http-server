
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>

#include "../include/server.h"

#define PORT 8080
#define WORKER_COUNT 10



volatile sig_atomic_t keep_running = 1;

void handle_sigint(int sig) {
    (void)sig;
    keep_running = 0;
    _exit(EXIT_SUCCESS);
}

int set_non_blocking(int fd);

int main() {
    
    struct sigaction sa_int;
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    if (sigaction(SIGINT, &sa_int, NULL) < 0) {
        perror("sigaction failed");
        exit(EXIT_FAILURE);
    }


    printf("Initializing socket...\n");
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (set_non_blocking(server_fd) < 0) {
        perror("set_non_blocking failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr_server;
    memset(&addr_server, 0, sizeof(addr_server));
    addr_server.sin_family = AF_INET;
    addr_server.sin_addr.s_addr = INADDR_ANY;
    addr_server.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&addr_server, sizeof(addr_server)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("bind Succeeded\n");

    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("listen Succeeded\n");

    pid_t workers[WORKER_COUNT];
    for(int i = 0; i < WORKER_COUNT; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            worker_process(server_fd);
            _exit(EXIT_SUCCESS);
        } else {
            workers[i] = pid;
        }
    }

    for (int i = 0; i < WORKER_COUNT; i++) {
        wait(NULL);
    }

    close(server_fd);
    return 0;
}
