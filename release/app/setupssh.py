# setup ssh keys

import os
import paramiko
import logging
import util

logger = logging.getLogger('golddrive')
logging.getLogger("paramiko.transport").setLevel(logging.WARNING)

DIR = os.path.dirname(os.path.realpath(__file__))


def testssh(ssh, userhost, seckey='', port=22):
	'''
	Test ssh key authentication, return True or False
	'''
	logger.info(f'Testing ssh keys for {userhost}...')
	if not seckey:
		user, host = userhost.split('@')
		seckey = util.defaultKey(user)
	seckey_win = seckey.replace('/','\\')
	if not os.path.exists(seckey):
		logger.info(f'Key does not exist: {seckey_win}')
		return False

	cmd = f'''"{ssh}" 
		-i "{seckey}"
		-p {port} 
		-o StrictHostKeyChecking=no 
		-o UserKnownHostsFile=/dev/null
		-o BatchMode=yes 
		{userhost} "echo ok"'''
	r= util.run(cmd, capture=True, timeout=5)
	return r.stdout == 'ok'

def main(ssh, userhost, password, seckey='', port=22):
	'''
	Setup ssh keys, return ReturnBox
	'''
	
	rb = util.ReturnBox()
	if not password:
		rb.error = 'password is empty'
		return rb

	user, host = userhost.split('@')
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

	if not seckey:
		seckey = util.defaultKey(user)	

	# Check if keys need to be generated
	sk = None
	if not os.path.exists(seckey):
		logger.info('Generating new ssh keys...')
		sk = paramiko.RSAKey.generate(2048)
		try:
			sk.write_private_key_file(seckey)	
		except Exception as ex:
			logger.error(f'{ex}, {seckey}')
			rb.error = str(ex)
			return rb
	else:
		logger.info('Private key already exists.')
		if testssh(ssh, userhost, seckey, port):
			rb.output = 'SSH authentication is OK.'
			logger.info(rb.output)
			return rb

	# Generate public key
	logger.info(f'Publising public key...')
	if not sk:
		sk = paramiko.RSAKey.from_private_key_file(seckey)
	key = f'ssh-rsa {sk.get_base64()} {userhost}'
	# with open(f'{seckey}.pub', 'r') as f:
	# 	key = f.read().strip()

	# Copy to the target machines.
	cmd = fr"mkdir -p .ssh && \
		echo '{key}' >> .ssh/authorized_keys && \
		chmod 700 .ssh/authorized_keys"
	# print(cmd)

	ok = False
	try:
		stdin, stdout, stderr = client.exec_command(cmd, timeout=10)		
	except Exception as ex:
		logger.error(ex)
		rb.error = f'error transfering public key: {ex}'
		return rb

	err = stderr.read()
	if err:
		logger.error(err)
		rb.error = f'error transfering public key, error: {err}'
		return rb
	
	ok = testssh(ssh, userhost, seckey, port)
	if ok:
		rb.output = "SSH setup successfull." 
		logger.info(rb.output)
	else:
		rb.error = "SSH setup failed."
		logger.error(rb.error)
	return rb


if __name__ == '__main__':

	import sys
	assert (len(sys.argv) > 2 and 
		'@' in sys.argv[1]) # usage: prog user@host password
	userhost = sys.argv[1]
	password = sys.argv[2]
	port=22
	if ':' in userhost:
		userhost, port = userhost.split(':')		
	
	ssh_path = fr'C:\Program Files\SSHFS-Win\bin'
	ssh = fr'{ssh_path}\ssh.exe'
	path = os.environ['PATH']
	os.environ['PATH'] = fr'{ssh_path};{path}'
	logging.basicConfig(level=logging.INFO)
	logging.basicConfig(level=logging.INFO)

	main(ssh, userhost, password, '', port)