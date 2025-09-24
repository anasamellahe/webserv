# Multi-stage build for webserv
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    g++ \
    make \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source code
COPY . .

# Build the project
RUN make clean && make all

# Runtime stage
FROM ubuntu:22.04 AS runtime

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    python3 \
    php-cgi \
    perl \
    && rm -rf /var/lib/apt/lists/*

# Create non-root user
RUN groupadd -r webserv && useradd -r -g webserv webserv

# Set working directory
WORKDIR /app

# Copy binary from builder stage
COPY --from=builder /app/webserv ./
COPY --from=builder /app/src/config.conf ./config.conf
COPY --from=builder /app/www ./www
COPY --from=builder /app/cgi ./cgi

# Change ownership
RUN chown -R webserv:webserv /app

# Switch to non-root user
USER webserv

# Expose port
EXPOSE 8080

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:8080/ || exit 1

# Run the server
CMD ["./webserv", "config.conf"]