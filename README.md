# Webserv - C++ 98 HTTP/1.1 Server

A high-performance, RFC-compliant HTTP/1.1 web server implemented from scratch in C++ 98. This project demonstrates low-level networking, non-blocking I/O multiplexing, and a robust state-machine based request parsing architecture.

## 🏗️ Technical Architecture

The server operates on a **Reactor Pattern** using a single-threaded event loop powered by `poll()` (or `select`). This ensures high concurrency without the overhead of multi-threading.

### Key Components:
- **I/O Multiplexing:** Utilizes a non-blocking event loop to monitor multiple file descriptors (sockets) simultaneously.
- **Request Parsing:** A robust state-machine parser handles complex HTTP/1.1 requests, including chunked transfer-encoding and multipart/form-data.
- **Configuration Engine:** Custom recursive-descent parser for NGINX-style configuration files, supporting virtual hosting and complex routing.
- **CGI Execution:** Asynchronous CGI execution using non-blocking pipes, allowing for dynamic content generation (Python, PHP, Perl) without blocking the main event loop.

## 🚀 Tech Stack
- **Language:** C++ 98 (Strict adherence to the standard)
- **Networking:** POSIX Sockets, TCP/IP
- **Event Loop:** `poll()` / `select()`
- **Build System:** Makefile
- **Environment:** Linux / macOS

## ✨ Key Features
- **Non-blocking I/O:** Efficiently handles hundreds of concurrent connections.
- **Virtual Hosting:** Support for multiple server blocks differentiated by `host` and `server_name`.
- **Custom Routing:** Per-route configuration for roots, indexes, directory listing, and HTTP redirects.
- **File Management:** Built-in support for file uploads via POST requests.
- **Error Handling:** Configurable custom error pages for all standard HTTP error codes.
- **Graceful Shutdown:** Proper signal handling for clean socket closure and resource deallocation.

## 🛠️ Setup Instructions

### Prerequisites
- `g++` compiler
- `make`
- Linux or macOS environment

### Installation
1. Clone the repository:
   ```bash
   git clone https://github.com/anasamellahe/webserv.git
   cd webserv
   ```
2. Build the project:
   ```bash
   make
   ```
3. Run the server:
   ```bash
   ./webserv [config_file]
   ```
   *If no config file is provided, it defaults to `src/config.conf`.*

## 🧪 Testing
The server can be tested using standard tools like `curl`, `Postman`, or any modern web browser.
```bash
curl -v http://localhost:8080
```

---
*Developed as part of the 42 Network curriculum.*
