#!/usr/bin/env python3
import time
import os

print("Content-Type: text/html")
print("")
print("<html><body>")
print("<h1>Slow CGI Script Test</h1>")
print("<p>Starting long operation...</p>")

# Flush output so we can see this immediately
import sys
sys.stdout.flush()

# Sleep for 10 seconds (longer than our 5-second timeout)
print("<p>Sleeping for 10 seconds...</p>")
sys.stdout.flush()

time.sleep(10)

print("<p>This should never be seen due to timeout!</p>")
print("</body></html>")
