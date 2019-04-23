import os
import time
import psutil
import subprocess
 

def main():
	sshfs_path = r'D:\Users\ganissa\GoldDrive\v1.0.6\sshfs\bin;'
	fsptool = r'C:\Program Files (x86)\WinFsp\bin\fsptool-x64.exe'
	 
	os.environ['PATH'] = sshfs_path + os.environ['PATH']
	cmd = '''start "" /b sshfs ganissa@vlcc32:/ S: 
		-o port=22 -o IdentityFile=D:/Users/ganissa/.ssh/id_rsa-ganissa-golddrive
		-o VolumePrefix=/sshfs/ganissa@vlcc32 -o FileSystemName=SSHFS
		-o StrictHostKeyChecking=no -o rellinks
		-f
		'''
	cmd = cmd.replace('\n',' ').replace('\r','').replace('\t','')
	 
	options = [
		'-o uid=1990801,gid=1049089',
		'-o uid=-1,gid=-1',
		'-o uid=-1,gid=99999',
		'-o uid=11111,gid=-1',
		'-o uid=19200,gid=103',
		'-o idmap=none',
		'-o idmap=none,uid=-1,gid=-1',
		'-o idmap=none,uid=-1,gid=99999',
		'-o idmap=none,uid=11111,gid=-1',
		'-o idmap=none,uid=19200,gid=103',
		'-o idmap=user',
		'-o idmap=user,uid=-1,gid=-1',
		'-o idmap=user,uid=-1,gid=99999',
		'-o idmap=user,uid=11111,gid=-1',
		'-o idmap=user,uid=19200,gid=103',
	]
	 
	for o in options:
		cmd1 = cmd + ' ' + o
		print(o)
		cp = subprocess.run(cmd1, shell=True)
		if cp.returncode != 0:
			print(cp)
		time.sleep(1)
	 
		cp = subprocess.run(fr'{fsptool} perm S:\red\ssd\usr\ganissa', capture_output=True,text=True)
		print(cp.stdout)
	 
		for p in psutil.process_iter(attrs=['name','cmdline']):
			if p.info['cmdline']:
				cmdline = ' '.join(p.info['cmdline'])
				if 'sshfs' in cmdline and ' S: ' in cmdline:
					p.terminate()
	 
 
# -o uid=-1,gid=-1 						O:S-1-5-21-245312111-887313111-311576111-942225G:DUD:P(A;;FA;;;S-1-5-21-245312111-887313111-311576111-942225)(A;;0x1200a9;;;DU)(A;;0x1200a9;;;WD) (perm=1990801:1049089:0755)
# -o uid=1990801,gid=1049089			O:S-1-5-21-245312111-887313111-311576111-942225G:DUD:P(A;;FA;;;S-1-5-21-245312111-887313111-311576111-942225)(A;;0x1200a9;;;DU)(A;;0x1200a9;;;WD) (perm=1990801:1049089:0755)
# -o idmap=none,uid=-1,gid=-1			O:S-1-5-21-245312111-887313111-311576111-942225G:DUD:P(A;;FA;;;S-1-5-21-245312111-887313111-311576111-942225)(A;;0x1200a9;;;DU)(A;;0x1200a9;;;WD) (perm=1990801:1049089:0755)
# -o idmap=user,uid=-1,gid=-1			O:S-1-5-21-245312111-887313111-311576111-942225G:DUD:P(A;;FA;;;S-1-5-21-245312111-887313111-311576111-942225)(A;;0x1200a9;;;DU)(A;;0x1200a9;;;WD) (perm=1990801:1049089:0755)

# -o uid=-1,gid=99999					O:S-1-5-21-245312111-887313111-311576111-942225G:S-1-5-24-1695D:P(A;;FA;;;S-1-5-21-245312111-887313111-311576111-942225)(A;;0x1200a9;;;S-1-5-24-1695)(A;;0x1200a9;;;WD) (perm=1990801:99999:0755)
# -o idmap=none,uid=-1,gid=99999		O:S-1-5-21-245312111-887313111-311576111-942225G:S-1-5-24-1695D:P(A;;FA;;;S-1-5-21-245312111-887313111-311576111-942225)(A;;0x1200a9;;;S-1-5-24-1695)(A;;0x1200a9;;;WD) (perm=1990801:99999:0755)
# -o idmap=user,uid=-1,gid=99999 		O:S-1-5-21-245312111-887313111-311576111-942225G:S-1-5-24-1695D:P(A;;FA;;;S-1-5-21-245312111-887313111-311576111-942225)(A;;0x1200a9;;;S-1-5-24-1695)(A;;0x1200a9;;;WD) (perm=1990801:99999:0755)

# -o uid=11111,gid=-1					O:S-1-5-2-2919G:DUD:P(A;;FA;;;S-1-5-2-2919)(A;;0x1200a9;;;DU)(A;;0x1200a9;;;WD) (perm=11111:1049089:0755)
# -o idmap=none,uid=11111,gid=-1		O:S-1-5-2-2919G:DUD:P(A;;FA;;;S-1-5-2-2919)(A;;0x1200a9;;;DU)(A;;0x1200a9;;;WD) (perm=11111:1049089:0755)
# -o idmap=user,uid=11111,gid=-1 		O:S-1-5-2-2919G:DUD:P(A;;FA;;;S-1-5-2-2919)(A;;0x1200a9;;;DU)(A;;0x1200a9;;;WD) (perm=11111:1049089:0755)

# -o uid=19200,gid=103			 		O:S-1-5-4-2888G:S-1-5-103D:P(A;;FA;;;S-1-5-4-2888)(A;;0x1200a9;;;S-1-5-103)(A;;0x1200a9;;;WD) (perm=19200:103:0755)
# -o idmap=none							O:S-1-5-4-2888G:S-1-5-103D:P(A;;FA;;;S-1-5-4-2888)(A;;0x1200a9;;;S-1-5-103)(A;;0x1200a9;;;WD) (perm=19200:103:0755)
# -o idmap=none,uid=19200,gid=103		O:S-1-5-4-2888G:S-1-5-103D:P(A;;FA;;;S-1-5-4-2888)(A;;0x1200a9;;;S-1-5-103)(A;;0x1200a9;;;WD) (perm=19200:103:0755)
# -o idmap=user,uid=19200,gid=103		O:S-1-5-4-2888G:S-1-5-103D:P(A;;FA;;;S-1-5-4-2888)(A;;0x1200a9;;;S-1-5-103)(A;;0x1200a9;;;WD) (perm=19200:103:0755)

# -o idmap=user							O:S-1-5-21-245312111-887313111-311576111-942225G:S-1-5-103D:P(A;;FA;;;S-1-5-21-245312111-887313111-311576111-942225)(A;;0x1200a9;;;S-1-5-103)(A;;0x1200a9;;;WD) (perm=1990801:103:0755)


# > "c:\Program Files (x86)\WinFsp\bin\fsptool-x64.exe" id
# User=S-1-5-21-245312111-887313111-311576111-942225(DOMAIN\user) (uid=1990801)
# Owner=S-1-5-21-245312111-887313111-311576111-942225(DOMAIN\user) (uid=1990801)
# Group=S-1-5-21-245312111-887313111-311576111-513(DOMAIN\Domain Users) (gid=1049089)

# > "c:\Program Files (x86)\WinFsp\bin\fsptool-x64.exe" perm s:\tmp
# O:S-1-5-21-245312111-887313111-311576111-942225G:DUD:P(A;;FA;;;S-1-5-21-245312111-887313111-311576111-942225)(A;;0x1201af;;;DU)(A;;0x1201af;;;WD) (perm=1990801:1049089:1777)

# > "C:\Program Files (x86)\WinFsp\bin\fsptool-x64.exe" id
# User=S-1-5-21-245312057-887313113-311576111-1063052(DOMAIN\user) (uid=2111628)
# Owner=S-1-5-21-245312057-887313113-311576111-1063052(DOMAIN\user) (uid=2111628)
# Group=S-1-5-21-245312057-887313113-311576111-513(DOMAIN\Domain Users) (gid=1049089)

# > "C:\Program Files (x86)\WinFsp\bin>fsptool-x64.exe" perm S:\tmp\file.txt
# O:S-1-0-65534G:DUD:P(A;;0x1f01bf;;;S-1-0-65534)(A;;0x1200a9;;;DU)(A;;0x120088;;;WD) (perm=65534:1049089:0750)

# > C:\Program Files (x86)\WinFsp\bin>del "s:\tmp\file.txt"
# s:\tmp\file.txt
# Access is denied.

if __name__ == '__main__':
	main()