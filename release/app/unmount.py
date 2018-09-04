# unmount drive
import util
import logging
import subprocess

logger = logging.getLogger('ssh-drive')


def restart_explorer():
	util.run(fr'taskkill /im explorer.exe /f 2>nul')
	subprocess.run(fr'start /b c:\windows\explorer.exe', shell=True)

def main(drive):
	print(f'Unmounting {drive}...')
	util.run('taskkill /im sshfs.exe /f 2>nul')
	util.run('taskkill /im ssh.exe /f 2>nul')

	# cleanup
	entry = fr'HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\MountPoints2'
	mounts = []
	out, err, ret = util.run(f'reg query {entry}', output=True)
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
	# restart_explorer()

	import sys
	assert len(sys.argv) > 1 # usage: unmount.py <drive>

	logging.basicConfig(level=logging.INFO)

	main(sys.argv[1])