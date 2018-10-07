# Get uid from windows sid
# cygwin idmap for domain accounts: 0x30000 + RID

import os
import subprocess
import getpass

user=getpass.getuser()
sid = ''
p = subprocess.run(f'wmic useraccount where name="{user}" get name,sid', 
					shell=True, capture_output=True, text=True)
for line in p.stdout.split('\n'):
	if user in line:
		sid = line.split()[-1]
		rid = sid.split('-')[-1]
		break

print('\npython:')
print(f'sid: {sid}')
print(f'rid: {rid}')

uid = int(0x30000) + int(rid)
print(f'uid: {uid}')

cygwin_id = fr'C:\cygwin64\bin\id.exe'
if os.path.exists(cygwin_id):
	p = subprocess.run(f'"{cygwin_id}"', shell=True, capture_output=True, text=True)
	print('\ncygwin id:')
	print(p.stdout.strip())

fsptool = fr'c:\Program Files (x86)\WinFsp\bin\fsptool-x64.exe'
if os.path.exists(fsptool):
	p = subprocess.run(f'"{fsptool}" id', shell=True, capture_output=True, text=True)
	print('\nfsptool:')
	print(p.stdout.strip())