#!/usr/bin/env bash

# Load test script for webserv
set -e

URL="${TEST_URL:-http://127.0.0.1:8080/}"
TOTAL="${TEST_REQUESTS:-100}"
CONCURRENCY="${TEST_CONCURRENCY:-10}"
TIMEOUT="${TEST_TIMEOUT:-30}"

echo "Starting load test..."
echo "URL: $URL"
echo "Total requests: $TOTAL"
echo "Concurrency: $CONCURRENCY"
echo "Timeout: ${TIMEOUT}s"

# Start webserv in background if not already running
if ! curl -s -f "$URL" > /dev/null 2>&1; then
    echo "Starting webserv server..."
    ./webserv src/config.conf &
    SERVER_PID=$!
    
    # Wait for server to start
    for i in {1..10}; do
        if curl -s -f "$URL" > /dev/null 2>&1; then
            echo "Server is ready!"
            break
        fi
        echo "Waiting for server to start... ($i/10)"
        sleep 1
    done
    
    # Check if server started successfully
    if ! curl -s -f "$URL" > /dev/null 2>&1; then
        echo "Error: Server failed to start"
        kill $SERVER_PID 2>/dev/null || true
        exit 1
    fi
else
    echo "Server is already running"
    SERVER_PID=""
fi

# Function to cleanup
cleanup() {
    if [ -n "$SERVER_PID" ]; then
        echo "Stopping server..."
        kill $SERVER_PID 2>/dev/null || true
        wait $SERVER_PID 2>/dev/null || true
    fi
}
trap cleanup EXIT

# Run the load test
echo "Running load test..."
FAILED=0
SUCCESS=0

# Create temporary file for results
RESULTS=$(mktemp)

# Run requests with timeout
timeout ${TIMEOUT}s bash -c "
    seq $TOTAL | xargs -n1 -P $CONCURRENCY -I{} curl -s -o /dev/null -w '%{http_code}\n' '$URL'
" > "$RESULTS" 2>/dev/null || true

# Analyze results
if [ -f "$RESULTS" ]; then
    TOTAL_RESPONSES=$(wc -l < "$RESULTS")
    SUCCESS=$(grep -c '^200$' "$RESULTS" 2>/dev/null || echo "0")
    FAILED=$((TOTAL_RESPONSES - SUCCESS))
    
    echo "Load test completed!"
    echo "Total responses: $TOTAL_RESPONSES"
    echo "Successful (200): $SUCCESS"
    echo "Failed: $FAILED"
    
    # Show response code distribution
    echo "Response code distribution:"
    sort "$RESULTS" | uniq -c | sort -nr
    
    rm -f "$RESULTS"
    
    # Consider test successful if at least 80% of requests succeeded
    if [ $TOTAL_RESPONSES -gt 0 ]; then
        SUCCESS_RATE=$((SUCCESS * 100 / TOTAL_RESPONSES))
        echo "Success rate: ${SUCCESS_RATE}%"
        
        if [ $SUCCESS_RATE -lt 80 ]; then
            echo "Error: Success rate below 80%"
            exit 1
        fi
    fi
else
    echo "No results file found - test may have been interrupted"
    exit 1
fi

echo "Load test passed!"
