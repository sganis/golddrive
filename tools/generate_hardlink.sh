#!/bin/sh
# generate hard link test data

rm -f doc.txt link.txt
echo "hello world" > doc.txt
ln doc.txt link.txt
echo "a new file" > ../folder2/doc.txt
