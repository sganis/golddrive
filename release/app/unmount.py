# unmount drive
import util
import subprocess
import time

def is_running(program):
	cmd = f"tasklist /FI \"IMAGENAME eq {program}\" 2>NUL | find /I /N \"{program}\""
	o,e,r = util.run(cmd)
	return len(o) > 0

def kill(program):
	os.system(f'taskkill /im {program} /t /f 2>nul')
	while is_running(program):
		time.sleep(1)

def restart_explorer():
	# QTimer.singleShot(200, _restart_explorer) 
	import os
	os.system(fr'taskkill /im explorer.exe /f & start /b c:\windows\explorer.exe')

def main(drive):
	print(f'Unmounting {drive}...')
	kill('sshfs.exe')
	kill('ssh.exe')

	# cleanup
	entry = fr'HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\MountPoints2'
	mounts = []
	out, err, ret = util.run(f'reg query {entry}')
	for e in out.split('\n'):
		# print(e)
		m = e.split('\\')[-1]
		if m.startswith('##'):
			mounts.append(e)

	# append extra entries that needs to be deleted
	for letter in 'IJKLMNOPQRSTUVWXYZ':
		mounts.append(fr'HKCU\Network\{letter}')

	if mounts:
		for e in mounts:
			# print (e)
			try:
				cmd = f'reg delete "{e.strip()}" /f 2>nul'
				# print (cmd)
				sys.stdout.flush()
				util.run(cmd)
			except Exception as ex:
				# print(ex)
				pass		
	else:
		print('No mounts')


if __name__ == '__main__':
	import sys
	assert len(sys.argv) > 1 # usage: unmount.py <drive>
	main(sys.argv[1])