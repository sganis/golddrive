# mount drive using sshfs
# 
import os
import util
import logging
import subprocess
import setupssh
import time
import winreg
import string
from ctypes import windll
	
logger = logging.getLogger('golddrive')

GOLDLETTERS = 'EGHIJKLMNOPQRSTUVWXYZ'

def drive_is_valid(drive):
	return (drive and len(drive)==2 and ':' in drive 
		and drive.split(':')[0] in GOLDLETTERS)

def reg_get(subkey):
	try:
		return winreg.OpenKeyEx(winreg.HKEY_CURRENT_USER, subkey, 0, winreg.KEY_ALL_ACCESS)
	except Exception as ex:
		logger.error(f'cannot read registry key {subkey}: {ex}')
		return None

def reg_add(subkey, name, valtype, value):
	try:
		reg_key = winreg.CreateKeyEx(winreg.HKEY_CURRENT_USER, subkey, 0)
		with reg_key:
			winreg.SetValueEx(reg_key, name, 0, valtype, value)
	except Exception as ex:
		logger.error(f'cannot add registry key {subkey}: {ex}')

def reg_del(subkey):
	try:
		winreg.DeleteKey(winreg.HKEY_CURRENT_USER, subkey)
		logger.info(fr'regkey HKCU\{subkey} deleted.')
	except Exception as ex:
		logger.error(f'cannot delete registry key {subkey}: {ex}')


def set_drive_name(name, userhost, client):
	logger.info(f'Setting drive name as {name}...')
	subkey = fr'Software\Microsoft\Windows\CurrentVersion\Explorer\MountPoints2\##{client}#{userhost}'
	reg_add(subkey, '_LabelFromReg', winreg.REG_SZ, name)


def set_drive_icon(letter, client):
	logger.info(f'Setting registry drive icon...')
	loc = os.path.dirname(os.path.realpath(__file__))
	if client == 'sanfs':
		ico = fr'{loc}\icon\golddrive_sanfs.ico'	
	else:
		ico = fr'{loc}\icon\golddrive.ico'	
	logger.info(f'Setting drive icon as {ico}...')	
	key = fr'Software\Classes\Applications\Explorer.exe\Drives\{letter}\DefaultIcon'
	reg_add(key, '', winreg.REG_SZ, ico)


def set_net_use(letter, userhost, client):
	logger.info(f'Setting registry net use info...')
	remotepath = f'\\\\{client}\\{userhost}\\..\\..'
	key = fr'Network\{letter}'	
	reg_add(key, 'RemotePath', winreg.REG_SZ, remotepath)
	reg_add(key, 'UserName', winreg.REG_SZ, '')
	reg_add(key, 'ProviderName', winreg.REG_SZ, "Windows File System Proxy")
	reg_add(key, 'ProviderType', winreg.REG_DWORD, 20737046)
	reg_add(key, 'ConnectionType', winreg.REG_DWORD, 1)
	reg_add(key, 'ConnectFlags', winreg.REG_DWORD, 0)

	# util.run(f'''reg add {key} /v RemotePath /d "{remotepath}" /f''', capture=True)
	# util.run(f'''reg add {key} /v UserName /d "" /f''', capture=True)
	# util.run(f'''reg add {key} /v ProviderName /d "Windows File System Proxy" /f''', capture=True)
	# util.run(f'''reg add {key} /v ProviderType /d 20737046 /t REG_DWORD /f''', capture=True)
	# util.run(f'''reg add {key} /v ConnectionType /d 1 /t REG_DWORD /f''', capture=True)
	# util.run(f'''reg add {key} /v ConnectFlags /d 0 /t REG_DWORD /f''', capture=True)

def mount(drive, userhost, appkey, client='sanfs', port=22, drivename='', args=''):

	logger.info(f'Mounting {drive} {userhost}...')
	rb = util.ReturnBox()

	user, host = userhost.split('@')

	letter = drive.strip(':')
	if not drivename:
		drivename = 'GOLDDRIVE'

	# result = setupssh.testssh(ssh, userhost, appkey, port)
	# if result == util.ReturnCode.BAD_LOGIN:
	# 	rb.error = 'SSH key authetication wrong'
	# 	rb.drive_status = 'KEYS_WRONG'
	# 	rb.returncode = result
	# 	return rb

	status = check_drive(drive, userhost, client)
	if not status == 'DISCONNECTED':
		rb.error = status
		rb.drive_status = status
		return rb

	cmd = ''
	if client == 'sanfs':
		cmd = f'''sanfs.exe {drive} 
			-o host={host}
			-o user={user}
			-o port={port}
			-o pkey={appkey}
			-o uid=-1,gid=-1,create_umask=007,mask=007 
			'''
	else:	
		cmd = f'''sshfs.exe {userhost}:/ {drive} 
			-o port={port}
			-o IdentityFile={appkey}
			-o VolumePrefix=/sshfs/{userhost}
			-o volname={userhost} 
			-o uid=-1,gid=-1,create_umask=007,mask=007 
			-o FileSystemName=SSHFS 
			-o PasswordAuthentication=no
			-o StrictHostKeyChecking=no	
			-o UserKnownHostsFile=/dev/null
			-o ServerAliveInterval=60 
			-o compression=no
			-o rellinks 
			-o reconnect 			
			'''
			# -o max_read=65536
			# -o max_write=65536
			# -o sshfs_sync
			# -f
			# -o ThreadCount=10
			# -o dir_cache=yes
			# -o dcache_timeout=5
			# -o dcache_clean_interval=300
			# -o KeepFileCache
			# -o FileInfoTimeout=-1
			# -o DirInfoTimeout=5000
			# -o VolumeInfoTimeout=5000
			# -o Ciphers=aes128-gcm@openssh.com
			# -o ServerAliveCountMax=10000
			# -o ssh_command='ssh -vv -d'
	
	# fuse_opts = ''' 
	# 		-o FileInfoTimeout=3000 
	# 		-o DirInfoTimeout=5000
	# 		-o VolumeInfoTimeout=10000
	# 		'''
	cmd = cmd.replace('\n',' ').replace('\r','').replace('\t','') 
	cmd += ' ' + args
	
	# logger.info(cmd)
	if client == 'sanfs':
		util.run(cmd, detach=True)
	else:
		proc = subprocess.Popen(cmd, shell=True, stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL)
		time.sleep(1)
	

	
	set_drive_name(drivename, userhost, client)
	set_net_use(letter, userhost, client)
	set_drive_icon(letter, client)
	rb.drive_status = 'CONNECTED'
	rb.returncode = util.ReturnCode.OK
	return rb

def clean_drive(drive, client):
	# cleanup registry
	user = ''
	host = ''
	letter = drive.split(':')[0].upper()
	regkey = fr'Network\{letter}'
	reg_key = reg_get(regkey)
	if not reg_key:
		return
	try:
		i = 0
		while 1:
			name, value, type = winreg.EnumValue(reg_key, i)
			if 'RemotePath' in name:
				user, host = value.split('\\')[3].split('@')
			i += 1
	except:
		pass
	reg_del(regkey)

	if not user:
		return

	regkey = fr'Software\Microsoft\Windows\CurrentVersion\Explorer\MountPoints2'
	reg_key = reg_get(regkey)
	if not reg_key:
		return
	keys = []
	try:
		i = 0
		while 1:
			asubkey = winreg.EnumKey(reg_key, i)
			if asubkey.startswith(f'##{client}#{user}@{host}'):
				keys.append(f'{regkey}\\{asubkey}')
			i += 1
	except:
		pass

	for k in keys:
		reg_del(k)

	# clean icon
	reg_del(fr'Software\Classes\Applications\Explorer.exe\Drives\{letter}\DefaultIcon')
	reg_del(fr'Software\Classes\Applications\Explorer.exe\Drives\{letter}')


def unmount(drive, client):

	logger.info(f'Unmounting {drive}...')
	rb = util.ReturnBox()

	if not drive_is_valid(drive):
		rb.returncode = util.ReturnCode.BAD_DRIVE
		return rb

	util.kill_drive(drive, client)
	clean_drive(drive, client)

	rb.drive_status = 'DISCONNECTED'
	rb.returncode = util.ReturnCode.OK
	return rb

def unmount_all():
	for letter in GOLDLETTERS:
		drive = f'{letter}:'
		unmount(drive)

	rb = util.ReturnBox()
	rb.returncode = util.ReturnCode.OK
	return rb

def drive_works(drive, userhost):
	'''check if drive is working'''
	tempfile = f'{drive}\\tmp\\{userhost}.{time.time()}'
	r = util.run(fr'type nul > {tempfile}', 10)
	if r.returncode == 0:
		util.run(fr'del {tempfile}', 10)
		return True
	return False

def get_free_drives():
	used = []
	bitmask = windll.kernel32.GetLogicalDrives()
	for letter in string.ascii_uppercase:
		if bitmask & 1:
			used.append(letter)
		bitmask >>= 1
	# cmd = ('wmic logicaldisk get name')
	# r = util.run(cmd, capture=True)
	# for line in r.stdout.split('\n'):
	# 	if ':' in line:
	# 		used.append(line.split(':')[0])
	return [f'{d}:' for d in GOLDLETTERS if d not in used]

def check_drive(drive, userhost, client):
	logger.info(f'Checking drive {drive} in {userhost}...')
	if not (drive and len(drive)==2 and drive.split(':')[0].upper() in GOLDLETTERS):
		return 'NOT SUPPORTED'
	r = util.run(f'net use', capture=True)
	# print(r.stdout)
	# cmd = (f'wmic logicaldisk where "providername like \'%{userhost}%\' '
	# 	   f'or caption=\'{drive}\'" get caption,providername')
	# r = util.run(cmd, shell=True, capture=True)
	# print(f'drive must be here: \'{ r.stdout }\'')
	free_drives = get_free_drives()
	# print(free_drives)
	in_use = drive not in free_drives
	# print(in_use)
	is_golddrive = False
	host_use = False
	for line in r.stdout.split('\n'):
		if fr'\\{client}\{userhost}' in line:
			if drive in line:
				is_golddrive = True
			else:
				host_use = True
			break
	if host_use:
		return 'HOST IN USE'
	elif not in_use:						
		return 'DISCONNECTED'
	elif not is_golddrive:	
		return 'IN USE'
	elif not drive_works(drive, userhost):				
		return 'BROKEN'
	else:
		return 'CONNECTED'




if __name__ == '__main__':

	import sys
	import os
	assert len(sys.argv) > 2 # usage: mount.py <drive> [<user@host>|-d]
	drive = sys.argv[1].upper()
	assert (len(drive)==2 
		and drive[-1]==':'
		and drive[0] in GOLDLETTERS) # invalid drive
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
		mount(drive, userhost, '', port)
