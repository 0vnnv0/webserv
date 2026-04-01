#!/usr/bin/env python3
import os

# Required HTTP header
print("Content-Type: text/plain")
print("")

# Show the QUERY_STRING
query_string = os.environ.get("QUERY_STRING", "")
print("QUERY_STRING:", query_string)
