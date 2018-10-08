# setup ssh keys

import os
import paramiko
import logging
import util

logger = logging.getLogger('golddrive')
logging.getLogger("paramiko.transport").setLevel(logging.WARNING)

DIR = os.path.dirname(os.path.realpath(__file__))

def testhost(userhost, port=22):
	'''
	Test if host respond to port
	Return: True or False
	'''
	logger.info(f'Testing port {port} at {userhost}...')
	user, host = userhost.split('@')
	client = paramiko.SSHClient()
	client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
	rb = util.ReturnBox()
	try:
		client.connect(hostname=host, username=user, 
				password='', port=port, timeout=5)	
		rb.returncode = util.ReturnCode.OK
	except (paramiko.ssh_exception.AuthenticationException,
		paramiko.ssh_exception.BadAuthenticationType,
		paramiko.ssh_exception.PasswordRequiredException):
		rb.returncode = util.ReturnCode.OK
	except Exception as ex:
		rb.returncode = util.ReturnCode.BAD_HOST
		rb.error = str(ex)
	finally:
		client.close()
	return rb

def testlogin(userhost, password, port=22):
	'''
	Test ssh password authentication
	'''
	logger.info(f'Logging in with password for {userhost}...')
	rb = util.ReturnBox()
	if not password:
		rb.returncode =util.ReturnCode.BAD_LOGIN
		rb.error = 'Empty password'
		return rb

	user, host = userhost.split('@')
	client = paramiko.SSHClient()
	client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
	
	try:
		client.connect(hostname=host, username=user, 
				password=password, port=port, timeout=10, 
				look_for_keys=False)
		rb.returncode = util.ReturnCode.OK
	except (paramiko.ssh_exception.AuthenticationException,
		paramiko.ssh_exception.BadAuthenticationType,
		paramiko.ssh_exception.PasswordRequiredException) as ex:
		rb.returncode = util.ReturnCode.BAD_LOGIN
		rb.error = str(ex)
	except Exception as ex:
		rb.returncode = util.ReturnCode.BAD_HOST
		rb.error = str(ex)
	finally:
		client.close()
	return rb

def testssh(userhost, seckey, port=22):
	'''
	Test ssh key authentication
	'''
	logger.info(f'Testing ssh keys for {userhost} using key {seckey}...')
	
	rb = testhost(userhost, port)
	if rb.returncode == util.ReturnCode.BAD_HOST:
		return rb
	
	if not os.path.exists(seckey):
		seckey_win = seckey.replace('/','\\')
		logger.error(f'Key does not exist: {seckey_win}')
		rb.returncode = util.ReturnCode.BAD_LOGIN
		rb.error = "No key"
		return rb

	cmd = f'''ssh
		-i "{seckey}"
		-p {port} 
		-o PasswordAuthentication=no
		-o StrictHostKeyChecking=no 
		-o UserKnownHostsFile=/dev/null
		-o BatchMode=yes 
		{userhost} "echo ok"'''
	r = util.run(cmd, capture=True, timeout=10)
	
	if r.stdout == 'ok':
		# success
		rb.returncode = util.ReturnCode.OK
	elif 'Permission denied' in r.stderr:
		# wrong user or key issue
		rb.returncode = util.ReturnCode.BAD_LOGIN
		rb.error = 'Access denied'
	else:
		# wrong port: connection refused 
		# unknown host: connection timeout
		logger.error(r.stderr)
		rb.returncode = util.ReturnCode.BAD_HOST
		rb.error = r.stderr
	return rb

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

def main(userhost, password, userkey='', port=22):
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
			rbkey.returncode = util.ReturnCode.BAD_SSH
			return rbkey
		else:
			pubkey = rbkey.output

	# connect
	client = paramiko.SSHClient()
	client.set_missing_host_key_policy(paramiko.AutoAddPolicy())

	# transfer key using user key
	if userkey:
		try:
			logger.info('Connecting using user keys...')
			pkey = paramiko.RSAKey.from_private_key_file(userkey)
			client.connect(hostname=host, username=user,
						pkey=pkey, look_for_keys=False, port=port, timeout=10)
		except paramiko.ssh_exception.AuthenticationException:
			rb.error = f'User or password wrong'
			rb.returncode = 1
		except Exception as ex:
			rb.error = f'connection error: {ex}'
			rb.returncode = 2
	if rb.error:
		logger.error(rb.error)

	# use password
	if rb.error or not userkey:
		rb.error = ''
		try:
			logger.info('Connecting using password...')
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
		rb.returncode = util.ReturnCode.BAD_SSH
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
		rb.returncode = util.ReturnCode.BAD_SSH
		rb.error = f'error transfering public key: {ex}'
		return rb
	finally:
		client.close()

	err = stderr.read()
	if err:
		logger.error(err)
		rb.returncode = util.ReturnCode.BAD_SSH
		rb.error = f'error transfering public key, error: {err}'
		return rb
	
	rb = testssh(userhost, seckey, port)
	if rb.returncode == util.ReturnCode.OK:
		rb.output = "SSH setup successfull." 
		logger.info(rb.output)
	else:
		message = 'SSH setup test failed'
		detail = ''
		if rb.returncode == util.ReturnCode.BAD_LOGIN:
			detail = ': authentication probem'
		else:
			message = ': connection problem'
		rb.error = message
		rb.returncode = util.ReturnCode.BAD_SSH
		logger.error(message + detail)
	return rb


if __name__ == '__main__':

	import sys
	import getpass
	assert (len(sys.argv) > 1 and
			'@' in sys.argv[1]) # usage: prog user@host
	userhost = sys.argv[1]
	password = getpass.getpass('Linux password: ')
	port=22
	userkey = os.path.expandvars(r'%USERPROFILE%\.ssh\id_rsa')
	if ':' in userhost:
		userhost, port = userhost.split(':')                             
	logging.basicConfig(level=logging.INFO)

	main(userhost, password, userkey, port)