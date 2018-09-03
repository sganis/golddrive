# mount drive using sshfs
# 
import util

def set_drive_name(name, userhost):
	print(f'Setting drive name as {name}...')
	key = fr'HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\MountPoints2\##sshfs#{userhost}'
	cmd = f'reg add {key} /v _LabelFromReg /d {name} /f'
	util.run(cmd)

def main(sshfs, drive, userhost, drivename=''):
	print(f'Mounting {drive} {userhost}...')
	
	cmd = f'''
		"{sshfs}" {userhost}:/ {drive} 
		-orellinks -oreconnect
		-ouid=-1,gid=-1,create_umask=0007 
		-oVolumePrefix=/sshfs/{userhost}
		-ovolname=LINUX-{userhost} 
		-oFileSystemName=SSHFS 
		-oStrictHostKeyChecking=no
		-oServerAliveInterval=10 
		-oServerAliveCountMax=10000
		-oFileInfoTimeout=10000 
		-oDirInfoTimeout=10000 
		-oVolumeInfoTimeout=10000
		'''.replace('\n',' ').replace('\t','')

		# google mount: "-o" "max_readahead=131072"
		
	# print(cmd)
	util.run(cmd)

	if not drivename:
		drivename = 'Cluster'
	set_drive_name(drivename, userhost)

	# set net use info
	letter = drive.strip(':')
	remotepath = f'\\\\sshfs\\{userhost}\\..\\..'
	key = fr'HKCU\Network\{letter}'
	util.run(f'reg add {key} /v RemotePath /d "{remotepath}" /f')
	util.run(f'reg add {key} /v UserName /d "" /f')
	util.run(f'reg add {key} /v ProviderName /d "Windows File System Proxy" /f')
	util.run(f'reg add {key} /v ProviderType /d 20737046 /t REG_DWORD /f')
	util.run(f'reg add {key} /v ConnectionType /d 1 /t REG_DWORD /f')
	util.run(f'reg add {key} /v ConnectFlags /d 0 /t REG_DWORD /f')

	# set drive icon
	import driveicon
	driveicon.main(letter)



if __name__ == '__main__':
	import sys
	import os
	assert len(sys.argv) > 2 and '@' in sys.argv[2] # usage: mount.py <drive> <user@host>
	sshfs = r'C:\Program Files\SSHFS-Win\bin\sshfs.exe'
	sys.path.insert(0, os.path.dirname(sshfs))
	main(sshfs, *sys.argv[1:])