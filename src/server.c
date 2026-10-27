#include "../include/server.h"

int set_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        perror("fcntl failed");
        exit(EXIT_FAILURE);
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static void conn_close(int epfd, Connection_t* conn) {
    epoll_ctl(epfd, EPOLL_CTL_DEL, conn->fd, NULL);
    close(conn->fd);
    free(conn);
}

static int conn_recv(int epfd, Connection_t *conn) {
    while(1) {
        size_t remaining = BUF_SIZE - conn->in_offset;
        if (remaining == 0) {
            fprintf(stderr, "Buffer full for %d", conn->fd);
            return -1;
        }

        ssize_t n = recv(conn->fd, conn->in_buf + conn->in_offset, remaining, 0);

        if (n > 0) { 
            conn->in_offset += (size_t) n;
        } else if (n == 0) {
            return -1;
        } else {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) return 1;
            perror("recv faild");
            return -1;
        }
    }
}

static void accpet_client(int epfd, int server_fd) {

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

        Connection_t *client_conn = calloc(1, sizeof(Connection_t));
        if (!client_conn) {
            perror("malloc failed"); 
            close(client_fd);
            continue;
        }
        client_conn->fd = client_fd;
        client_conn->event = EPOLLIN;

        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET;
        ev.data.ptr = client_conn;

        if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev) < 0) {
            perror("epoll_ctl failed");
            close(client_fd);
            continue;
        }

        printf("[SERVER] client [%d] has been added\n", client_fd); 
    }
}

void worker_process(int server_fd) {
    int epfd = epoll_create1(0);
    if (epfd < 0) {
        perror("epoll_create1 failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    struct epoll_event ev;
    ev.data.ptr = NULL;
    ev.events = EPOLLIN | EPOLLET | EPOLLEXCLUSIVE;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev) < 0) {
        perror("epoll_ctl failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    struct epoll_event events[MAX_EVENTS];
    
    while (1) {
        int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("epoll_wait failed");
            break;
        }
        for (int i = 0; i < n; i++) {
            Connection_t *conn = events[i].data.ptr;   

            if (conn == NULL) {
                accpet_client(epfd, server_fd);
                continue;
            }

            uint32_t e = events[i].events;
            int rc = 0;

            if (e & (EPOLLERR | EPOLLHUP)) {
                rc = -1;
            } else {
                if (e & EPOLLIN) rc = conn_recv(epfd, conn);
                handle_client(conn);
            }
            
            if (rc < 0) conn_close(epfd, conn);
        }
    }
    close(epfd);
    close(server_fd);
};

