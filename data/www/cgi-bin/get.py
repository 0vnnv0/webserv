#!/usr/bin/env python3
import os

print("Content-Type: text/plain\n")

# Just report the query string and cwd
print("QUERY_STRING:", os.environ.get("QUERY_STRING", ""))
print("Current directory:", os.getcwd())

##expected:
## query string 
## + the current working directory - should be the one where this file is located.