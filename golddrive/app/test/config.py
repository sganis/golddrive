#!/usr/bin/env python

import sys
import os

DIR = os.path.dirname(os.path.realpath(__file__))
sys.path.append(fr'{DIR}\..')
import util

password 	= os.environ.get('GOLDDRIVE_PASS', '')
host 		= os.environ.get('GOLDDRIVE_HOST', '')
user 		= os.environ.get('GOLDDRIVE_USER', '')
port  	 	= os.environ.get('GOLDDRIVE_PORT', '')
assert password  # env var 'GOLDDRIVE_PASS' empty, run setenv.bat

golddrive   = fr'{DIR}\..\..'
os.environ['GOLDDRIVE'] = golddrive
js 			= util.loadConfig(fr'{golddrive}\config.json')
assert js

path 		= js['sshfs_path']
sshfs 		= fr'{path}\sshfs.exe'
ssh 		= fr'{path}\ssh.exe'
assert os.path.exists(sshfs)
assert os.path.exists(ssh)
os.environ['PATH'] += f';{path}'

userhost 	= f'{user}@{host}'
sshdir 		= os.path.expandvars("%USERPROFILE%")
appkey 		= util.getAppKey(user)
appkey_bak 	= fr'{appkey}.pytest_backup'
userkey 	= util.getUserKey()
# sshfs 	= fr'C:\Program Files\SSHFS-Win\bin\sshfs.exe'
# ssh 		= fr'C:\Program Files\SSHFS-Win\bin\ssh.exe'

drive 		= 'Y:'


