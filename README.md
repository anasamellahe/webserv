# webserv
🧠 HTTP Metadata (Headers): Format Rules
Every HTTP header must be in this format:

makefile
Copy
Edit
field-name: field-value
✅ field-name → the key (like Host, Content-Type)
✅ field-value → the value (like example.com, application/json)

📜 What the HTTP specification (RFC 7230) says about field-value:
1. Field-name (the key) must:
Be case-insensitive (meaning Host and host are the same)

Only use visible ASCII letters, digits, and special characters:

Allowed characters: A-Z, a-z, 0-9, and these symbols: ! # $ % & ' * + - . ^ _ | ~`

Spaces are not allowed in the key!

✅ Example of valid keys: Content-Type, X-Custom-Header

2. Field-value (the value) must:
Contain printable ASCII characters only (characters between ASCII 33 and 126).

Can contain spaces inside the value (but no line breaks or control characters).

Must not contain unescaped control characters (like tabs \t, carriage return \r, newline \n).

🎯 In simple terms:

Rule	Meaning
Only printable ASCII (except DEL, no control characters)	✅ Values must be readable
Spaces are allowed	✅ Yes (inside value)
Line breaks are forbidden inside the value	❌ No
Unicode (like Arabic, Chinese)	❌ Not allowed directly (needs special encoding if needed)
⚡ Examples:
✅ Good header:

css
Copy
Edit
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)
✅ Good header with custom metadata:

arduino
Copy
Edit
X-Client-Metadata: version=1.2.3 platform=web
❌ Bad header (illegal control character):

pgsql
Copy
Edit
X-Bad-Header: hello \n world   <-- ❌ newline inside value is forbidden
🔥 Special notes:
If you need to send special characters (like emojis, or non-ASCII text), you must encode them (example: use Base64 or percent-encoding).

Otherwise, HTTP expects only simple clean ASCII.

🚀 Final Summary:

Part	Rules
Key (field-name)	Letters, digits, and special characters like - _ . (NO spaces)
Value (field-value)	Printable ASCII (space OK, but no newlines, no tabs, no control characters)
Would you like me to also show you how to validate a header field programmatically (for example, with a small Python or JavaScript function)? 🚀
It’s very cool and helpful if you're building parsers! 🎯