#!/bin/sh
for i in $(seq 0 7); do
	mode="7${i}0"
	echo $mode	
 	echo hello > ${mode}.txt
 	chmod ${mode} ${mode}.txt 
done