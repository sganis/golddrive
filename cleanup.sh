#!/bin/sh
find . -type d -name "__pycache__" -exec rm -r "{}" \; 2>/dev/null
find . -type d -name "x64" -exec rm -r "{}" \; 2>/dev/null
git clean -dfx

