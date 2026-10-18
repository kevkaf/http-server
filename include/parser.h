#ifndef PARSER_H
#define PARSER_H

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

int parse_request_line(HttpRequest* request, char* line);
int parse_http_request(HttpRequest* http, char* request);
int get_content_length(HttpRequest* http);


#endif
