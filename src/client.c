
#include "../include/client.h"
#include "../include/server.h"
#include "../include/parser.h"

#include <sys/sendfile.h>

typedef enum {
    GET_METHOD,
    POST_METHOD, 
    PUT_METHOD,
    UNKNOWN_METHOD
} method_type;

static method_type get_method(const char* method) {
    if (strcmp(method, "GET") == 0) return GET_METHOD;
    else if (strcmp(method, "POST") == 0) return POST_METHOD;
    else if (strcmp(method, "PUT") == 0) return PUT_METHOD;
    else return UNKNOWN_METHOD;
}

static const char* get_mime_type(const char* filepath) {
    const char *ext = strrchr(filepath, '.');
    if (!ext) return "application/octet-stream";

    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) return "text/html";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".js") == 0) return "application/javascript";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".gif") == 0) return "image/gif";
    if (strcmp(ext, ".json") == 0) return "application/json";
    if (strcmp(ext, ".ico") == 0) return "image/x-icon";

    return "application/octet-stream";
}

static void conn_reset_file(Connection_t *conn) {
    if (conn->file_fd > 0) close(conn->file_fd);

    conn->file_fd = -1;
    conn->file_size = 0;
    conn->file_off = 0;
}

int conn_send(Connection_t* conn) {
    while(conn->out_offset < conn->out_size) {

        ssize_t n = send(conn->fd, conn->out_buf + conn->out_offset, conn->out_size - conn->out_offset, 0);

        if (n > 0) {
            conn->out_offset += (size_t) n;
        } else if (n == 0) {
            return -1; 
        } else {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                conn->want_write = 1;
                return 1; 
            }
            if (errno != EPIPE && errno != ECONNRESET) perror("send failed");
            return -1;
        }
    }
    conn->out_offset = conn->out_size = 0;

    while (conn->file_fd >= 0 && conn->file_off < conn->file_size) {

        size_t remaining = (size_t)(conn->file_size - conn->file_off);
        ssize_t n = sendfile(conn->fd, conn->file_fd, &conn->file_off, remaining);
 
        if (n > 0) {
            continue;
        } else if (n == 0) {
            fprintf(stderr, "file shrank while sending on fd %d\n", conn->fd);
            return -1;
        } else {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                conn->want_write = 1;
                return 1;
            }
            if (errno != EPIPE && errno != ECONNRESET) perror("sendfile failed");
            return -1;
        }
    }
 
    conn_reset_file(conn);
    conn->want_write = 0;
    return 0;
}

static void fill_error_response(Connection_t *conn, const char *status_line, const char *body) {
    size_t body_len = strlen(body);
    conn->out_size = snprintf(conn->out_buf, BUF_SIZE,
                              "HTTP/1.1 %s\r\n"
                              "Content-Length: %zu\r\n"
                              "Content-Type: text/plain\r\n\r\n"
                              "%s", status_line, body_len, body);
    conn->out_offset = 0;
    conn_send(conn);
}

static void handle_get(Connection_t *conn, const char* uri) {
    char filepath[1024];
    struct stat st;

    const char* target_uri = (strcmp(uri, "/") == 0) ? "/index.html" : uri;
    snprintf(filepath, sizeof(filepath), "www%s", target_uri);

    char resolved_path[PATH_MAX];
    char root_path[PATH_MAX];

    if (realpath("www", root_path) == NULL) {
        fill_error_response(conn, "500 Internal Server Error", "Internal Server Error");
        return;
    }
    size_t len_root = strlen(root_path);

    if (realpath(filepath, resolved_path) == NULL) {
        fill_error_response(conn, "404 Not Found", "404 Not Found");
        return;
    }

    if (strncmp(resolved_path, root_path, len_root) != 0 || (resolved_path[len_root] != '\0' && resolved_path[len_root] != '/')) {
        fill_error_response(conn, "403 Forbidden", "403 Forbidden");
        return;
    }

    if (stat(resolved_path, &st) || !S_ISREG(st.st_mode)) { 
        fill_error_response(conn, "404 Not Found", "404 Not Found");
        return;
    }

    FILE* file = fopen(resolved_path, "rb");
    if (!file) {
        fill_error_response(conn, "500 Internal Server Error", "Internal Server Error");
        return;
    }

    const char* mime_type = get_mime_type(resolved_path);
    
    int header_len = snprintf(conn->out_buf, BUF_SIZE, 
                              "HTTP/1.1 200 OK\r\n"
                              "Content-Type: %s\r\n"
                              "Content-Length: %ld\r\n\r\n", mime_type, st.st_size);

    if (header_len < 0 || header_len >= BUF_SIZE) {
        fclose(file);
        fill_error_response(conn, "500 Internal Server Error", "Header Overflow");
        return;
    }

    conn->out_size = (size_t)header_len;

    size_t free_space = BUF_SIZE - conn->out_size;
    size_t bytes_read = fread(conn->out_buf + conn->out_size, 1, free_space, file);
    conn->out_size += bytes_read;

    fclose(file);

    conn->out_offset = 0;
    
    int result = conn_send(conn);
    if (result < 0) {
    }
}

int handle_client(Connection_t *conn) {
    if (!conn) return -1;

    HttpRequest http = {0};

    parse_http_request(&http, conn->in_buf);

    switch (get_method(http.method)) {
        case GET_METHOD:
            handle_get(conn, http.uri);
            break;

        case POST_METHOD:
        case PUT_METHOD:
            fill_error_response(conn, "501 Not Implemented", "Method not implemented yet");
            break;

        default:
            fill_error_response(conn, "501 Not Implemented", "Method not accepted");
            break;
    }

    conn->in_offset = 0; 
    return 0;
}

