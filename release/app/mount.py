# mount drive using sshfs
# 
import util
import logging
import subprocess

logger = logging.getLogger('ssh-drive')

def set_drive_name(name, userhost):
	logger.info(f'Setting drive name as {name}...')
	key = fr'HKCU\Software\Microsoft\Windows\CurrentVersion'
	key = fr'{key}\Explorer\MountPoints2\##sshfs#{userhost}'
	cmd = f'reg add {key} /v _LabelFromReg /d {name} /f >nul 2>&1'
	util.run(cmd)


def set_drive_icon(letter):
	
	loc = os.path.dirname(os.path.realpath(__file__))
	ico = fr'{loc}\images\ssh-drive.ico'
	
	logger.info(f'Setting drive icon as {ico}...')
	
	explorer = fr'HKCU\Software\Classes\Applications\Explorer.exe'
	keys = [ 
		explorer,	
		fr'{explorer}\Drives',
		fr'{explorer}\Drives\{letter}']

	for key in keys:
		util.run(f'reg add {key} /ve /f >nul 2>&1')

	key = fr'{explorer}\Drives\{letter}\DefaultIcon'

	util.run(f'reg add {key} /ve /d "{ico}" /f >nul 2>&1')

def set_net_use(letter, userhost):

	logger.info(f'Setting net use info...')
	remotepath = f'\\\\sshfs\\{userhost}\\..\\..'
	key = fr'HKCU\Network\{letter}'	
	util.run(f'''reg add {key} /v RemotePath
					/d "{remotepath}" /f >nul 2>&1''')
	util.run(f'''reg add {key} /v UserName /d "" /f >nul 2>&1''')
	util.run(f'''reg add {key} /v ProviderName
					/d "Windows File System Proxy" /f >nul 2>&1''')
	util.run(f'''reg add {key} /v ProviderType
					/d 20737046 /t REG_DWORD /f >nul 2>&1''')
	util.run(f'''reg add {key} /v ConnectionType 
					/d 1 /t REG_DWORD / >nul 2>&1''')
	util.run(f'''reg add {key} /v ConnectFlags 
					/d 0 /t REG_DWORD / >nul 2>&1''')


def main(sshfs, drive, userhost, seckey, port=22, drivename=''):

	logger.info(f'Mounting {drive} {userhost}...')
	rb = util.ReturnBox()

	letter = drive.strip(':')
	if not seckey:
		seckey = util.defaultKey()
	if not drivename:
		drivename = 'SSH'

	cmd = f'''
		"{sshfs}" {userhost}:/ {drive} 
		-o IdentityFile={seckey}
		-o port={port}
		-o VolumePrefix=/sshfs/{userhost}
		-o volname={drivename}-{userhost} 
		-o uid=-1,gid=-1,create_umask=0007 
		-o rellinks -o reconnect
		-o FileSystemName=SSHFS 
		-o StrictHostKeyChecking=no
		-o ServerAliveInterval=10 
		-o ServerAliveCountMax=10000
		-o FileInfoTimeout=10000 
		-o DirInfoTimeout=10000 
		-o VolumeInfoTimeout=10000
		-o max_readahead=131072
		'''
		# google mount: "-o" "max_readahead=131072"
	r = util.run(cmd, capture=True)
	if r.stderr:
		logger.error(r.stderr)
		rb.error = r.stderr
		if 'mount point in use' in rb.error:
			rb.error = 'Drive in use'
		return rb
	
	set_drive_name(drivename, userhost)
	set_net_use(letter, userhost)
	set_drive_icon(letter)
	return rb

if __name__ == '__main__':
	import sys
	import os
	assert len(sys.argv) > 2 and '@' in sys.argv[2] # usage: mount.py <drive> <user@host>
	drive = sys.argv[1]
	userhost = sys.argv[2]
	port=22
	if ':' in userhost:
		userhost, port = userhost.split(':')		
	sshfs = r'C:\Program Files\SSHFS-Win\bin\sshfs.exe'
	sys.path.insert(0, os.path.dirname(sshfs))

	logging.basicConfig(level=logging.INFO)

	main(sshfs, drive, userhost, '', port)