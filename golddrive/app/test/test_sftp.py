# test_sftp
import os
import time
import paramiko
import subprocess

host = '192.168.100.201'
port = 22
username = 'sag'
ssh = paramiko.SSHClient()
ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
pkey = paramiko.RSAKey.from_private_key_file(os.path.expanduser('~/.ssh/id_rsa-sag-golddrive'))
ssh.connect(hostname=host, port=port, username=username, pkey=pkey)
sftp = ssh.open_sftp()

print('copying with paramiko...')
t = time.time()
sftp.get('/tmp/vs2015.3.com_enu.iso', r'C:\\Temp\\vs2015.3.com_enu.iso')
secs = time.time() - t
print(f'elapsed: {secs} secs')
subprocess.run('del C:\\Temp\\vs2015.3.com_enu.iso', shell=True)
sftp.close()
ssh.close()

print('copying...')
t = time.time()
subprocess.run('copy S:\\tmp\\vs2015.3.com_enu.iso C:\\Temp\\vs2015.3.com_enu2.iso', shell=True)
secs = time.time() - t
print(f'elapsed: {secs} secs')
subprocess.run('del C:\\Temp\\vs2015.3.com_enu2.iso', shell=True)

