# WebServ

A high-performance HTTP/1.1 server implementation in C++.

[![CI](https://github.com/anasamellahe/webserv/actions/workflows/ci.yml/badge.svg)](https://github.com/anasamellahe/webserv/actions/workflows/ci.yml)
[![Advanced Testing](https://github.com/anasamellahe/webserv/actions/workflows/advanced-testing.yml/badge.svg)](https://github.com/anasamellahe/webserv/actions/workflows/advanced-testing.yml)
[![Docker](https://github.com/anasamellahe/webserv/actions/workflows/deploy.yml/badge.svg)](https://github.com/anasamellahe/webserv/actions/workflows/deploy.yml)

## Features

- **HTTP/1.1 Protocol Support**: Full implementation of HTTP/1.1 specification
- **CGI Support**: Execute Python, PHP, and Perl scripts
- **Static File Serving**: Efficient serving of static content
- **Multi-port Binding**: Support for multiple server configurations
- **Chunked Transfer Encoding**: Handle large request/response bodies
- **Custom Error Pages**: Configurable error page handling
- **Virtual Hosts**: Multiple server configurations per instance

## Quick Start

### Building

```bash
# Clone the repository
git clone https://github.com/anasamellahe/webserv.git
cd webserv

# Build the server
make

# Run the server
./webserv src/config.conf
```

### Using Docker

```bash
# Build Docker image
docker build -t webserv .

# Run container
docker run -p 8080:8080 webserv
```

## Development

### Local Testing

```bash
# Run comprehensive local tests
./scripts/test_local.sh

# Run with memory checks (requires valgrind)
./scripts/test_local.sh --with-valgrind

# Run load test only
./scripts/load_test.sh
```

### CI/CD Pipeline

The project uses GitHub Actions for continuous integration with multiple workflows:

#### Main CI Pipeline (`.github/workflows/ci.yml`)
- **Multi-compiler builds**: Tests with both GCC and Clang
- **Debug/Release builds**: Validates both optimization levels  
- **Static analysis**: Runs cppcheck for code quality
- **Security scanning**: CodeQL analysis for vulnerabilities
- **Memory leak detection**: Valgrind integration
- **Load testing**: Performance validation
- **Docker builds**: Container compatibility testing

#### Advanced Testing (`.github/workflows/advanced-testing.yml`)
- **Unit tests**: Comprehensive test coverage
- **Integration tests**: End-to-end functionality validation
- **Performance benchmarks**: Apache Bench integration
- **Cross-platform builds**: Ubuntu and macOS compatibility
- **Coverage reporting**: Code coverage with Codecov

#### Deployment (`.github/workflows/deploy.yml`)
- **Docker registry**: Automated image publishing to GHCR
- **GitHub releases**: Binary releases for tagged versions
- **Multi-architecture support**: Optimized builds

## Testing

### Automated Tests

The CI pipeline runs several types of tests:

1. **Build Tests**: Verify compilation across different compilers and configurations
2. **Functional Tests**: Basic HTTP request/response validation
3. **CGI Tests**: Script execution and response handling  
4. **Load Tests**: Performance under concurrent connections
5. **Memory Tests**: Leak detection and resource usage validation
6. **Security Tests**: Static analysis and vulnerability scanning

### Manual Testing

```bash
# Test basic functionality
curl http://localhost:8080/

# Test file serving
curl http://localhost:8080/css/style.css

# Test CGI scripts
curl http://localhost:8080/cgi-bin/hello.py

# Test error handling
curl http://localhost:8080/nonexistent
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Run local tests: `./scripts/test_local.sh`
5. Submit a pull request

All pull requests trigger the full CI pipeline.

## License

This project is part of the 42 School curriculum.
# CI Pipeline Status

This project includes comprehensive CI/CD automation.
