# setup ssh keys

import os
import util

DIR = os.path.dirname(os.path.realpath(__file__))

def test(ssh, userhost):

	cmd = f'"{ssh}" -o StrictHostKeyChecking=no -o BatchMode=yes {userhost} echo ok 2>&1'
	ok, err, ret = util.run(cmd)
	return ok == 'ok'

def main(ssh, keygen, userhost):

	print(f'Checking ssh keys for {userhost}...')

	sshdir = os.path.expandvars("%USERPROFILE%\\.ssh")
	seckey = os.path.join(sshdir, 'id_rsa')
	pubkey = os.path.join(sshdir, 'id_rsa.pub')
	
	# Check if keys need to be generated
	if not os.path.exists(seckey):
		print('Generating new ssh keys...')
		cmd = f'mkdir {sshdir} 2>nul && echo y | "{keygen}" -f {seckey} -q -N ""'
		os.system(cmd)
	else:
		print('Private key already exists.')
		if test(ssh, userhost):
			return

	print(f'Publising public key...\n')

	# Generate and read the new keys into a variable
	cmd = f'"{keygen}" -y -f {seckey} > {pubkey}'
	with open(f'{pubkey}', 'r') as f:
		key = f.read().strip()

	# Copy these keys to the target machines.
	cmd = f'echo y | "{ssh}" -o StrictHostKeyChecking=no {userhost} "mkdir -p .ssh && echo \'{key}\' >> .ssh/authorized_keys && chmod 700 .ssh/authorized_keys"'
	ret = os.system(cmd)
	if test(ssh, userhost):
		print("SSH setup successfull")
	else:
		print('SSH setup failed')


if __name__ == '__main__':
	import sys
	assert len(sys.argv) > 1 and '@' in sys.argv[1] # usage: prog user@host
	user, host = sys.argv[1].split('@')
	ssh = r'C:\Program Files\SSHFS-Win\bin\ssh.exe'
	keygen = r'C:\Program Files\SSHFS-Win\bin\ssh-keygen.exe'
	main(ssh, keygen, userhost)