#!/usr/bin/env python3
import os
import sys
import urllib.parse
from datetime import datetime

# Print CGI headers
print("Content-type: text/html\r\n\r\n")

# Read POST data if present
post_data = ""
if os.environ.get("REQUEST_METHOD") == "POST":
    content_length = int(os.environ.get("CONTENT_LENGTH", 0))
    if content_length > 0:
        post_data = sys.stdin.read(content_length)

# Parse query string
query_string = os.environ.get("QUERY_STRING", "")
query_params = urllib.parse.parse_qs(query_string)

print("""<!DOCTYPE html>
<html>
<head>
    <title>CGI Test - GET and POST</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 40px; }}
        .section {{ margin: 20px 0; padding: 20px; border: 1px solid #ccc; }}
        .method {{ background-color: #f0f8ff; }}
        .env {{ background-color: #f5f5f5; }}
        input, textarea {{ margin: 5px; padding: 5px; }}
        button {{ padding: 10px 20px; margin: 5px; }}
    </style>
</head>
<body>
    <h1>CGI Test Script</h1>
    <p><strong>Current time:</strong> {}</p>
""".format(datetime.now()))

print("""
    <div class="section method">
        <h2>Request Information</h2>
        <p><strong>Method:</strong> {}</p>
        <p><strong>Query String:</strong> {}</p>
        <p><strong>Content Length:</strong> {}</p>
""".format(
    os.environ.get("REQUEST_METHOD", "Unknown"),
    query_string,
    os.environ.get("CONTENT_LENGTH", "0")
))

if query_params:
    print("<h3>GET Parameters:</h3><ul>")
    for key, values in query_params.items():
        for value in values:
            print(f"<li><strong>{key}:</strong> {value}</li>")
    print("</ul>")

if post_data:
    print(f"<h3>POST Data:</h3><pre>{post_data}</pre>")
    # Try to parse as form data
    try:
        post_params = urllib.parse.parse_qs(post_data)
        if post_params:
            print("<h3>Parsed POST Parameters:</h3><ul>")
            for key, values in post_params.items():
                for value in values:
                    print(f"<li><strong>{key}:</strong> {value}</li>")
            print("</ul>")
    except:
        pass

print("</div>")

print("""
    <div class="section">
        <h2>Test Forms</h2>
        
        <h3>GET Test</h3>
        <form method="GET" action="/test_cgi.py">
            <input type="text" name="name" placeholder="Your name" value="Mehdi">
            <input type="text" name="age" placeholder="Your age" value="25">
            <button type="submit">Submit GET</button>
        </form>
        
        <h3>POST Test</h3>
        <form method="POST" action="/test_cgi.py">
            <input type="text" name="title" placeholder="Title" value="GHANSEFT CHI HAJA">
            <textarea name="message" placeholder="Your message">Hello from POST!</textarea>
            <button type="submit">Submit POST</button>
        </form>
    </div>
""")

print("""
    <div class="section env">
        <h2>Environment Variables</h2>
        <table border="1" style="border-collapse: collapse;">
            <tr><th>Variable</th><th>Value</th></tr>
""")

# Display relevant CGI environment variables
cgi_vars = [
    "REQUEST_METHOD", "SCRIPT_NAME", "SCRIPT_FILENAME", "QUERY_STRING",
    "CONTENT_TYPE", "CONTENT_LENGTH", "SERVER_NAME", "SERVER_PORT",
    "SERVER_PROTOCOL", "GATEWAY_INTERFACE", "HTTP_HOST", "HTTP_USER_AGENT",
    "REMOTE_ADDR", "REMOTE_HOST"
]

for var in cgi_vars:
    value = os.environ.get(var, "")
    print(f"<tr><td><strong>{var}</strong></td><td>{value}</td></tr>")

print("""
        </table>
    </div>
</body>
</html>""")