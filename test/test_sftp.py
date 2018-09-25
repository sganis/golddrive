# test_sftp
import os
import time
import paramiko
import subprocess

def main():
	host = '192.168.100.201'
	port = 22
	user = 'sag'
	keypath = os.path.expanduser('~/.ssh/id_rsa-sag-golddrive')
	pkey = paramiko.RSAKey.from_private_key_file(keypath)
	
	# ssh = paramiko.SSHClient()
	# ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
	# ssh.connect(hostname=host, port=port, username=user, pkey=pkey)
	# sftp = ssh.open_sftp()

	transport = paramiko.Transport((host, port))
	# transport.get_security_options().ciphers = ('chacha20-poly1305@openssh.com', )
	transport.connect(username=user, pkey=pkey) 
	sftp = paramiko.SFTPClient.from_transport(transport)
	
	print('copying with paramiko...')
	t = time.time()
	sftp.get('/tmp/file.bin', r'C:\\Temp\\file.bin')
	secs = time.time() - t
	print(f'elapsed: {secs} secs')
	subprocess.run('del C:\\Temp\\file.bin', shell=True)
	sftp.close()
	# ssh.close()

	print('copying with copy...')
	t = time.time()
	subprocess.run('copy S:\\tmp\\file.bin C:\\Temp\\file.bin', shell=True)
	secs = time.time() - t
	print(f'elapsed: {secs} secs')
	subprocess.run('del C:\\Temp\\file.bin', shell=True)

	print('using sftp directly...')
	os.environ['PATH'] = os.path.dirname(os.path.realpath(__file__)) + '\\..\\golddrive\\sshfs\\openssh;' + os.environ['PATH'] 
	os.chdir('C:\\Temp')
	t = time.time()
	subprocess.run(f'echo get /tmp/file.bin |sftp -q -i {keypath} -B 65536 -R 256 -P {port} {user}@{host}', 
		shell=True)
	secs = time.time() - t
	print(f'elapsed: {secs} secs')
	subprocess.run('del C:\\Temp\\file.bin', shell=True)



if __name__ == '__main__':
	main()