#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>

#define PORT 8080
#define MAX_EVENTS 64
#define WORKER_COUNT 10

volatile sig_atomic_t keep_running = 1;

void handle_sigint(int sig) {
    (void)sig;
    keep_running = 0;
    _exit(EXIT_SUCCESS);
}

int set_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        perror("fcntl failed");
        exit(EXIT_FAILURE);
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void worker_process(int server_fd) {
    int epfd = epoll_create1(0);
    if (epfd < 0) {
        perror("epoll_create1 failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    struct epoll_event ev;
    ev.data.fd = server_fd;
    ev.events = EPOLLIN | EPOLLET | EPOLLEXCLUSIVE;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev) < 0) {
        perror("epoll_ctl failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    struct epoll_event events[MAX_EVENTS];
    
    while (keep_running) {
        int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("epoll_wait failed");
            break;
        }
        for (int i = 0; i < n; i++) {
            int fd = events[i].data.fd;

            if (fd == server_fd) {
                while (1) { 
                    struct sockaddr_in addr_client;
                    socklen_t len_addr_client = sizeof(addr_client);

                    int client_fd = accept(server_fd, (struct sockaddr*)&addr_client, &len_addr_client);
                    if (client_fd < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                        if (errno == EINTR) continue;
                        perror("accept failed");
                        continue;
                    }

                    if (set_non_blocking(client_fd) < 0) {
                        perror("set_non_blocking failed");
                        close(client_fd);
                        continue;
                    }

                    struct epoll_event ev;
                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.fd = client_fd;

                    if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev) < 0) {
                        perror("epoll_ctl failed");
                        close(client_fd);
                        continue;
                    }

                    printf("[SERVER] client [%d] has been added\n", client_fd); 
                }
            } else {
                printf("some sockets have activity\n");
            }
        }
    }
};

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
