#!/usr/bin/env python

import sys
import os
DIR = os.path.dirname(os.path.realpath(__file__))
sys.path.append(fr'{DIR}\..\golddrive\app')
import util

password 	= os.environ.get('GOLDDRIVE_PASS', '')
host 		= os.environ.get('GOLDDRIVE_HOST', '')
user 		= os.environ.get('GOLDDRIVE_USER', '')
port  	 	= os.environ.get('GOLDDRIVE_PORT', '')
assert password  # env var 'GOLDDRIVE_PASS' empty, run setenv.bat

golddrive   = fr'{DIR}\..\golddrive'
os.environ['GOLDDRIVE'] = golddrive
js 			= util.loadConfig(fr'{golddrive}\config.json')
assert js

path 		= js['sshfs_path']
assert os.path.exists(fr'{path}\sshfs.exe')
assert os.path.exists(fr'{path}\ssh.exe')
os.environ['PATH'] = f"{path};{ os.environ['PATH'] }"

userhost 	= f'{user}@{host}'
sshdir 		= os.path.expandvars("%USERPROFILE%")
appkey 		= util.getAppKey(user)
appkey_bak 	= fr'{appkey}.pytest_backup'
userkey 	= util.getUserKey()
drive 		= 'Y:'


