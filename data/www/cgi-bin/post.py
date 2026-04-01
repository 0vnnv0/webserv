#!/usr/bin/env python3
import os
import sys

print("Content-Type: text/plain\n")

content_length = os.environ.get("CONTENT_LENGTH")
post_data = ""
if content_length:
    post_data = sys.stdin.read(int(content_length))

print("POST data received:", post_data)
print("Current directory:", os.getcwd())


## expected: POST data received: HelloServer
## + current working dir