#!/usr/bin/env python3
import os
import sys
from datetime import datetime

print("Content-type: text/html\r\n\r\n")
print("<html><head><title>Hello CGI</title></head>")
print("<body>")
print("<h1>Hello, CGI World!</h1>")
print("<p>This is a simple Python CGI script.</p>")
print("<h2>Environment Variables:</h2>")
print("<ul>")
for key in sorted(os.environ.keys()):
    print(f"<li><strong>{key}</strong>: {os.environ[key]}</li>")
print("</ul>")
print(f"<p>Current time: {datetime.now()}</p>")
print("</body></html>")