#!/usr/bin/python3
import sys
import os

body = sys.stdin.read()

print("Content-Type: text/plain")
print()
print(body)
