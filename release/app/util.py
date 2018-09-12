# util module for running commands and load config
import os
import re
import subprocess
import logging
import yaml
import re
from enum import Enum

logger = logging.getLogger('golddrive')

class DriveStatus(Enum):
	CONNECTED = 1
	DISCONNECTED = 2
	IN_USE = 3
	BROKEN = 4
	NOT_SUPPORTED = 5
	NONE = -1

class WorkStatus(Enum):
	SUCCESS = 1
	FAILURE = 2
	INPUT_REQUIRED = 3
	NONE = -1

class Page(Enum):
	MAIN = 0
	LOGIN = 1
	ABOUT = 2

class ReturnBox():
	def __init__(self, out='', err=''):
		self.output = out
		self.error = err
		self.drive_status = None
		self.work_status = None
		self.object = None

def loadConfig(path):
	try:
		with open(path) as f:
			js = str(f.read())
			js = re.sub(r'(?m)^[\t ]*//.*\n?', '', js)
			js = js.replace('\t','')
			config = yaml.load(js)
			for key in config:
				value = config[key]
				if r'%' in value:
					config[key] = os.path.expandvars(value)
					# print(f'path expanded: {config[key]}')
			return config
	except Exception as ex:
		logger.error(f'Cannot read config file: {path}. Error: {ex}')
	return {}


def getAppKey(user):
	sshdir = os.path.expandvars("%USERPROFILE%")
	seckey = fr'{sshdir}\.ssh\id_rsa-{user}-golddrive'
	return seckey.replace(f'\\', '/')

def getUserKey():
	sshdir = os.path.expandvars("%USERPROFILE%")
	seckey = fr'{sshdir}\.ssh\id_rsa'
	return seckey.replace(f'\\', '/')

def richText(text):
	t = text.replace('\n','<br/>') #.replace('\'','\\\'')
	return f'<html><head/><body><p>{t}</p></body></html>'

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
		r = subprocess.CompletedProcess(cmd, 1)
		r.stdout = ''
		r.stderr = repr(ex)
		return r

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