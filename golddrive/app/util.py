# util module for running commands and load config
import os
import re
import subprocess
import logging
import yaml
import re
import getpass
from enum import Enum

logger = logging.getLogger('golddrive')

IPADDR = r'(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])'
HOSTNAME = r'(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\-]*[a-zA-Z0-9])\.)*([A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9\-]*[A-Za-z0-9])'
PATTERN_HOST = fr'^{HOSTNAME}|{IPADDR}$'
REGEX_HOST = re.compile(PATTERN_HOST)
CURRENT_USER = getpass.getuser()

class DriveStatus(Enum):
	CONNECTED = 1
	DISCONNECTED = 2
	IN_USE = 3
	BROKEN = 4
	NOT_SUPPORTED = 5
	NONE = -1

class Page(Enum):
	# match the stackwidget page index
	MAIN 	 = 0
	HOST 	 = 1
	LOGIN 	 = 2
	ABOUT 	 = 3

class ReturnCode(Enum):
	OK 			= 0
	BAD_DRIVE 	= 1
	BAD_HOST 	= 2
	BAD_LOGIN 	= 3
	BAD_SSH 	= 4
	BAD_MOUNT	= 5
	NONE 		= -1

class ReturnBox():
	def __init__(self, out='', err=''):
		self.output = out
		self.error = err
		self.drive_status = None
		self.returncode = ReturnCode.NONE
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
			config['ssh'] = fr"{config.get('sshfs_path','')}\ssh.exe"
			config['sshfs'] = fr"{config.get('sshfs_path','')}\sshfs.exe"

			if not os.path.exists(config['ssh']):
				logger.error('ssh not found')
			if not os.path.exists(config['sshfs']):
				logger.error('sshfs not found')
			return config

	except Exception as ex:
		logger.error(f'Cannot read config file: {path}. Error: {ex}')
	return {}

def addDriveConfig(**p):
	'''Add drive to config.json
	'''
	logger.info('Adding drive to config file...')
	drive = p['drive']
	user, host = p['userhost'].split('@')
	port = p['port']
	drivename =	p['drivename']
	configfile = p['configfile']

	text = ('	"drives" : {\n'
			f'		"{drive}" : {{\n'
			f'			"drivename" : "{drivename}",\n'
			f'			"user" 		: "{user}",\n'
			f'			"port" 		: "{port}",\n'
			f'			"hosts" 	: ["{host}"],\n'
			'		},\n'
			'	}\n')
	try:
		with open(configfile) as r:
			lines = r.readlines()
		with open(configfile, 'w') as w:
			for line in lines:
				if r'"drives" : {}' in line:
					line = text
				w.write(line)
	except Exception as ex:
		logger.error(str(ex))

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

def makeHyperlink(href, text):
	return f"<a href='{href}'><span style=\"text-decoration: none; color:#0E639C;\">{text}</span></a>"

def getUserHostPort(text):
	userhostport = text
	userhost = text
	host = text
	port = None
	user = CURRENT_USER
	if ':' in text:
		userhost, port = text.split(':')
		host = userhost
	if '@' in userhost:
		user, host = userhost.split('@')
	if not REGEX_HOST.match(host):
		host = '<invalid>'
	if not user:
		user = '<invalid>'
	if port:
		try:
			port = int(port)
			if port < 0 or port > 65635:
				raise
		except:
			port = '<invalid>'
	else:
		port = 22
	return user, host, port

def run(cmd, capture=False, shell=True, timeout=30):
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
	# else:
	# 	logger.info(r)
	if capture:
		r.stdout = r.stdout.strip()
		r.stderr = r.stderr.strip()
	return r
	
def getVersions():
	ssh = ''
	sshfs = ''
	cygwin = ''
	winfsp = ''
	
	r = run(f'ssh -V', capture=True)
	if r.returncode == 0:
		# m = re.match(r'(OpenSSH[^,]+),[\s]*(OpenSSL[\s]?[\w.]+)[\s]+([\w\s]+)', r.stderr)
		# if m:
		# 	ssh = f'{ m.group(1) }\n{ m.group(2) } ({ m.group(3) })'
		for c in r.stderr.split(','):
			if c.strip():
				ssh += c.strip().replace('  ',' ') + '\n'
		ssh = ssh.strip()
		
	r = run(f'sshfs -V', capture=True)
	if r.returncode == 0:
		out = r.stdout.replace('version ','')
		sshfs = fr"{out}"
	

	sshfs_path = os.path.dirname(run(f'where sshfs', capture=True).stdout)
	cmd =f"wmic datafile where name='{ sshfs_path }\\cygwin1.dll' get version /format:list"
	r = run(cmd.replace('\\','\\\\'), capture=True)
	if r.returncode == 0 and '=' in r.stdout:
		ver = r.stdout.split("=")[-1]
		cygwin = f'Cygwin {ver}'


	p86 = os.path.expandvars('%ProgramFiles(x86)%')
	cmd =f"wmic datafile where name='{p86}\\WinFsp\\bin\\winfsp-x64.dll' get version /format:list"
	r = run(cmd.replace('\\','\\\\'), capture=True)
	if r.returncode == 0 and '=' in r.stdout:
		ver = r.stdout.split("=")[-1]
		winfsp = f'WINFSP {ver}'

	result = f'{ssh}\n{sshfs}\n{cygwin}\n{winfsp}'
	return result


if __name__ == '__main__':

	logging.basicConfig(level=logging.INFO)
	run('tasklist')	
	run(fr'more "%USERPROFILE%\.ssh\id_rsa"')	
	run(fr'echo "hello world"')