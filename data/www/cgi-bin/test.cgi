#!/usr/bin/env python3
import os
import sys
import urllib.parse

# HTTP header
print("Content-Type: text/plain")
print("")

print("CGI Test Script Running")
print("REQUEST_METHOD:", os.environ.get("REQUEST_METHOD", ""))

# GET
qs = os.environ.get("QUERY_STRING", "")
print("QUERY_STRING:", qs)
if qs:
    params = urllib.parse.parse_qs(qs)
    print("Parsed GET parameters:", params)

# POST
cl = os.environ.get("CONTENT_LENGTH")
if cl:
    try:
        length = int(cl)
        data = sys.stdin.read(length)
        print("POST data:", data)
    except:
        print("Failed to read POST data")
