1. 400 Bad Request

    Cause: General syntax/structural errors in the request.

    Examples:

        Missing/invalid HTTP method (e.g., GIT instead of GET).

        Missing request line (e.g., GET HTTP/1.1 without a path).

        Invalid header formatting (e.g., Content-Length: abc).

2. 405 Method Not Allowed

    Cause: Invalid HTTP method for the requested resource.

    Example:

        Sending a POST request to a resource that only supports GET.

3. 411 Length Required

    Cause: Missing Content-Length header when required.

    Example:

        POST request without Content-Length or Transfer-Encoding: chunked.

4. 413 Payload Too Large

    Cause: Request body exceeds server limits.

    Example:

        Uploading a 10GB file when the server allows a maximum of 1MB.

5. 414 URI Too Long

    Cause: Request URI exceeds server limits.

    Example:

        A URL with 10,000 characters when the server allows 2,048.

6. 415 Unsupported Media Type

    Cause: Unsupported Content-Type in the request body.

    Example:

        Sending Content-Type: application/octet-stream when the server expects application/json.

7. 431 Request Header Fields Too Large

    Cause: Request headers exceed server size limits.

    Example:

        A single header field (e.g., Cookie) is larger than allowed.

8. 505 HTTP Version Not Supported

    Cause: Invalid HTTP protocol version.

    Example:

        Using HTTP/2.0 when the server only supports HTTP/1.1.# webserv
