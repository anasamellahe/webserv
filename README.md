# webserv (C++98 HTTP/1.1 server)

A lightweight HTTP/1.1 web server written in C++98 for Linux. It uses a non-blocking, single-threaded reactor based on poll() and supports static files, virtual hosting, routes, uploads, redirects, and asynchronous CGI.

## Features
- Single-process, single-threaded event loop (poll)
- Non-blocking sockets, partial reads/writes, HTTP keep-alive
- HTTP/1.1 request parsing, chunked transfer decoding, max body limits
- Virtual hosting (by host/server_name), multiple ports
- Routes with per-route root, index, directory listing, redirects
- Static file serving with simple content-type detection
- File uploads (multipart/form-data) to configurable directory
- Configurable error pages (e.g., 404.html, 500.html) using relative paths
- CGI support (async): .py/.pl/.cgi/.php with interpreter via cgi_pass
- Per-connection timeouts and graceful cleanup

## Repository layout
```
/home/sami/webserv
├── Makefile
├── *.conf                        # Example configs (relative paths)
├── src/
│   ├── main.cpp                  # Entry point (uses src/config.conf by default)
│   ├── CGI/CGIHandler.{hpp,cpp}  # Async CGI executor (startCGI/processCGIOutput)
│   ├── Server/                   # poll()-based event loop and client handling
│   ├── HTTP/                     # Request/Response parsing utilities
│   ├── methods/                  # GET/POST/DELETE handlers
│   └── Config/                   # Config parser and structures
└── www/                          # Web root (static files, errors, cgi examples)
```

## Build
Requirements: g++ (C++98), make, Linux.

- Build: `make`
- Clean: `make clean`
- Rebuild: `make re`

The build produces `./webserv` at the project root.

## Run
By default the server loads `src/config.conf`. You can pass any config file as the first argument.

- Run with default config:
  - `./webserv`
- Run with a specific config:
  - `./webserv config_cgi_test.conf`

The configs in this repo use relative paths (e.g., `./www/`) so they work on any Linux machine when run from the project root.

## Configuration
Configs are simple key/value blocks. Common directives:
- `host`, `port`, `server_name`, `root`
- `error_page <code> = <path>`
- `client_max_body_size`
- Route block keys:
  - `path`, `root`, `accepted_methods`, `directory_listing`, `index`
  - `cgi_enabled`, `cgi_extensions`, `cgi_pass`
  - `upload_enabled`, `upload_path`
  - `redirect` (e.g., `301 http://example.com` or local path)

Example (excerpt):
```
#server
host = 127.0.0.1
port = 8080
server_name = localhost
root = ./www/
error_page 404 = ./www/errors/404.html

a#route
path = /cgi-bin/
root = ./www/
accepted_methods = GET POST
cgi_enabled = true
cgi_extensions = .py .cgi
cgi_pass = /usr/bin/python3
```

## Testing
After `./webserv` starts (check console for bound ports), try:
- Static: http://127.0.0.1:8080/
- Fast CGI: http://127.0.0.1:8080/hello.py
- Slow CGI (should timeout): http://127.0.0.1:8080/slow_cgi.py
- PHP/Perl examples (if interpreters are installed): `www/info.php`, `www/test.pl`
- Directory listing (if enabled by route): `/files/` in relevant configs

Example CGI scripts in `www/` must output at least:
```
Content-Type: text/html

<html>...body...</html>
```

## Notes on CGI (async)
- CGI is started with non-blocking pipes. The poll loop watches the child’s stdout for POLLIN and POLLHUP to finalize even for short-lived scripts.
- The interpreter path is taken from `cgi_pass` per route; defaults are used for common extensions if omitted.
- For Python/Perl/PHP ensure the interpreter exists (e.g., `/usr/bin/python3`) and the script prints proper headers with a blank line.

## Troubleshooting
- Blank page / hanging on fast CGI:
  - Ensure the route has `cgi_enabled = true`, includes the file extension in `cgi_extensions`, and sets `cgi_pass`.
  - Verify script outputs a Content-Type header and a blank line before body.
  - Make sure the server is started from the project root so relative `./www/` paths resolve.
- 404 for files you expect: confirm route `root` and request path mapping; server canonicalizes and prevents path traversal.
- 405 Method Not Allowed: check `accepted_methods` in the matched route.
- Payload too large: increase `client_max_body_size` or send smaller bodies.

## Logging
The server prints helpful logs to stdout, e.g. configuration summary, accepted connections, CGI lifecycle, and timeouts.

