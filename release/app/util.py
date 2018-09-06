# util module for running commands and load config
import os
import re
import subprocess
import logging

logger = logging.getLogger('golddrive')

class ReturnBox():
	def __init__(self, out='', err=''):
		self.output = out
		self.error = err
		self.drive_state = 'DISCONNECTED'


def defaultKey(user):
	sshdir = os.path.expandvars("%USERPROFILE%")
	seckey = fr'{sshdir}\.ssh\id_rsa-{user}-golddrive'
	return seckey.replace(f'\\', '/')


def run(cmd, capture=False, shell=True, timeout=10):
	cmd = re.sub(r'[\n\r\t ]+',' ', cmd).replace('  ',' ').strip()
	header = 'CMD'
	if shell:
		header += ' (SHELL)'
	logger.info(f'{header}: {cmd}')
	try:
		r = subprocess.run(cmd, 
			capture_output=capture, 
			shell=shell, 
			timeout=timeout, 
			text=True)
	except Exception as ex:
		logger.error(ex)
		return subprocess.CompletedProcess()

	if r.returncode != 0:
		if r.stderr and r.stderr.startswith('Warning'):
			logger.warning(r)
		else:
			logger.error(r)		
	else:
		logger.info(r)
	if capture:
		r.stdout = r.stdout.strip()
		r.stderr = r.stderr.strip()
	return r
	

if __name__ == '__main__':

	logging.basicConfig(level=logging.INFO)
	run('tasklist')	
	run(fr'more "%USERPROFILE%\.ssh\id_rsa"')	
	run(fr'echo "hello world"')