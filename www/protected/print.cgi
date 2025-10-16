#!/usr/bin/env python3
import os
import sys

# CGI headers
print("Content-Type: text/html")
print("")  # Empty line required between headers and body

# HTML content
print("<html>")
print("<head><title>CGI Test</title></head>")
print("<body>")
print("<h1>CGI Script Working!</h1>")
print("<p>Request Method: {}</p>".format(os.environ.get('REQUEST_METHOD', 'Unknown')))
print("<p>Query String: {}</p>".format(os.environ.get('QUERY_STRING', 'None')))
print("<p>Content Type: {}</p>".format(os.environ.get('CONTENT_TYPE', 'None')))
print("<p>Content Length: {}</p>".format(os.environ.get('CONTENT_LENGTH', '0')))
print("<p>Script Name: {}</p>".format(os.environ.get('SCRIPT_NAME', 'Unknown')))
print("<p>Server Name: {}</p>".format(os.environ.get('SERVER_NAME', 'Unknown')))
print("<p>Server Port: {}</p>".format(os.environ.get('SERVER_PORT', 'Unknown')))

# Show POST data if any
if os.environ.get('REQUEST_METHOD') == 'POST':
    content_length = int(os.environ.get('CONTENT_LENGTH', 0))
    if content_length > 0:
        post_data = sys.stdin.read(content_length)
        print("<p>POST Data: {}</p>".format(post_data))

print("</body>")
print("</html>")
