#!/usr/bin/env python

import sys
import os
import subprocess

assert len(sys.argv) > 1 # usage: prog user@host

userhost = sys.argv[1]

def run(cmd):
	return subprocess.run(cmd, capture_output=True, shell=True, text=True)

# get ciphers
ciphers = run(f'ssh -Q cipher').stdout.split()
print(ciphers)

# get ciphers
# p = run(f'ssh -vv -oStrictHostKeyChecking=no {userhost} hostname')
# ciphers = ''
# for line in p.stderr.split('\n'):
# 	if 'ciphers stoc' in line:
# 		ciphers = line.split()[3]
# 		break
# ciphers = ciphers.split(',')
# print(ciphers)

# create a 1gb file'
bigfile = 'file.bin'
if not os.path.exists(bigfile):
	with open(bigfile,'wb') as w:
		w.write(os.urandom(1024*1024*1024))
		# w.write(os.urandom(1024))


for c in ciphers:
	print(f'testing {c}...')
	p = run(f'ssh -c {c} {userhost} "time -p cat > {bigfile}" < {bigfile}')
	if p.returncode == 0:
		print(f'\tupload  : {p.stderr.split()[1]}')
		p = run(f'ssh -c {c} {userhost} "time -p cat {bigfile}" > {bigfile}-copy')
		print(f'\tdownload: {p.stderr.split()[1]}')
	else:
		print(p.stderr)




