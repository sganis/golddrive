# mount drive using sshfs
# 
import util

def main(sshfs, drive, userhost):
	print(f'Mounting {drive} {userhost}...')
	
	cmd = f'''
		"{sshfs}" {userhost}:/ {drive} 
		-orellinks -oreconnect
		-ouid=-1,gid=-1,create_umask=0007 
		-oVolumePrefix=/sshfs/{userhost}
		-ovolname=LINUX-{userhost} 
		-oFileSystemName=SSHFS 
		-oStrictHostKeyChecking=no
		-oServerAliveInterval=10 
		-oServerAliveCountMax=10000
		-oFileInfoTimeout=10000 
		-oDirInfoTimeout=10000 
		-oVolumeInfoTimeout=10000
		'''.replace('\n',' ').replace('\t','')
	# print(cmd)
	util.run(cmd)

	# [HKEY_CURRENT_USER\Network\Z]
	# "RemotePath"="\\\\sshfs\\user@localhost\\..\\.."
	# "UserName"=""
	# "ProviderName"="Windows File System Proxy"
	# "ProviderType"=dword:20737046
	# "ConnectionType"=dword:00000001
	# "ConnectFlags"=dword:00000000
	letter = drive.strip(':')
	remotepath = f'\\\\sshfs\\{userhost}\\..\\..'
	key = fr'HKCU\Network\{letter}'
	util.run(f'reg add {key} /v RemotePath /d "{remotepath}" /f')
	util.run(f'reg add {key} /v UserName /d "" /f')
	util.run(f'reg add {key} /v ProviderName /d "Windows File System Proxy" /f')
	util.run(f'reg add {key} /v ProviderType /d 20737046 /t REG_DWORD /f')
	util.run(f'reg add {key} /v ConnectionType /d 1 /t REG_DWORD /f')
	util.run(f'reg add {key} /v ConnectFlags /d 0 /t REG_DWORD /f')

if __name__ == '__main__':
	import sys
	assert len(sys.argv) > 2 and '@' in sys.argv[2] # usage: mount.py <drive> <user@host>
	sshfs = r'C:\Program Files\SSHFS-Win\bin\sshfs.exe'
	sys.path.insert(0, os.path.dirname(sshfs))
	main(sshfs, *sys.argv[1:])