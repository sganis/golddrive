# unmount drive
import subprocess
import logging

logger = logging.getLogger('ssh-drive')


def restart_explorer():
	subprocess.run(fr'taskkill /im explorer.exe /f & start /b c:\windows\explorer.exe')

def main(drive):
	print(f'Unmounting {drive}...')
	subprocess.run('taskkill /im sshfs.exe /f')
	subprocess.run('taskkill /im ssh.exe /f')

	# cleanup
	entry = fr'HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\MountPoints2'
	mounts = []
	response = subprocess.run(f'reg query {entry}', capture_output=True, text=True)
	for e in response.stdout.split('\n'):
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
				subprocess.run(cmd, shell=True)
			except Exception as ex:
				# print(ex)
				pass		
	else:
		print('No mounts')


if __name__ == '__main__':
	import sys
	assert len(sys.argv) > 1 # usage: unmount.py <drive>

	logging.basicConfig(level=logging.INFO)

	main(sys.argv[1])