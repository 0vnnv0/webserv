#!/usr/bin/env python3
# print("Content-Type: text/html")
# print()  # Empty line separates headers from body
# print("<html><body>")
# print("<h1>Hello from CGI!</h1>")
# print("</body></html>")
# ```

# **Your server's response:**
# ```
# HTTP/1.1 200 OK
# Content-Type: text/html

# <html><body>
# <h1>Hello from CGI!</h1>
# </body></html>
print("Content-Type: text/html\r\n\r\n")
print("<html><body>")
print("<h1>CGI Test Success!</h1>")
print("</body></html>")