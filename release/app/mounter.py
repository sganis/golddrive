# mount drive using sshfs
# 
import os
import util
import logging
import subprocess
import setupssh

logger = logging.getLogger('golddrive')


def get_host():
	'''get hostname to ssh'''
	# if service == 'simulation':
	# 	return 'localhost'
	return 'localhost'

def get_config():
	try:
		c = json.loads(open(DIR + '\\config.json').read())
		path 			= c['sshfs_path']
		c['sshfs']  	= f'{path}\\sshfs.exe'
		c['ssh']  		= f'{path}\\ssh.exe'
		c['host'] 		= get_host()
		c['userhost'] 	= f"{user}@{c['host']}"
		return c
	except Exception as ex:
		print(ex)
		return {}

def has_sshfs(sshfs):
	return os.path.exists(sshfs)

def get_sshfs_version(ssh, sshfs):
	util.run(f'{sshfs} -v')
	util.run(f'{ssh} -v')

def check_drive(drive):
	'''check if drive is working'''
	try:
		now = time.time()
		tempfile = f'{drive}\\tmp\\{user}.{now}'
		# print(f'temp file: {tempfile}')
		with open(tempfile, 'w') as w: w.write('test')
		if os.path.exists(tempfile):
			os.remove(tempfile)
		print(f'Drive {drive} is OK')
		return True
	except Exception as ex:
		# print(ex)
		return False

def set_drive_name(name, userhost):
	logger.info(f'Setting drive name as {name}...')
	key = fr'HKCU\Software\Microsoft\Windows\CurrentVersion'
	key = fr'{key}\Explorer\MountPoints2\##sshfs#{userhost}'
	cmd = f'reg add {key} /v _LabelFromReg /d {name} /f >nul 2>&1'
	util.run(cmd)


def set_drive_icon(letter):
	
	loc = os.path.dirname(os.path.realpath(__file__))
	ico = fr'{loc}\assets\golddrive.ico'
	
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

def restart_explorer():
	util.run(fr'taskkill /im explorer.exe /f >nul 2>&1')
	util.run(fr'start /b c:\windows\explorer.exe')

def mount(sshfs, ssh, drive, userhost, seckey, port=22, drivename=''):

	logger.info(f'Mounting {drive} {userhost}...')
	rb = util.ReturnBox()

	letter = drive.strip(':')
	if not seckey:
		seckey = util.defaultKey()
	if not drivename:
		drivename = 'SSH'

	ssh_ok = setupssh.testssh(ssh, userhost, seckey, port)
	if not ssh_ok:
		rb.error = 'SSH key authetication wrong'
		return rb

	cmd = f'''
		"{sshfs}" {userhost}:/ {drive} 
		-o IdentityFile={seckey}
		-o port={port}
		-o VolumePrefix=/sshfs/{userhost}
		-o volname={drivename}-{userhost} 
		-o idmap=user,create_umask=007,mask=007 
		-o rellinks -o reconnect
		-o FileSystemName=SSHFS 
		-o StrictHostKeyChecking=no
		-o UserKnownHostsFile=/dev/null
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

def get_process_id(drive):
	wmic = fr'c:\windows\system32\wbem\wmic.exe'
	cmd = f"""{wmic} process where (commandline like '% {drive} %' 
		and name='sshfs.exe') get processid"""
	r = util.run(cmd, capture=True)
	if r.returncode == 0 and r.stdout:
		pid = r.stdout.split('\n')[-1]
		return pid
	else:
		return '0'

def unmount(drive):
	logger.info(f'Unmounting {drive}...')
	rb = util.ReturnBox()
	pid = get_process_id(drive)
	if pid != '0':
		util.run(f'taskkill /pid {pid} /t /f >nul 2>&1')
	
	# cleanup
	letter = drive.split(':')[0].upper()
	regkey = fr'HKCU\Network\{letter}'
	r = util.run(f'reg query "{regkey}"', capture=True)
	user = ''
	host = ''
	for line in r.stdout.split('\n'):
		# print(line)
		if 'RemotePath' in line:
			fields = line.strip().split()
			if len(fields) > 2:
				remotepath = fields[2].split('\\')
				if len(remotepath) > 3:
					user, host = remotepath[3].split('@')

	util.run(f'reg delete "{regkey}" /f >nul 2>&1', )

	regkey = fr'HKCU\Software\Microsoft\Windows'
	regkey = fr'{regkey}\CurrentVersion\Explorer\MountPoints2'
	mounts = []
	r = util.run(f'reg query {regkey}', capture=True)
	for e in r.stdout.split('\n'):
		m = e.split('\\')[-1]
		if m.startswith(f'##sshfs#{user}@{host}'):
			util.run(f'reg delete "{e.strip()}" /f')

	rb.output = 'DISCONNECTED'
	return rb


# def remount(sshfs, drive, userhost):
# 	print('Mounting remote filesystem using SSHFS...')
	
# 	# read configuration
# 	c = get_config()
# 	# print(c)
# 	if not c:
# 		print('Cannot read configuration.')
# 		return 1

# 	# check if sshfs is installed, if not exit
# 	if not has_sshfs(c['sshfs']):
# 		print('SSHFS not installed.')
# 		return 2

# 	# test drive
# 	ok = check_drive(c['drive'])

# 	if not ok:
# 		sys.path.insert(0, os.path.dirname(c['sshfs']))
# 		unmount(c['drive'])
# 		setup_ssh(c['ssh'], c['keygen'], c['userhost'])
# 		mount(c['sshfs'], c['drive'], c['userhost'], c['drivename'])

# 	return 0


if __name__ == '__main__':

	import sys
	import os
	assert len(sys.argv) > 2 # usage: mount.py <drive> [<user@host>|-d]
	drive = sys.argv[1].upper()
	assert (len(drive)==2 
		and drive[-1]==':'
		and drive[0] in 'EFGHIJKLMNOPQRSTUVWXYZ') # invalid drive
	assert ('@' in sys.argv[2] 
		or sys.argv[2]=='-d') # wrong argument, try user@host or -d

	logging.basicConfig(level=logging.INFO)
	
	ssh_path = fr'C:\Program Files\SSHFS-Win\bin'
	sshfs = fr'{ssh_path}\sshfs.exe'
	ssh = fr'{ssh_path}\ssh.exe'
	path = os.environ['PATH']
	os.environ['PATH'] = fr'{ssh_path};{path}'
	
	
	if sys.argv[2]=='-d':
		unmount(drive)
	else:		
		userhost = sys.argv[2]
		port=22
		if ':' in userhost:
			userhost, port = userhost.split(':')		
		mount(sshfs, ssh, drive, userhost, '', port)
