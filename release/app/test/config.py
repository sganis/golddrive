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

js 			= util.loadConfig(fr'{DIR}\..\..\config.json')
userhost 	= f'{user}@{host}'
sshdir 		= os.path.expandvars("%USERPROFILE%")
appkey 		= util.getAppKey(user)
appkey_bak 	= fr'{appkey}.pytest_backup'
userkey 	= util.getUserKey()
# sshfs 	= fr'C:\Program Files\SSHFS-Win\bin\sshfs.exe'
# ssh 		= fr'C:\Program Files\SSHFS-Win\bin\ssh.exe'
path 		= js['sshfs_path']
os.environ['PATH'] += f';{path}'
sshfs 		= fr'{path}\sshfs.exe'
ssh 		= fr'{path}\ssh.exe'
drive 		= 'Y:'
