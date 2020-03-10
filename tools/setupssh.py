# setup ssh keys

import os
import paramiko
import logging
import subprocess
import re
import time
from enum import Enum

logger = logging.getLogger('golddrive')
logging.getLogger("paramiko.transport").setLevel(logging.WARNING)

DIR = os.path.dirname(os.path.realpath(__file__))

class ReturnBox():
	def __init__(self, out='', err=''):
		self.output = out
		self.error = err
		self.drive_status = None
		self.returncode = ReturnCode.NONE
		self.object = None

class ReturnCode(Enum):
	OK 			= 0
	BAD_DRIVE 	= 1
	BAD_HOST 	= 2
	BAD_LOGIN 	= 3
	BAD_SSH 	= 4
	BAD_MOUNT	= 5
	BAD_WINFSP	= 6
	NONE 		= -1

def run(cmd, capture=False, detach=False, shell=True, timeout=30):
	cmd = re.sub(r'[\n\r\t ]+',' ', cmd).replace('  ',' ').strip()
	header = 'CMD'
	if shell:
		header += ' (SHELL)'
	logger.debug(f'{header}: {cmd}')

	r = subprocess.CompletedProcess(cmd, 0)
	r.stdout = ''
	r.stderr = ''

	try:
		if detach:
			CREATE_NEW_PROCESS_GROUP = 0x00000200
			DETACHED_PROCESS = 0x00000008
			# p = subprocess.Popen(shlex.split(cmd), 
			# 		stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
			# 		creationflags=DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP)
			p = subprocess.Popen(shlex.split(cmd), close_fds=True,
					creationflags=DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP)
			logger.debug(f'Detached process {p.pid} started.')
		else:
			r = subprocess.run(cmd, capture_output=capture, shell=shell, 
					timeout=timeout, text=True)
	except Exception as ex:
		r.stderr = repr(ex)
		logger.error(r)
		return r

	if r.returncode != 0:
		if r.stderr and r.stderr.startswith('Warning'):
			logger.warning(r)
		else:
			logger.error(r)		
	# else:
	# 	logger.debug(r)
	if capture:
		r.stdout = r.stdout.strip()
		r.stderr = r.stderr.strip()
	return r
	
def get_app_key():

	return f'{ os.path.expandvars("%USERPROFILE%") }\\.ssh\\id_rsa'
	
def testhost(userhost, port=22):
	'''
	Test if host respond to port
	Return: True or False
	'''
	logger.debug(f'Testing port {port} at {userhost}...')
	user, host = userhost.split('@')
	client = paramiko.SSHClient()
	client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
	rb = ReturnBox()
	try:
		client.connect(hostname=host, username=user, 
				password='', port=port, timeout=5)	
		rb.returncode = ReturnCode.OK
	except (paramiko.ssh_exception.AuthenticationException,
		paramiko.ssh_exception.BadAuthenticationType,
		paramiko.ssh_exception.PasswordRequiredException):
		rb.returncode = ReturnCode.OK
	except Exception as ex:
		rb.returncode = ReturnCode.BAD_HOST
		rb.error = str(ex)
	finally:
		client.close()
	return rb

def testlogin(userhost, password, port=22):
	'''
	Test ssh password authentication
	'''
	logger.debug(f'Logging in with password for {userhost}...')
	rb = ReturnBox()
	if not password:
		rb.returncode =ReturnCode.BAD_LOGIN
		rb.error = 'Empty password'
		return rb

	user, host = userhost.split('@')
	client = paramiko.SSHClient()
	client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
	
	try:
		client.connect(hostname=host, username=user, 
				password=password, port=port, timeout=10, 
				look_for_keys=False)
		rb.returncode = ReturnCode.OK
	except (paramiko.ssh_exception.AuthenticationException,
		paramiko.ssh_exception.BadAuthenticationType,
		paramiko.ssh_exception.PasswordRequiredException) as ex:
		rb.returncode = ReturnCode.BAD_LOGIN
		rb.error = str(ex)
	except Exception as ex:
		rb.returncode = ReturnCode.BAD_HOST
		rb.error = str(ex)
	finally:
		client.close()
	return rb

def testssh(userhost, port=22):
	'''
	Test ssh key authentication
	'''
	seckey = get_app_key()
	logger.debug(f'Testing ssh keys for {userhost} using key {seckey}...')
	
	rb = testhost(userhost, port)
	if rb.returncode == ReturnCode.BAD_HOST:
		return rb
	
	if not os.path.exists(seckey):
		seckey_win = seckey.replace('/','\\')
		logger.error(f'Key does not exist: {seckey_win}')
		rb.returncode = ReturnCode.BAD_LOGIN
		rb.error = "No key"
		return rb

	cmd = f'''ssh.exe
		-i "{seckey}"
		-p {port} 
		-o PasswordAuthentication=no
		-o StrictHostKeyChecking=no 
		-o UserKnownHostsFile=/dev/null
		-o BatchMode=yes 
		{userhost} "echo ok"'''
	r = run(cmd, capture=True, timeout=10)
	
	if r.stdout == 'ok':
		# success
		rb.returncode = ReturnCode.OK
	elif 'Permission denied' in r.stderr:
		# wrong user or key issue
		rb.returncode = ReturnCode.BAD_LOGIN
		rb.error = 'Access denied'
	else:
		# wrong port: connection refused 
		# unknown host: connection timeout
		logger.error(r.stderr)
		rb.returncode = ReturnCode.BAD_HOST
		rb.error = r.stderr
	return rb

def generate_keys(userhost):
	logger.debug('Generating new ssh keys...')
	seckey = get_app_key()
	pubkey = f'{seckey}.pub'
	now = time.time()
	if os.path.exists(seckey):
		os.rename(seckey, f'{seckey}.{ now }.bak')
	if os.path.exists(pubkey):
		os.rename(pubkey, f'{pubkey}.{ now }.bak')
	rb = ReturnBox()
	sk = paramiko.RSAKey.generate(2048)
	try:
		sshdir = os.path.dirname(seckey)
		if not os.path.exists(sshdir):
			os.makedirs(sshdir)
			os.chmod(sshdir, 0o700)
		sk.write_private_key_file(seckey)	
	except Exception as ex:
		logger.error(f'{ex}, {seckey}')
		rb.error = str(ex)
		return rb	
	
	pubkey = f'ssh-rsa {sk.get_base64()} {userhost}'
	
	try:
		with open(seckey + '.pub', 'wt') as w:
			w.write(pubkey)
	except Exception as ex:
		logger.error(f'Could not save public key: {ex}')

	rb.output = pubkey
	return rb

def has_app_keys():
	seckey = get_app_key()
	pubkey = f'{seckey}.pub'
	output = ''
	if os.path.exists(seckey):
		try:
			output = run(fr'ssh-keygen -y -f {seckey}', capture=True).stdout
			if os.path.exists(pubkey):
				with open(pubkey) as r:
					output2 = r.read()
				if output.split()[1] != output2.split()[1]:
					logger.error('public key invalid!!!!!!!!!!!!!')
					logger.error(output)
					logger.error(output2)
					output = ''
		except Exception as ex:
			logger.error(ex)
	return output and output.startswith('ssh-rsa')

def set_key_permissions(user):
	
	logger.debug('setting ssh key permissions...')
	seckey = get_app_key()
	ssh_folder = os.path.dirname(seckey)
	# Remove Inheritance ::
	# subprocess.run(fr'icacls {ssh_folder} /c /t /inheritance:d')
	run(fr'icacls {seckey} /c /t /inheritance:d', capture=True)
	
	# Set Ownership to Owner and SYSTEM account
	# subprocess.run(fr'icacls {ssh_folder} /c /t /grant %username%:F')
	run(fr'icacls {seckey} /c /t /grant { os.environ["USERNAME"] }:F', capture=True)
	run(fr'icacls {seckey} /c /t /grant SYSTEM:F', capture=True)
	
	# Remove All Users, except for Owner 
	# subprocess.run(fr'icacls {ssh_folder} /c /t /remove Administrator BUILTIN\Administrators BUILTIN Everyone System Users')
	run(fr'icacls {seckey} /c /t /remove Administrator BUILTIN\Administrators BUILTIN Everyone Users', capture=True)
	
	# Verify 
	# run(fr'icacls {seckey}')
	
def main(userhost, password, port=22):
	'''
	Setup ssh keys, return ReturnBox
	'''
	logger.debug(f'Setting up ssh keys for {userhost}...')
	rb = ReturnBox()

	# app key
	user, host = userhost.split('@')
	seckey = get_app_key()	

	# Check if keys need to be generated
	pubkey = ''
	if has_app_keys():
		logger.debug('Private key already exists.')
		sk = paramiko.RSAKey.from_private_key_file(seckey)
		pubkey = f'ssh-rsa {sk.get_base64()} {userhost}'
	else:
		rbkey = generate_keys(userhost)
		if rbkey.error:
			rbkey.returncode = ReturnCode.BAD_SSH
			return rbkey
		else:
			pubkey = rbkey.output

	# connect
	client = paramiko.SSHClient()
	client.set_missing_host_key_policy(paramiko.AutoAddPolicy())

	rb.error = ''
	try:
		logger.debug('Connecting using password...')
		client.connect(hostname=host, username=user,
						password=password, port=port, timeout=10,
						look_for_keys=False)     
	except paramiko.ssh_exception.AuthenticationException:
		rb.error = f'User or password wrong'
		rb.returncode = 1
	except Exception as ex:
		rb.error = f'connection error: {ex}'
		rb.returncode = 2

	if rb.error:
		logger.error(rb.error)
		if 'getaddrinfo failed' in rb.error:
			rb.error = f'{host} not found'
		client.close()
		rb.returncode = ReturnCode.BAD_SSH
		return rb

	set_key_permissions(user)

	logger.debug(f'Publising public key...')
		
	# Copy to the target machines.
	# cmd = f"exec bash -c \"cd; umask 077; mkdir -p .ssh && echo '{pubkey}' >> .ssh/authorized_keys || exit 1\" || exit 1"
	cmd = f"exec sh -c \"cd; umask 077; mkdir -p .ssh; echo '{pubkey}' >> .ssh/authorized_keys\""
	logger.debug(cmd)
	ok = False
	
	try:
		stdin, stdout, stderr = client.exec_command(cmd, timeout=10)
		rc = stdout.channel.recv_exit_status()   
		if rc == 0:
			logger.debug('Key transfer successful')
			rb.returncode = ReturnCode.OK
		else:
			logger.error(f'Error transfering public key: exit {rc}, error: {stderr}')
	except Exception as ex:
		logger.error(ex)
		rb.returncode = ReturnCode.BAD_SSH
		rb.error = f'error transfering public key: {ex}'
		return rb
	finally:
		client.close()

	err = stderr.read()
	if err:
		logger.error(err)
		rb.returncode = ReturnCode.BAD_SSH
		rb.error = f'error transfering public key, error: {err}'
		return rb
	
	rb = testssh(userhost, port)
	if rb.returncode == ReturnCode.OK:
		rb.output = "SSH setup successfull." 
		logger.info(rb.output)
	else:
		message = 'SSH setup test failed'
		detail = ''
		if rb.returncode == ReturnCode.BAD_LOGIN:
			detail = ': authentication probem'
		else:
			message = ': connection problem'
		rb.error = message
		rb.returncode = ReturnCode.BAD_SSH
		logger.error(message + detail)
	return rb


if __name__ == '__main__':

	import sys
	import os
	import getpass

	assert (len(sys.argv) > 1 and '@' in sys.argv[1]) # usage: prog user@host
	
	os.environ['PATH'] = f'{DIR}\\openssh;' + os.environ['PATH']
	userhost = sys.argv[1]
	password = os.environ['GOLDDRIVE_PASS']
	port=22

	if ':' in userhost:
		userhost, port = userhost.split(':')                             
	
	logging.basicConfig(level=logging.INFO)
	# logging.basicConfig(level=logging.DEBUG)

	if has_app_keys() and testssh(userhost, port).returncode == ReturnCode.OK:
		logger.info('SSH authentication is OK, no need to setup.')
	else:
		main(userhost, password, port)


