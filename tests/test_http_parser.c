#include <assert.h>
#include <string.h>

#include "../include/parser.h"

void test_valid_request_line(void)
{
    HttpRequest http = {0};

    char line[] = "GET /index.html HTTP/1.1\r\n";

    int result = parse_request_line(&http, line);

    assert(result == 0);
    assert(strcmp(http.method, "GET") == 0);
    assert(strcmp(http.uri, "/index.html") == 0);
    assert(strcmp(http.version, "HTTP/1.1") == 0);
}

int main(void)
{
    test_valid_request_line();

    return 0;
}
