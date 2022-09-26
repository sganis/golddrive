# patch openssh-portable windows to build with openssl
import os

DIR = os.path.dirname(os.path.realpath(__file__))


# add these defines
#pragma warning(disable: 4005 4030)
#define _CRT_INTERNAL_NONSTDC_NAMES 0
path = r'contrib\win32\win32compat\inc\unistd.h'
with open(path) as r:
	lines = r.readlines()
done = False
for line in lines:
	if '#define _CRT_INTERNAL_NONSTDC_NAMES 0' in line:
		done = True
		break
if not done:
	with open(path, 'wt') as w:
		for line in lines:
			if '#define STDIN_FILENO 0' in line:
				w.write('#pragma warning(disable: 4005 4030)\n')
				w.write('#define _CRT_INTERNAL_NONSTDC_NAMES 0\n')
			w.write(line)

# add openssl and zlib 
path = r'contrib\win32\openssh\paths.targets'
with open(path) as r:
	lines = r.readlines()
with open(path, 'wt') as w:
	for line in lines:
		# if '<LibreSSL-Path>' in line:
		# 	w.write(f'\t<LibreSSL-Path>{DIR}\\..\\vendor\\openssl\\</LibreSSL-Path>\n')
		# elif '<LibreSSL-x64-Path>' in line:
		# 	w.write(f'\t<LibreSSL-x64-Path>{DIR}\\..\\vendor\\openssl\\lib\\x64\\</LibreSSL-x64-Path>\n')
		# elif '<LibreSSL-x86-Path>' in line:
		# 	w.write(f'\t<LibreSSL-x86-Path>{DIR}\\..\\vendor\\openssl\\lib\\x86\\</LibreSSL-x86-Path>\n')
		if '<ZLib-Path>' in line:
			w.write(f'\t<ZLib-Path>{DIR}\\..\\vendor\\zlib\\include\\</ZLib-Path>\n')
		elif '<ZLib-x64-Path>' in line:
			w.write(f'\t<ZLib-x64-Path>{DIR}\\..\\vendor\\zlib\\lib\\x64\\</ZLib-x64-Path>\n')
		elif '<ZLib-x86-Path>' in line:
			w.write(f'\t<ZLib-x86-Path>{DIR}\\..\\vendor\\zlib\\lib\\x86\\</ZLib-x86-Path>\n')
		# elif '<SSLLib>' in line:
		# 	w.write('\t<SSLLib>libeay32.lib;</SSLLib>\n')
		elif '<WindowsSDKVersion>' in line:
			w.write('\t<WindowsSDKVersion>10</WindowsSDKVersion>\n')
		else:
			w.write(line)

# comment out download of libressl and zlib
# path = r'contrib\win32\openssh\config.vcxproj'
# with open(path) as r:
# 	lines = r.readlines()
# with open(path, 'wt') as w:
# 	for line in lines:
# 		if 'powershell.exe' in line and 'rem' not in line:
# 			w.write('rem ' + line)
# 		else:
# 			w.write(line)

# # replace libcrypto.lib in ssh-keygen 32-bit release
# path = r'contrib\win32\openssh\keygen.vcxproj'
# with open(path) as r:
# 	data = r.read()
# with open(path, 'wt') as w:
# 	w.write(data.replace('libcrypto.lib','$(SSLLib)'))

# replace zlib.lib by zlibstatic.lib
path = r'contrib\win32\openssh\ssh.vcxproj'
with open(path) as r:
	data = r.read()
with open(path, 'wt') as w:
	w.write(data.replace('zlib.lib','zlibstatic.lib'))

# ssh 32-bit needs /nodefaultlib:msvcrt.lib linker option
path = r'contrib\win32\openssh\ssh.vcxproj'
with open(path) as r:
	lines = r.readlines()
with open(path, 'wt') as w:
	link=False
	for line in lines:
		if '<Link>' in line: link=True
		if '</Link>' in line: link=False
		if link and '<AdditionalOptions>' in line:
			w.write('      <AdditionalOptions>/incremental:no /ignore:4099 /nodefaultlib:msvcrt.lib %(AdditionalOptions)</AdditionalOptions>\n')
		else:
			w.write(line)


    