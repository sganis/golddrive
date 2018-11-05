# benchmark different technologies
import os
import time
import paramiko
import subprocess
import hashlib
import socket
from ssh2.session import Session
from ssh2.sftp import LIBSSH2_FXF_READ, LIBSSH2_SFTP_S_IRUSR

BYTES = 1024*1024*1024
results = {}

host = os.environ['GOLDDRIVE_HOST']
port = os.environ['GOLDDRIVE_PORT']
user = os.environ['GOLDDRIVE_USER']

remote_down = '/tmp/file.bin'
remote_up = f'/home/{user}/file.bin'
remote_sanfs = fr'W:\tmp\file.bin'
remote_sanfs_up = fr'W:\home\{user}\file.bin'
remote_sshfs = fr'S:\tmp\file.bin'
remote_sshfs_up = fr'S:\home\{user}\file.bin'
localfile = fr'C:\Temp\file.bin'
privatekey = os.path.expanduser(fr'~\.ssh\id_rsa-{user}-golddrive')
DIR = os.path.dirname(os.path.realpath(__file__))

os.environ['PATH'] = fr'{DIR}\sanssh;' + os.environ['PATH']
# os.environ['PATH'] = fr'{DIR}\sanssh-libssh;' + os.environ['PATH']
os.environ['PATH'] = fr'{DIR}\openssh;' + os.environ['PATH']


def validate(fname, md5):	
	t = time.time()
	h = md5sum(fname)
	print(f'md5 took: { time.time() - t } secs')
	print(h)
	assert md5 == h

def set_result(name, secs):
	results[name] = round(BYTES/1024/1024/secs)
	
def md5sum(fname):
	h = hashlib.md5()
	with open(fname, "rb") as f:
		for chunk in iter(lambda: f.read(32768), b""):
			h.update(chunk)
	return h.hexdigest()

def md5sumRemote(remote_down):
	ssh = paramiko.SSHClient()
	pkey = paramiko.RSAKey.from_private_key_file(privatekey)
	ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
	ssh.connect(host, port, user, pkey=pkey)
	cmd = f'md5sum {remote_down}'
	stdin, stdout, stderr = ssh.exec_command(cmd)
	md5 = stdout.read().split()[0].strip().decode('utf-8')
	ssh.close()
	return md5

def main():
	# test data: 
	# create a 1gb file in server
	# python -c "import os; w=open('/tmp/file.bin','wb');w.write(os.urandom(1024*1024*1024))"
	# update md5 = md5sum file.bin

	md5 = md5sumRemote(remote_down)
	print(f'testing with file: {remote_down}\nmd5: {md5}')

	# print('\n### sanssh ###')
	# t = time.time()
	# subprocess.run(       f'sanssh {host} {port} {user} {remote_down} {localfile} {privatekey}', shell=True)
	# set_result('sanssh', time.time() - t)
	# subprocess.run(f'del {localfile}', shell=True)
	
	print('downloading with sanfs...')
	t = time.time()
	subprocess.run(f'copy {remote_sanfs} {localfile}', shell=True)
	set_result('sanfs download', time.time() - t)
	assert md5 == md5sum(localfile)
	# subprocess.run(f'del {localfile}', shell=True)

	print('uploading with sanfs...')
	t = time.time()
	subprocess.run(f'copy {localfile} {remote_sanfs_up}', shell=True)
	set_result('sanfs upload', time.time() - t)
	assert md5 == md5sumRemote(remote_up)
	subprocess.run(f'del {localfile}', shell=True)

	print('downloading with sshfs...')
	t = time.time()
	subprocess.run(f'copy {remote_sshfs} {localfile}', shell=True)
	set_result('sshfs download', time.time() - t)
	assert md5 == md5sum(localfile)
	# subprocess.run(f'del {localfile}', shell=True)

	print('uploading with sshfs...')
	t = time.time()
	subprocess.run(f'copy {localfile} {remote_sshfs_up}', shell=True)
	set_result('sshfs upload', time.time() - t)
	assert md5 == md5sumRemote(remote_up)
	subprocess.run(f'del {localfile}', shell=True)

	# print('copying with openssh...')
	# os.chdir('C:\\Temp')
	# t = time.time()
	# subprocess.run(
	# 	f'echo get {remote_down} |sftp -q -i {privatekey} -B 65536 -R 256 -P {port} {user}@{host}', 
	# 	shell=True)
	# set_result('openssh', time.time() - t)
	# subprocess.run(f'del {localfile}', shell=True)


	# print('copying with paramiko...')
	# pkey = paramiko.RSAKey.from_private_key_file(privatekey)
	# transport = paramiko.Transport((host, port))
	# transport.connect(username=user, pkey=pkey) 
	# sftp = paramiko.SFTPClient.from_transport(transport)
	# t = time.time()
	# sftp.get(remote_down, localfile)
	# set_result('paramiko', time.time() - t)
	# subprocess.run(f'del {localfile}', shell=True)
	# sftp.close()
	# transport.close()

	# print('copying with ssh2-python...')
	# sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	# sock.connect((host, 22))
	# s = Session()
	# s.handshake(sock)
	# s.userauth_publickey_fromfile(user, privatekey)   
	# sftp = s.sftp_init()
	# t = time.time()
	# with sftp.open(remote_down, 
	# 	LIBSSH2_FXF_READ, LIBSSH2_SFTP_S_IRUSR) as r, \
	# 	open(localfile,'wb') as w:
	# 	for size, data in r:
	# 		w.write(data)
	# set_result('ssh2-python', time.time() - t)
	# subprocess.run(f'del {localfile}', shell=True)

	# print('copying with sanssh...')
	# t = time.time()
	# subprocess.run(	f'sanssh {host} {user} {remote_down} {localfile} {privatekey}', shell=True)
	# set_result('sanssh', time.time() - t)
	# subprocess.run(f'del localfile', shell=True)

	# print('copying with sanssh-libssh...')
	# t = time.time()
	# subprocess.run(	f'sanssh-libssh {host} {user} {remote_down} {localfile} {privatekey}', shell=True)
	# set_result('sanssh-libssh', time.time() - t)
	# subprocess.run('del localfile', shell=True)

	# display results order by fastest
	sorted_results = sorted(results, key=results.get)
	slowest = sorted_results[0]
	for r in sorted_results:
		rel = round(results[r]/results[slowest],1)
		rel = rel if rel != 1.0 else 'Ref'
		print(f'{r:<14}: {results[r]:>4} MB/s {rel}x')




if __name__ == '__main__':
	main()