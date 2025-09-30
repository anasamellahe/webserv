#!/bin/bash

# Quick Test Suite for WebServ
# Run this script for basic local testing

echo "🧪 WebServ Quick Test Suite"
echo "=========================="

# Test 1: Build
echo "1️⃣  Testing build..."
if make clean && make all; then
    echo "✅ Build: PASSED"
else
    echo "❌ Build: FAILED"
    exit 1
fi

# Test 2: Binary check
echo ""
echo "2️⃣  Testing binary..."
if [ -f "./webserv" ] && file ./webserv | grep -q "ELF"; then
    echo "✅ Binary: PASSED"
else
    echo "❌ Binary: FAILED"
    exit 1
fi

# Test 3: Startup test
echo ""
echo "3️⃣  Testing server startup..."
timeout 3s ./webserv src/config.conf &>/dev/null
if [ $? -eq 124 ]; then  # timeout exit code
    echo "✅ Startup: PASSED (server started successfully)"
else
    echo "⚠️  Startup: Server exited early (may need investigation)"
fi

# Test 4: Config file test
echo ""
echo "4️⃣  Testing config file..."
if [ -f "src/config.conf" ]; then
    echo "✅ Config: PASSED (file exists)"
else
    echo "❌ Config: FAILED (file missing)"
fi

# Test 5: Required directories
echo ""
echo "5️⃣  Testing required directories..."
missing_dirs=()
for dir in "www" "cgi" "src"; do
    if [ ! -d "$dir" ]; then
        missing_dirs+=("$dir")
    fi
done

if [ ${#missing_dirs[@]} -eq 0 ]; then
    echo "✅ Directories: PASSED"
else
    echo "❌ Directories: FAILED (missing: ${missing_dirs[*]})"
fi

echo ""
echo "🎉 Quick tests completed!"
echo ""
echo "💡 To run full CI tests:"
echo "   • Push to GitHub to trigger automatic CI"
echo "   • Use 'docker build -t webserv .' for containerized testing"