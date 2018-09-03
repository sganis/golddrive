# setup ssh keys

import os
import subprocess
import paramiko
import logging
from util import ReturnBox

DIR = os.path.dirname(os.path.realpath(__file__))
logger = logging.getLogger('ssh-drive')
logging.getLogger("paramiko.transport").setLevel(logging.WARNING)

def testssh(ssh, user, host, port=22):
	'''
	Test ssh key authentication, return True or False
	'''
	logger.info(f'Testing ssh keys for {user}@{host}...')
	cmd = f'"{ssh}" -p {port} -o StrictHostKeyChecking=no -o BatchMode=yes {user}@{host} echo ok 2>&1'
	r = subprocess.run(cmd, capture_output=True, shell=True, text=True)
	return r.stdout.strip() == 'ok'

def main(ssh, user, host, password, port=22):
	'''
	Setup ssh keys, return ReturnBox
	'''
	rb = ReturnBox()
	userhost =f'{user}@{host}'
	logger.info(f'Setting up ssh keys for {userhost}...')
	client = paramiko.SSHClient()
	client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
	try:
		client.connect(hostname=host, username=user, 
			password=password, port=port, timeout=10)
	except paramiko.ssh_exception.AuthenticationException:
		rb.error = f'User or password wrong for\n{userhost}:{port}'
	except Exception as ex:
		rb.error = f'connection error: {ex}'
	if rb.error:
		logger.error(rb.error)
		if 'getaddrinfo failed' in rb.error:
			rb.error = f'{host} not found'
		return rb

	sshdir = os.path.expandvars("%USERPROFILE%\\.ssh")
	seckey = os.path.join(sshdir, 'id_rsa')
	pubkey = os.path.join(sshdir, 'id_rsa.pub')
	sk 	   = None

	# Check if keys need to be generated
	if not os.path.exists(seckey):
		# generate rsa keys
		logger.info('Generating new ssh keys...')
		sk = paramiko.RSAKey.generate(2048)
		sk.write_private_key_file(seckey)	
	else:
		logger.info('Private key already exists.')
		if testssh(ssh, user, host, port):
			rb.output = 'SSH authentication is OK.'
			logger.info(rb.output)
			return rb

	# Generate public key
	logger.info(f'Publising public key...')
	if not sk:
		sk = paramiko.RSAKey.from_private_key_file(seckey)
	key = f'ssh-rsa {sk.get_base64()} {userhost}'
	# with open(f'{pubkey}', 'r') as f:
	# 	key = f.read().strip()

	# Copy to the target machines.
	cmd = f"mkdir -p .ssh && echo '{key}' >> .ssh/authorized_keys && chmod 700 .ssh/authorized_keys"
	# print(cmd)

	ok = False
	stdin, stdout, stderr = client.exec_command(cmd)
	logger.info(stdout)
	if stderr:
		logger.error(stderr)
	ok = testssh(ssh, user, host, port)
	if ok:
		rb.output = "SSH setup successfull." 
		logger.info(rb.output)
	else:
		rb.error = "SSH setup failed."
		logger.error(rb.error)
	return rb


if __name__ == '__main__':

	import sys
	assert len(sys.argv) > 2 and '@' in sys.argv[1] # usage: prog user@host password
	userhost = sys.argv[1]
	password = sys.argv[2]
	port=22
	if ':' in userhost:
		userhost, port = userhost.split(':')		
	ssh = fr'C:\Program Files\SSHFS-Win\bin\ssh.exe'
	user, host = userhost.split('@')
	main(ssh, user, host, password, port)