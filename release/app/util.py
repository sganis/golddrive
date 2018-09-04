# util module for running commands and load config
import subprocess
import re
import os

class ReturnBox():
	def __init__(self, out='', err=''):
		self.output = out
		self.error = err
		self.drive_state = 'DISCONNECTED'


def defaultKey():
	sshdir = os.path.expandvars("%USERPROFILE%")
	seckey = fr'{sshdir}\.ssh\id_rsa.ssh-drive'
	return seckey.replace(f'\\', '/')


def run(cmd, capture=False, timeout=10):
	shell=False
	if '"' in cmd or '%' in cmd or '>' in cmd:
		shell=True
	cmd = re.sub(r'[\n\r\t ]+',' ', cmd)
	# print(f'CMD:\n{cmd}')

	r = subprocess.run(cmd, timeout, shell=shell, 
		capture_output=capture, text=True)
	# print(f'{r}')
	if capture:
		r.stdout = r.stdout.strip()
		r.stderr = r.stderr.strip()
		if r.stderr.strip():
			print(f'ERROR:\n{r}')
	return r
	

if __name__ == '__main__':
	run('tasklist')	
	run(fr'more "%USERPROFILE%\.ssh\id_rsa"')	
	run(fr'echo "hello world"')