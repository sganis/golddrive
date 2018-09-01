#!/bin/sh
find . -type d -name "__pycache__" -exec rm -r "{}" \;
find . -type d -name "x64" -exec rm -r "{}" \; 
find . -type d -name ".vs" -exec rm -r "{}" \; 
git clean -dfx

