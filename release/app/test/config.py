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
userhost 	= f'{user}@{host}'
seckey 		= os.path.dirname(os.path.realpath(__file__)) + '\\rsa.sec'
seckey		= seckey.replace('\\','/') # ssh cygwin needs full path of key like c:/users/..
sshdir 		= os.path.expandvars("%USERPROFILE%")
idrsa 		= util.defaultKey(user)
idrsa_bak 	= fr'{idrsa}.pytest_backup'
# sshfs 		= fr'C:\Program Files\SSHFS-Win\bin\sshfs.exe'
# ssh 		= fr'C:\Program Files\SSHFS-Win\bin\ssh.exe'
path 		= os.path.realpath(fr'{DIR}\..\..\sshfs\bin')
os.environ['PATH'] += f';{path}'
sshfs 		= fr'{path}\sshfs.exe'
ssh 		= fr'{path}\ssh.exe'
drive 		= 'Y:'