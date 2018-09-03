# setup ssh keys

import os
import subprocess
import paramiko

DIR = os.path.dirname(os.path.realpath(__file__))

def checkssh(ssh, userhost, port=22):

	cmd = f'"{ssh}" -p {port} -o StrictHostKeyChecking=no -o BatchMode=yes {userhost} echo ok 2>&1'
	r = subprocess.run(cmd, capture_output=True, shell=True, text=True)
	return r.stdout.strip() == 'ok'

def main(ssh, userhost, password, port=22):

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
	else:
		print('Private key already exists.')
		if checkssh(ssh, userhost, port):
			print('SSH authentication is ok.')
			return 0

	# Generate public key
	print(f'Publising public key...')
	if not sk:
		sk = paramiko.RSAKey.from_private_key_file(seckey)
	key = f'ssh-rsa {sk.get_base64()} {userhost}'
	# with open(f'{pubkey}', 'r') as f:
	# 	key = f.read().strip()

	# Copy to the target machines.
	cmd = f"mkdir -p .ssh && echo '{key}' >> .ssh/authorized_keys && chmod 700 .ssh/authorized_keys"
	# print(cmd)
	user, host = userhost.split('@')
	client = paramiko.SSHClient()
	client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
	client.connect(hostname=host, username=user, password=password, port=port)
	stdin, stdout, stderr = client.exec_command(cmd)
	ok = checkssh(ssh, userhost, port)
	if ok:
		print("SSH setup successfull.")
	else:
		print("SSH setup failed.")
	return 0 if ok else 1


if __name__ == '__main__':
	import sys
	assert len(sys.argv) > 2 and '@' in sys.argv[1] # usage: prog user@host password
	userhost = sys.argv[1]
	password = sys.argv[2]
	port=22
	if ':' in userhost:
		userhost, port = userhost.split(':')		
	ssh = r'C:\Program Files\SSHFS-Win\bin\ssh.exe'
	sys.exit(main(ssh, userhost, password, port))