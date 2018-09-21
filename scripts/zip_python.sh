#!/bin/sh
#
# Embed python
# 1. Generate legacy bytecode
# cd $app/lib
# python -m compileall -b .
# 2. Delete source code
# find . -type f -name "*.py" -delete
# 3. Zip lib folder as python37.zip
# 4. Delete lib folder
