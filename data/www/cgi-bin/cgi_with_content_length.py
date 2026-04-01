#!/usr/bin/python3
body = "<html><body>Content-Length test</body></html>"

print("Content-Type: text/html")
print("Content-Length:", len(body))
print()
print(body)