# setup ssh keys

import os
import paramiko
import logging
import util

logger = logging.getLogger('golddrive')
logging.getLogger("paramiko.transport").setLevel(logging.WARNING)

DIR = os.path.dirname(os.path.realpath(__file__))

def login_password(ssh, userhost, password, port=22):
	'''
	Test ssh password authentication
	Return:
		0 - Success
		1 - Bad authentication
		2 - Bad connection
	'''
	logger.info(f'Logging in with password for {userhost}...')
	if not password:
		return 1
	user, host = userhost.split('@')
	client = paramiko.SSHClient()
	client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
	result = -1
	try:

		client.connect(hostname=host, username=user, 
				password=password, port=port, timeout=10, 
				look_for_keys=False)	
		result = 0
	except (paramiko.ssh_exception.AuthenticationException,
		paramiko.ssh_exception.BadAuthenticationType,
		paramiko.ssh_exception.PasswordRequiredException):
		result = 1
	except Exception as ex:
		result = 2
	finally:
		client.close()
	return result

def testssh(ssh, userhost, seckey, port=22):
	'''
	Test ssh key authentication
	Return:
		0 - Success
		1 - Bad authentication
		2 - Bad connection
	'''
	logger.info(f'Testing ssh keys for {userhost} using key {seckey}...')
	# if not seckey:
	# 	user, host = userhost.split('@')
	# 	seckey = util.getAppKey(user)
	
	if not os.path.exists(seckey):
		seckey_win = seckey.replace('/','\\')
		logger.info(f'Key does not exist: {seckey_win}')
		return 1

	cmd = f'''"{ssh}" 
		-i "{seckey}"
		-p {port} 
		-o PasswordAuthentication=no
		-o StrictHostKeyChecking=no 
		-o UserKnownHostsFile=/dev/null
		-o BatchMode=yes 
		{userhost} "echo ok"'''
	r= util.run(cmd, capture=True, timeout=10)
	
	if r.stdout == 'ok':
		# success
		return 0
	elif 'Permission denied' in r.stderr:
		# wrong user or key issue
		return 1
	else:
		# wrong port: connection refused 
		# unknown host: connection timeout
		logger.error(r.stderr)
		return 2 

def generate_keys(seckey, userhost):
	logger.info('Generating new ssh keys...')
	rb = util.ReturnBox()
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
	rb.output = pubkey
	return rb

def has_app_keys(user):
	appkey = util.getAppKey(user)
	return os.path.exists(appkey)

def setup_keys(userhost, seckey, userkey='', port=22):
	pass

def setup_user_keys(userhost, password, port):
	pass
	'''
	Setup ~/.ssh/id_rsa defaut keys, usefull for testing.
	Run this from commmand line before testing
	'''
	# generate key

	# transfer using password

def main(ssh, userhost, password, userkey='', port=22):
	'''
	Setup ssh keys, return ReturnBox
	'''
	logger.info(f'Setting up ssh keys for {userhost}...')
	rb = util.ReturnBox()

	# app key
	user, host = userhost.split('@')
	seckey = util.getAppKey(user)	

	# Check if keys need to be generated
	pubkey = ''
	if has_app_keys(user):
		logger.info('Private key already exists.')
		sk = paramiko.RSAKey.from_private_key_file(seckey)
		pubkey = f'ssh-rsa {sk.get_base64()} {userhost}'
	else:
		rbkey = generate_keys(seckey, userhost)
		if rbkey.error:
			return rbkey
		else:
			pubkey = rbkey.output

	# connect
	client = paramiko.SSHClient()
	client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
	try:
		if userkey:
			# transfer key using user key
			logger.info('Connecting using user keys...')
			pkey = paramiko.RSAKey.from_private_key_file(userkey)
			client.connect(hostname=host, username=user, 
				pkey=pkey, port=port, timeout=10)
		else:
			# use password
			logger.info('Connecting using password...')
			client.connect(hostname=host, username=user, 
				password=password, port=port, timeout=10,
				look_for_keys=False)			
	except paramiko.ssh_exception.AuthenticationException:
		rb.error = f'User or password wrong'
	except Exception as ex:
		rb.error = f'connection error: {ex}'

	if rb.error:
		logger.error(rb.error)
		if 'getaddrinfo failed' in rb.error:
			rb.error = f'{host} not found'
		client.close()
		return rb

	logger.info(f'Publising public key...')
		
	# Copy to the target machines.
	cmd = fr"mkdir -p .ssh && \
		echo '{pubkey}' >> .ssh/authorized_keys && \
		chmod 700 .ssh/authorized_keys"
	# print(cmd)

	ok = False
	try:
		stdin, stdout, stderr = client.exec_command(cmd, timeout=10)		
	except Exception as ex:
		logger.error(ex)
		rb.error = f'error transfering public key: {ex}'
		return rb
	finally:
		client.close()

	err = stderr.read()
	if err:
		logger.error(err)
		rb.error = f'error transfering public key, error: {err}'
		return rb
	
	result = testssh(ssh, userhost, seckey, port)
	if result == 0:
		rb.output = "SSH setup successfull." 
		logger.info(rb.output)
	else:
		message = 'SSH setup failed'
		detail = ''
		if result == 1:
			detail = ': authentication probem'
		else:
			message = ': connection problem'
		rb.error = message
		logger.error(message + detail)
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