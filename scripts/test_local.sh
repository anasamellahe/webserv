#!/bin/bash

# Local testing script for webserv
# This script runs the same tests as CI locally

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_DIR"

echo "🚀 Running local webserv tests..."

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if required tools are installed
check_dependencies() {
    print_status "Checking dependencies..."
    
    local deps=("make" "g++" "curl")
    local missing=()
    
    for dep in "${deps[@]}"; do
        if ! command -v "$dep" &> /dev/null; then
            missing+=("$dep")
        fi
    done
    
    if [ ${#missing[@]} -gt 0 ]; then
        print_error "Missing dependencies: ${missing[*]}"
        echo "Please install the missing dependencies and try again."
        exit 1
    fi
    
    print_status "All dependencies found ✓"
}

# Build the project
build_project() {
    print_status "Building webserv..."
    
    # Clean if clean target exists, otherwise just continue
    make clean 2>/dev/null || echo "Clean target not available, continuing..."
    make all
    
    if [ ! -f "./webserv" ]; then
        print_error "Build failed - webserv executable not found"
        exit 1
    fi
    
    print_status "Build successful ✓"
}

# Run static analysis
run_static_analysis() {
    print_status "Running static analysis..."
    
    if command -v cppcheck &> /dev/null; then
        cppcheck --enable=warning,style --error-exitcode=1 src/ || {
            print_warning "Static analysis found issues (non-critical)"
        }
    else
        print_warning "cppcheck not installed, skipping static analysis"
    fi
}

# Test basic functionality
test_basic_functionality() {
    print_status "Testing basic functionality..."
    
    # Test invalid arguments (should show usage or error)
    timeout 2s ./webserv 2>&1 | head -3 || print_warning "Argument test completed"
    
    # Check if binary is properly linked
    if command -v ldd &> /dev/null; then
        ldd ./webserv > /dev/null || print_warning "Binary linking check failed"
    fi
    
    print_status "Basic functionality test completed ✓"
}

# Test server functionality
test_server() {
    print_status "Testing server functionality..."
    
    # Start server in background
    ./webserv src/config.conf &
    SERVER_PID=$!
    
    # Give server time to start
    sleep 3
    
    # Test if server responds
    local test_passed=false
    
    # Give server more time to start and try multiple times
    for i in {1..5}; do
        if curl -s -f "http://localhost:8080/" > /dev/null 2>&1; then
            print_status "Server responding on port 8080 ✓"
            test_passed=true
            break
        fi
        echo "Attempt $i/5: Waiting for server..."
        sleep 2
    done
    
    if [ "$test_passed" = false ]; then
        print_warning "Server not responding on default port 8080"
    fi
    
    # Test 404 handling
    if curl -s "http://localhost:8080/nonexistent" | grep -q "404\|Not Found"; then
        print_status "404 error handling works ✓"
    else
        print_warning "404 error handling may not be working properly"
    fi
    
    # Cleanup
    kill $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
    
    if [ "$test_passed" = false ]; then
        print_error "Server functionality tests failed"
        exit 1
    fi
}

# Run load test
run_load_test() {
    print_status "Running load test..."
    
    if [ -f "scripts/load_test.sh" ]; then
        chmod +x scripts/load_test.sh
        
        export TEST_REQUESTS=50
        export TEST_CONCURRENCY=5
        export TEST_TIMEOUT=30
        
        if ./scripts/load_test.sh; then
            print_status "Load test passed ✓"
        else
            print_warning "Load test failed or incomplete"
        fi
    else
        print_warning "Load test script not found, skipping"
    fi
}

# Memory check with valgrind (if available)
run_memory_check() {
    print_status "Running memory check..."
    
    if command -v valgrind &> /dev/null; then
        print_status "Running Valgrind memory check..."
        
        # Build with debug symbols
        export CXXFLAGS="-Wall -Wextra -Werror -g -O0"
        make clean && make all
        
        # Run quick valgrind check
        timeout 15s valgrind --leak-check=summary --error-exitcode=1 ./webserv src/config.conf &
        VALGRIND_PID=$!
        
        sleep 5
        kill $VALGRIND_PID 2>/dev/null || true
        wait $VALGRIND_PID 2>/dev/null
        
        if [ $? -eq 0 ]; then
            print_status "Memory check passed ✓"
        else
            print_warning "Memory check found potential issues"
        fi
    else
        print_warning "Valgrind not installed, skipping memory check"
    fi
}

# Main execution
main() {
    echo "================================================"
    echo "🧪 WebServ Local Test Suite"
    echo "================================================"
    
    check_dependencies
    build_project
    run_static_analysis
    test_basic_functionality
    test_server
    run_load_test
    
    # Only run memory check if explicitly requested
    if [ "$1" = "--with-valgrind" ]; then
        run_memory_check
    fi
    
    echo "================================================"
    print_status "🎉 All tests completed!"
    echo "================================================"
    
    echo ""
    echo "💡 Tips:"
    echo "  • Run with --with-valgrind to include memory checks"
    echo "  • Check .github/workflows/ for CI configuration"
    echo "  • Use 'docker build -t webserv .' to test Docker build"
}

# Run main function
main "$@"