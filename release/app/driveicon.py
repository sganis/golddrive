# set drive icon
import os
import subprocess


def main(letter):
	
	keys = [
		fr'HKCU\Software\Classes\Applications\Explorer.exe',
		fr'HKCU\Software\Classes\Applications\Explorer.exe\Drives',
		fr'HKCU\Software\Classes\Applications\Explorer.exe\Drives\{letter}',
	]

	for key in keys:
		subprocess.run(f'reg add {key} /ve /f'.split())

	key = fr'HKCU\Software\Classes\Applications\Explorer.exe\Drives\{letter}\DefaultIcon'
	ico = fr'{os.path.dirname(os.path.realpath(__file__))}\images\ssh-drive.ico'
	subprocess.run(f'reg add {key} /ve /d "{ico}" /f', shell=True)


if __name__ == '__main__':
	main('Z')