# mount drive using sshfs
# 
import sys
import os
import subprocess
import json
import time
import getpass
import util

DIR = os.path.dirname(os.path.realpath(__file__))
user = getpass.getuser()


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
		c['keygen'] 	= f'{path}\\ssh-keygen.exe'
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

def setup_ssh(ssh, keygen, userhost):
	import setupssh
	setupssh.main(ssh, keygen, userhost)

def unmount(drive):
	import unmount
	unmount.main(drive)

def set_drive_name(name, userhost):
	print(f'Setting drive name as {name}...')
	key = fr'HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\MountPoints2\##sshfs#{userhost}'
	cmd = f'reg add {key} /v _LabelFromReg /d {name} /f'
	util.run(cmd)

def mount(sshfs, drive, userhost):
	import mount
	mount.main(sshfs, drive, userhost)

def main():
	print('Mounting remote filesystem using SSHFS...')
	
	# read configuration
	c = get_config()
	# print(c)
	if not c:
		print('Cannot read configuration.')
		return 1

	# check if sshfs is installed, if not exit
	if not has_sshfs(c['sshfs']):
		print('SSHFS not installed.')
		return 2

	# test drive
	ok = check_drive(c['drive'])

	if not ok:
		sys.path.insert(0, os.path.dirname(c['sshfs']))
		unmount(c['drive'])
		setup_ssh(c['ssh'], c['keygen'], c['userhost'])
		set_drive_name(c['drivename'], c['userhost'])	
		mount(c['sshfs'], c['drive'], c['userhost'])

	return 0


if __name__ == '__main__':
	sys.exit(main())