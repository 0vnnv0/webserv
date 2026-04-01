#!/usr/bin/python3
import cgi, sys

form = cgi.FieldStorage()
fileitem = form['file']

if fileitem.filename:
    content = fileitem.file.read()
else:
    content = b''

print("Content-Type: text/html")
print()
print("<html><body>POST received</body></html>")