import os
import socket
from ssh2.session import Session
from ssh2.sftp import LIBSSH2_FXF_READ, LIBSSH2_SFTP_S_IRUSR
from ssh2.exceptions import SFTPHandleError, SFTPProtocolError

BYTES = 1024*1024*1024
results = {}

remotefile = '/tmp/file.bin'
localfile = 'C:\\Temp\\file.bin'
md5 = '6e18dce63ed54dae611f4172b5b2c5b6'
host = '192.168.100.201'
port = 2222
user = 'support'
privatekey = os.path.expanduser('~\\.ssh\\id_rsa-support-golddrive')

statfs_cache = {}
stat_cache = {}

class SFTP(object):

	def __init__(self):
		self.fh = None
		self.connected = False
		# testing
		self.connect(user, privatekey, host, port)

	def connect(self, user, privatekey, host, port=22):
		self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		self.sock.connect((host, port))
		self.session = Session()
		self.session .handshake(self.sock)
		self.session .userauth_publickey_fromfile(user, privatekey)   
		self.sftp = self.session.sftp_init()
		self.connected = True
		print(f'SFTP connected to {user}@{host}:{port}')

	def hello(self):
		
		return 'hello'

	def disconnect(self):

		self.session.disconnect()


	def realpath(self, path):
		pass

	def statfs(self, path):		
		print(f'python statfs: {path}')
		if path in statfs_cache:
			return statfs_cache[path]

		st = -1
		try:
			st = self.sftp.statvfs(path)		
			if type(st) == type(int):
				print('error:', st)
				return st
			else:
				result = (st.f_bsize, st.f_frsize, st.f_blocks, st.f_bfree, st.f_bavail, st.f_fsid, st.f_namemax)
				# print(type(st), st)
				print(f'statfs {path}: {result}')
				statfs_cache[path] = result
				return result
		except Exception as ex:
			print('error:', ex)
			return st

	def open(self, path):
		pass

	def close(self, path):
		pass

	def getattr(self, path):
		pass

	def fstat(self, path):
		print(f'python fstat: {path}')
		if path in stat_cache:
			return stat_cache[path]
		st = -1
		try:
			st = self.sftp.stat(path)
			if type(st) == type(int):
				print('error:', st)
			else:
				print(type(st))
				print(f'permissions: { st.permissions }')
				print(f'flags: { st.flags }')
				print(os.lstat(path))
				result = (st.uid, st.gid, st.atime, st.mtime, st.permissions, st.filesize, st.flags)
				stat_cache[path] = result
				return result
		except Exception as ex:
			print('error:', ex)
		return st

	def lstat(self, path):
		pass

	def unlink(self, path):
		pass

	def rename(self, oldpath, newpath):
		pass

	def mkdir(self, path):
		pass

	def opendir(self, path):

		fh = None
		try:
			fh = self.sftp.opendir(path)
		except SFTPHandleError as ex:
			print(ex)
		return fh

	def readdir(self, fh):
		if fh:
			for length, name, attrs in fh.readdir():
				print(length, name.decode('utf-8'), attrs.uid, attrs.filesize)

	def closedir(self, fh):
		rc = 0
		if fh:
			rc = fh.close()
		return rc

	def truncate(self, path):
		pass

	def read(self, fh):
		pass

	def write(self, fh):
		pass




	def copy(self):
		print('copying with ssh2-python...')
		t = time.time()
		with sftp.open(remotefile, 
			LIBSSH2_FXF_READ, LIBSSH2_SFTP_S_IRUSR) as r, \
			open(localfile,'wb') as w:
			for size, data in r:
				w.write(data)
				# print(f'size: {size}')
				# return
		set_result('ssh2-python', time.time() - t)
		subprocess.run(f'del {localfile}', shell=True)


def main():
	sftp = SFTP()
	# sftp.connect(user, privatekey, host, port)
	# fh = sftp.opendir('/home')
	sftp.statfs('/home')
	sftp.statfs('/desktop.ini')
	sftp.fstat('/home')
	sftp.fstat('/desktop.ini')
	# sftp.readdir(fh)
	# sftp.closedir(fh)
	

print('spft module imported')

if __name__ == '__main__':
	main()