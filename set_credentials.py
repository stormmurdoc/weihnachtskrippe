#!/usr/bin/python

import subprocess
import os

out=subprocess.run(["pass", "arduino/ota", "/dev/null"], capture_output=True)
#print(out.stdout.decode('UTF-8').strip())

os.environ["OTA_PASSWORD"]=out.stdout.decode('UTF-8').strip()

