#!/usr/bin/env python3
import time
import sys

sys.stdout.write("Content-Type: text/plain\r\n\r\n")
sys.stdout.flush()

for i in range(5):
    sys.stdout.write(f"chunk-{i}\n")
    sys.stdout.flush()
    time.sleep(0.5)