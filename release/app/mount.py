# mount drive using sshfs
# 
import util
import logging

logger = logging.getLogger('ssh-drive')

def set_drive_name(name, user, host):
	print(f'Setting drive name as {name}...')
	key = fr'HKCU\Software\Microsoft\Windows\CurrentVersion'
	key = fr'{key}\Explorer\MountPoints2\##sshfs#{user}@{host}'
	cmd = f'reg add {key} /v _LabelFromReg /d {name} /f 2>nul'
	util.run(cmd)

def main(sshfs, drive, user, host, port=22, drivename=''):
	print(f'Mounting {drive} {user}@{host}...')
	
	cmd = f'''
		"{sshfs}" {user}@{host}:/ {drive} 
		-oport={port}
		-oVolumePrefix=/sshfs/{user}@{host}
		-ovolname={drivename}-{user}@{host} 
		-ouid=-1,gid=-1,create_umask=0007 
		-orellinks -oreconnect
		-oFileSystemName=SSHFS 
		-oStrictHostKeyChecking=no
		-oServerAliveInterval=10 
		-oServerAliveCountMax=10000
		-oFileInfoTimeout=10000 
		-oDirInfoTimeout=10000 
		-oVolumeInfoTimeout=10000
		-omax_readahead=131072
		'''.replace('\n',' ').replace('\t','')

		# google mount: "-o" "max_readahead=131072"
		
	# print(cmd)
	util.run(cmd)

	if not drivename:
		drivename = 'SSH'
	set_drive_name(drivename, user, host)

	# set net use info
	letter = drive.strip(':')
	remotepath = f'\\\\sshfs\\{user}@{host}\\..\\..'
	key = fr'HKCU\Network\{letter}'	
	util.run(f'reg add {key} /v RemotePath /d "{remotepath}" /f 2>nul')
	util.run(f'reg add {key} /v UserName /d "" /f 2>nul')
	util.run(f'reg add {key} /v ProviderName /d "Windows File System Proxy" /f 2>nul')
	util.run(f'reg add {key} /v ProviderType /d 20737046 /t REG_DWORD /f 2>nul')
	util.run(f'reg add {key} /v ConnectionType /d 1 /t REG_DWORD / 2>nul')
	util.run(f'reg add {key} /v ConnectFlags /d 0 /t REG_DWORD / 2>nul')

	# set drive icon
	import driveicon
	driveicon.main(letter)



if __name__ == '__main__':
	import sys
	import os
	assert len(sys.argv) > 2 and '@' in sys.argv[2] # usage: mount.py <drive> <user@host>
	drive = sys.argv[1]
	userhost = sys.argv[2]
	port=22
	if ':' in userhost:
		userhost, port = userhost.split(':')		
	user, host = userhost.split('@')
	sshfs = r'C:\Program Files\SSHFS-Win\bin\sshfs.exe'
	sys.path.insert(0, os.path.dirname(sshfs))

	logging.basicConfig(level=logging.INFO)

	main(sshfs, drive, user, host, port)