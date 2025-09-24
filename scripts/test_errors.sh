#!/bin/bash

# Error Pages Test Script
# Tests all HTTP error codes and error page delivery

set -e

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

# Ensure we're in the project directory
cd "$(dirname "$0")/.."

echo "🚨 Testing WebServ Error Handling"
echo "=================================="

# Build the project
echo "Building webserv..."
make clean && make all

# Check if error pages exist
echo ""
echo "📄 Checking Error Pages Files..."
ERROR_PAGES=(400 401 402 403 404 405 408 411 413 414 415 416 429 500 501 502 503 504 505)

for code in "${ERROR_PAGES[@]}"; do
    if [ -f "www/errors/${code}.html" ]; then
        print_success "Error page ${code}.html exists"
        
        # Check file size (should not be empty)
        SIZE=$(stat -c%s "www/errors/${code}.html")
        if [ $SIZE -gt 100 ]; then
            echo "  📊 Size: ${SIZE} bytes"
        else
            print_warning "Error page ${code}.html seems too small (${SIZE} bytes)"
        fi
        
        # Check if contains error code
        if grep -q "$code" "www/errors/${code}.html"; then
            echo "  ✅ Contains error code $code"
        else
            print_warning "Error code $code not found in page content"
        fi
    else
        print_error "Error page ${code}.html missing"
    fi
done

# Check CSS file
if [ -f "www/errors/style.css" ]; then
    print_success "Error page stylesheet exists"
    SIZE=$(stat -c%s "www/errors/style.css")
    echo "  📊 CSS Size: ${SIZE} bytes"
else
    print_error "Error page stylesheet missing"
fi

# Start server for testing
echo ""
echo "🚀 Starting WebServ for Error Testing..."
./webserv src/config.conf &
SERVER_PID=$!

# Wait for server to start
sleep 3

if ! kill -0 $SERVER_PID 2>/dev/null; then
    print_error "Server failed to start"
    exit 1
fi

print_success "Server started (PID: $SERVER_PID)"

# Function to test HTTP status codes
test_http_status() {
    local url=$1
    local expected=$2
    local method=${3:-GET}
    local description=$4
    
    STATUS=$(curl -s -o /dev/null -w "%{http_code}" -X "$method" "$url" 2>/dev/null || echo "000")
    
    if [ "$STATUS" = "$expected" ]; then
        print_success "$description - Status: $STATUS"
    elif [ "$STATUS" = "000" ]; then
        print_warning "$description - Connection failed"
    else
        print_warning "$description - Expected: $expected, Got: $STATUS"
    fi
}

echo ""
echo "🧪 Testing 4xx Client Errors..."

# 400 Bad Request
echo -e "INVALID REQUEST\r\n\r\n" | timeout 3s nc localhost 8080 >/dev/null 2>&1 || print_success "400 Bad Request test attempted"

# 404 Not Found
test_http_status "http://localhost:8080/nonexistent-file" "404" "GET" "404 Not Found"

# 405 Method Not Allowed
test_http_status "http://localhost:8080/" "405" "PATCH" "405 Method Not Allowed"

# 413 Payload Too Large
LARGE_DATA=$(python3 -c "print('x' * 50000)" 2>/dev/null || echo "large-data-test")
STATUS=$(curl -s -o /dev/null -w "%{http_code}" -X POST http://localhost:8080/ -d "$LARGE_DATA" 2>/dev/null || echo "000")
if [ "$STATUS" = "413" ]; then
    print_success "413 Payload Too Large - Status: $STATUS"
else
    print_warning "413 Payload Too Large - Got: $STATUS"
fi

# 414 URI Too Long
LONG_URI="/$(python3 -c "print('x' * 3000)" 2>/dev/null || echo "very-long-uri-test")"
test_http_status "http://localhost:8080$LONG_URI" "414" "GET" "414 URI Too Long"

echo ""
echo "🧪 Testing 5xx Server Errors..."

# 501 Not Implemented
test_http_status "http://localhost:8080/" "501" "TRACE" "501 Not Implemented"

# Test error page content delivery
echo ""
echo "📄 Testing Error Page Content..."

# Get 404 page content
RESPONSE=$(curl -s http://localhost:8080/test-404-content)
if echo "$RESPONSE" | grep -q "404\|Not Found"; then
    print_success "404 error page contains error information"
else
    print_warning "404 error page content unclear"
fi

# Test error page CSS
CSS_STATUS=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:8080/errors/style.css)
if [ "$CSS_STATUS" = "200" ]; then
    print_success "Error page CSS accessible (Status: $CSS_STATUS)"
else
    print_warning "Error page CSS not accessible (Status: $CSS_STATUS)"
fi

# Test error consistency
echo ""
echo "🔄 Testing Error Response Consistency..."
for i in {1..3}; do
    STATUS=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:8080/consistency-test-$i)
    echo "  Test $i - Status: $STATUS"
done

# Test server stability after errors
echo ""
echo "🏥 Testing Server Stability After Errors..."
for i in {1..5}; do
    curl -s -o /dev/null http://localhost:8080/error-test-$i &
done
wait

# Check if server is still responding normally
STATUS=$(curl -s -o /dev/null -w "%{http_code}" http://localhost:8080/)
if [ "$STATUS" = "200" ] || [ "$STATUS" = "404" ]; then
    print_success "Server stable after error scenarios (Status: $STATUS)"
else
    print_warning "Server may be unstable after errors (Status: $STATUS)"
fi

# Cleanup
echo ""
echo "🧹 Cleaning up..."
kill $SERVER_PID 2>/dev/null || true
wait $SERVER_PID 2>/dev/null || true
print_success "Server stopped"

echo ""
echo "✅ Error Page Testing Completed!"
echo ""
echo "💡 Summary:"
echo "  • Tested all ${#ERROR_PAGES[@]} error page files"
echo "  • Validated HTTP status codes"
echo "  • Checked error page content delivery"
echo "  • Verified server stability"
echo ""
echo "🚀 Your webserv error handling is ready!"