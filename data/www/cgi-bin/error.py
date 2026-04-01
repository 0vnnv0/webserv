#!/usr/bin/env python3

print("Content-Type: text/plain\n")

# Force an exception
raise Exception("Intentional error for testing")


## will exit with code 1
## respond with 502