#!/usr/bin/python3
import os

path_info = os.environ.get("PATH_INFO", "")
print("Content-Type: text/plain")
print()
print(path_info)