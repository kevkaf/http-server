#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define MAX_KEY_LEN 50
#define MAX_VALUE_LEN 256
#define MAX_HEADERS 6

typedef struct {
    char key[MAX_KEY_LEN];
    char value[MAX_VALUE_LEN];
} HttpHeader;

typedef struct {
    char method[16];
    char uri[256];
    char version[16];
    HttpHeader headers[MAX_HEADERS];
    int header_parsed_count;
    const char* body;
    size_t body_len;
} HttpRequest;


int parse_request_line(HttpRequest* request, char* line) {
    if (!request || !line) return -1;

    char* space = strchr(line, ' ');
    if (!space) return -1;

    size_t len_method = space - line;
    memcpy(request->method, line, len_method);
    request->method[len_method] = '\0';

    char* uri_start = space + 1;
    char* next_space = strchr(uri_start, ' ');    
    if (!next_space) return -1;

    size_t len_uri = next_space - uri_start;
    memcpy(request->uri, uri_start, len_uri);
    request->uri[len_uri] = '\0';

    char* version_start = next_space + 1;
    size_t len_version = strcspn(version_start, "\r\n");
    if (len_version >= sizeof(request->version)) return -1;

    memcpy(request->version, version_start, len_version);
    request->version[len_version] = '\0';

    return 0;
}


int parse_http_request(HttpRequest* http, char* request) {
    if (!http || !request) return -1;

    memset(http, 0, sizeof(HttpRequest));

    char* line_end = strstr(request, "\r\n");
    if (!line_end) return -1;

    *line_end = '\0';
    if (parse_request_line(http, request) != 0) {
        return -1;
    }
    line_end = "\r";

    char* cursor = line_end + 2;

    while (cursor && *cursor != '\r' && *cursor != '\n' && *cursor != '\0') {
        line_end = strstr(request, "\r\n");
        if (!line_end) return -1;

        *line_end = '\0';
        char* colon = strchr(request, ':');
        if (colon) {
            *colon = '\0';
            char* key = cursor;
            char* value = colon + 1;

            while (*value == ' ') value++;
            
            if (http->header_parsed_count < MAX_HEADERS) {
                snprintf(http->headers[http->header_parsed_count].key, sizeof(http->headers[http->header_parsed_count].key), "%s", key);
                snprintf(http->headers[http->header_parsed_count].value, sizeof(http->headers[http->header_parsed_count].value), "%s", value);
                http->header_parsed_count++;
            }
            *colon = ':';
        }
        line_end = "\r";
        cursor = line_end + 2;
    }

    if (cursor[0] == '\r' && cursor[1] == '\n') {
        cursor += 2;
    }

    if (*cursor != '\0') {
        http->body = cursor;
        http->body_len = strlen(cursor);
    }

    return 0;
}

int get_content_length(HttpRequest* http) {
    if (!http) return -1;

    for (int i = 0; i < http->header_parsed_count; i++) {
        if (strcasecmp(http->headers[i].key, "Content-Length") == 0) {
            char* end;
            errno = 0;

            long value = strtol(http->headers[i].value, &end, 10);

            if (errno == ERANGE || end == http->headers[i].value || *end == '\0' || *end == '\r' || *end == '\n' || value < 0) {
                return -1;
            }
            return (int)value;
        }
    }
    return -1;
}
