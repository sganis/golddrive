# setup ssh keys

import os
import subprocess
import paramiko

DIR = os.path.dirname(os.path.realpath(__file__))

def test(ssh, userhost, port=22):

	cmd = f'"{ssh}" -p {port} -o StrictHostKeyChecking=no -o BatchMode=yes {userhost} echo ok 2>&1'
	r = subprocess.run(cmd, capture_output=True, shell=True, text=True)
	return str(r.stdout).strip() == 'ok'

def main(ssh, keygen, userhost, password, port=22):

	print(f'Checking ssh keys for {userhost}...')

	sshdir = os.path.expandvars("%USERPROFILE%\\.ssh")
	seckey = os.path.join(sshdir, 'id_rsa')
	pubkey = os.path.join(sshdir, 'id_rsa.pub')
	sk 	   = None

	# Check if keys need to be generated
	if not os.path.exists(seckey):
		# generate rsa keys
		print('Generating new ssh keys...')
		sk = paramiko.RSAKey.generate(2048)
		sk.write_private_key_file(seckey)	
		# cmd = f'mkdir {sshdir} 2>nul && echo y | "{keygen}" -f {seckey} -q -N ""'
		# os.system(cmd)
	else:
		print('Private key already exists.')
		if test(ssh, userhost, port):
			return

	print(f'Publising public key...\n')

	# Generate and read the new keys into a variable
	if not sk:
		sk = paramiko.RSAKey.from_private_key_file(seckey)
	key = sk.get_base64()

	# todo: compare key with the existing 

	with open(pubkey, 'w') as w:
		w.write(key)

	# cmd = f'"{keygen}" -y -f {seckey} > {pubkey}'
	# with open(f'{pubkey}', 'r') as f:
	# 	key = f.read().strip()

	# Copy these keys to the target machines.
	cmd = f"mkdir -p .ssh && echo '{key}' >> .ssh/authorized_keys && chmod 700 .ssh/authorized_keys"
	# cmd = f'"{ssh}" -p {port} -o StrictHostKeyChecking=no {userhost} "mkdir -p .ssh && echo \'{key}\' >> .ssh/authorized_keys && chmod 700 .ssh/authorized_keys"'
	# cmd = [f'"{ssh}"','-p',f'{port}','-oStrictHostKeyChecking=no',f'{userhost}',
	# 	f'"mkdir -p .ssh && echo \'{key}\' >> .ssh/authorized_keys && chmod 700 .ssh/authorized_keys"']
	print(cmd)
	# os.system(cmd)
	user, host = userhost.split('@')
	client = paramiko.SSHClient()
	client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
	client.connect(hostname=host, username=user, password=password, port=port)
	stdin, stdout, stderr = client.exec_command(cmd)

	if test(ssh, userhost, port):
		print("SSH setup successfull")
	else:
		print('SSH setup failed')


if __name__ == '__main__':
	import sys
	assert len(sys.argv) > 2 and '@' in sys.argv[1] # usage: prog user@host password
	userhost = sys.argv[1]
	password = sys.argv[2]
	port=22
	if ':' in userhost:
		userhost, port = userhost.split(':')		
	ssh = r'C:\Program Files\SSHFS-Win\bin\ssh.exe'
	keygen = r'C:\Program Files\SSHFS-Win\bin\ssh-keygen.exe'
	main(ssh, keygen, userhost, password, port)