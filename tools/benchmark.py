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

remotefile = '/tmp/file.bin'
localfile = 'C:\\Temp\\file.bin'
md5 = '9aba09e0fc1d288d7ccd7719f3d0f184'
host = '192.168.100.201'
port = 22
user = 'sag'
privatekey = os.path.expanduser('~\\.ssh\\id_rsa-sag-golddrive')
DIR = os.path.dirname(os.path.realpath(__file__))

os.environ['PATH'] = fr'{DIR}\sanssh;' + os.environ['PATH']
os.environ['PATH'] = fr'{DIR}\sanssh2;' + os.environ['PATH']
os.environ['PATH'] = fr'{DIR}\openssh;' + os.environ['PATH']


def validate(fname, md5):	
	t = time.time()
	h = md5sum(fname)
	print(f'md5 took: { time.time() - t } secs')
	print(h)
	assert md5 == h

def set_result(name, secs):
	results[name] = round(BYTES/1024/1024/secs)
	validate(localfile, md5)
	
def md5sum(fname):
	h = hashlib.md5()
	with open(fname, "rb") as f:
		for chunk in iter(lambda: f.read(32768), b""):
			h.update(chunk)
	return h.hexdigest()


def main():
	# test data: 
	# create a 1gb file in server
	# python -c "import os; w=open('/tmp/file.bin','wb');w.write(os.urandom(1024*1024*1024))"
	# update md5 = md5sum file.bin
	pkey = paramiko.RSAKey.from_private_key_file(privatekey)
	transport = paramiko.Transport((host, port))
	transport.connect(username=user, pkey=pkey) 
	sftp = paramiko.SFTPClient.from_transport(transport)
	print('copying with paramiko...')
	t = time.time()
	sftp.get(remotefile, localfile)
	set_result('paramiko', time.time() - t)
	subprocess.run(f'del {localfile}', shell=True)
	sftp.close()
	transport.close()

	print('copying with ssh2-python...')
	sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	sock.connect((host, 22))
	s = Session()
	s.handshake(sock)
	s.userauth_publickey_fromfile(user, privatekey)   
	sftp = s.sftp_init()
	t = time.time()
	with sftp.open(remotefile, 
		LIBSSH2_FXF_READ, LIBSSH2_SFTP_S_IRUSR) as r, \
		open(localfile,'wb') as w:
		for size, data in r:
			w.write(data)
	set_result('ssh2-python', time.time() - t)
	subprocess.run(f'del {localfile}', shell=True)

	print('copying with sanssh...')
	t = time.time()
	subprocess.run(	f'sanssh {host} {user} {remotefile} {localfile} {privatekey}', shell=True)
	set_result('sanssh', time.time() - t)
	subprocess.run('del localfile', shell=True)

	print('copying with sanssh2...')
	t = time.time()
	subprocess.run(	f'sanssh2 {host} {user} {remotefile} {localfile} {privatekey}', shell=True)
	set_result('sanssh2', time.time() - t)
	subprocess.run('del localfile', shell=True)

	print('copying with sshfs...')
	t = time.time()
	subprocess.run(f'copy Y:\\tmp\\file.bin {localfile}', shell=True)
	set_result('sshfs', time.time() - t)
	subprocess.run(f'del {localfile}', shell=True)


	print('copying with openssh...')
	os.chdir('C:\\Temp')
	t = time.time()
	subprocess.run(
		f'echo get {remotefile} |sftp -q -i {privatekey} -B 65536 -R 256 -P {port} {user}@{host}', 
		shell=True)
	set_result('openssh sftp', time.time() - t)
	subprocess.run(f'del {localfile}', shell=True)

	# display results order by fastest
	sorted_results = sorted(results, key=results.get)
	slowest = sorted_results[0]
	for r in sorted_results:
		rel = round(results[r]/results[slowest],1)
		rel = rel if rel != 1.0 else 'Ref'
		print(f'{r:<14}: {results[r]:>4} MB/s {rel}x')


if __name__ == '__main__':
	main()