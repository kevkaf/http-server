# http-server

A small static-file HTTP/1.1 server written from scratch in C. It uses a pool of pre-forked worker processes, each running its own non-blocking, edge-triggered `epoll` loop, and streams files to clients with `sendfile()`.

It's a learning project: the goal is to understand how servers like nginx are put together, not to replace them.

## Features

- **Pre-forked workers.** 10 processes share one listening socket. Each has its own `epoll` instance and uses `EPOLLEXCLUSIVE`, so only one worker wakes up for each new connection (no thundering herd).
- **Non-blocking, edge-triggered I/O.** Accept and receive loops drain the socket until `EAGAIN`.
- **Large files.** Bodies are streamed with `sendfile()`. When a client reads slowly, the worker registers for `EPOLLOUT` and resumes the transfer when the socket is writable again.
- **Static file serving** from the `www/` directory (`/` maps to `/index.html`).
- **Path traversal protection.** Every path is resolved with `realpath()` and must stay inside `www/`; anything else gets `403`.
- **Persistent connections.** A client can send several requests on one connection.
- **Strict build.** Compiled with `-Wall -Wextra -Werror -pedantic`.

## Requirements

- Linux (the server uses `epoll`, `EPOLLEXCLUSIVE` and `sendfile`, so it won't build on macOS or native Windows)
- `gcc` and `make`
- A kernel of 4.5 or newer (for `EPOLLEXCLUSIVE`)

## Build and run

```sh
make          # builds bin/http
./bin/http    # listens on port 8080
```

Run it **from the repository root**: the server looks for the `www/` directory relative to the current working directory.

Then, from another terminal:

```sh
curl -i http://localhost:8080/
```

Stop it with `Ctrl+C`.

### Make targets

| Target        | What it does                                   |
| ------------- | ---------------------------------------------- |
| `make`        | Build `bin/http`                               |
| `make test`   | Build and run the unit tests (`bin/http_tests`) |
| `make clean`  | Remove `build/` (object and dependency files)  |
| `make fclean` | `clean`, plus remove `bin/`                    |
| `make re`     | `fclean`, then `all`                           |

## Configuration

There are no command-line options or config files yet. These are compile-time constants:

| Setting                   | Where                | Default |
| ------------------------- | -------------------- | ------- |
| Port                      | `src/main.c` (`PORT`)           | `8080`  |
| Number of worker processes | `src/main.c` (`WORKER_COUNT`)   | `10`    |
| Per-connection buffer size | `include/server.h` (`BUF_SIZE`) | `4096`  |
| Max events per `epoll_wait` | `include/server.h` (`MAX_EVENTS`) | `64`    |

## Supported requests

| Method | Behavior                          |
| ------ | --------------------------------- |
| `GET`  | Serves a file from `www/`         |
| `POST`, `PUT` | `501 Not Implemented` (planned)  |
| anything else | `501 Not Implemented`      |

| Status | When                                                    |
| ------ | ------------------------------------------------------- |
| `200`  | File found and sent                                     |
| `400`  | The request line couldn't be parsed                     |
| `403`  | The resolved path is outside `www/`                     |
| `404`  | File doesn't exist, or the path isn't a regular file    |
| `500`  | Internal error (for example, the file couldn't be opened) |
| `501`  | Method not implemented                                  |

Content types are chosen from the file extension: `html`/`htm`, `css`, `js`, `json`, `png`, `jpg`/`jpeg`, `gif` and `ico`. Anything else is sent as `application/octet-stream`.

## Project layout

```
.
├── Makefile
├── include/
│   ├── client.h      # request handling (includes the parser)
│   ├── parser.h      # HttpRequest and header parsing API
│   └── server.h      # Connection_t, buffer sizes, worker API
├── src/
│   ├── main.c        # socket setup, signal handling, forks the workers
│   ├── server.c      # per-worker epoll loop: accept, receive, write
│   ├── client.c      # routing, static files, response sending
│   └── parser.c      # request line / header / Content-Length parsing
├── tests/
│   └── test_http_parser.c
└── www/
    └── index.html    # the document root
```

## How it works

1. `main.c` creates one non-blocking listening socket, then forks `WORKER_COUNT` workers that all inherit it.
2. Each worker (`worker_process` in `server.c`) creates its own `epoll` instance and adds the listening socket with `EPOLLEXCLUSIVE`.
3. When a connection arrives, the worker accepts as many clients as are waiting and registers each one, edge-triggered, with its own `Connection_t` (input buffer, output buffer and file-transfer state).
4. On readable, the worker reads everything available and calls `handle_client`, which routes the request. For a `GET`, it validates the path, builds the headers in the output buffer and hands the open file to `conn_send`.
5. `conn_send` flushes the headers, then calls `sendfile()` until the whole file is sent. If the socket's send buffer fills up, it records where it stopped and the worker adds `EPOLLOUT` to the socket's events. When the socket is writable again, the transfer resumes from that offset and `EPOLLOUT` is removed.

## Known limitations

This is a work in progress. Current gaps:

- A request is assumed to arrive in a single read; the server doesn't yet wait for the end of the headers (`\r\n\r\n`) before handling it.
- Requests are limited to one 4 KB buffer, and the request-line parsing in `client.c` uses `sscanf`; the fuller parser in `parser.c` isn't connected to the server yet.
- `POST`/`PUT` and request bodies aren't supported yet.
- No `HEAD`, query-string handling or percent-decoding (`/index.html?x=1` returns `404`).
- Idle connections never time out, and `Connection: close` and HTTP/1.0 keep-alive rules aren't honored.
- If a worker dies it isn't restarted.
- Port and worker count can't be changed without recompiling.

## Testing

```sh
make test
```

This builds and runs the parser unit tests in `tests/`. Server behavior (large files, slow readers, concurrent downloads) can be checked by hand with `curl`:

```sh
curl -s http://localhost:8080/some-large-file | cmp - www/some-large-file
```

## License

No license has been chosen yet.
