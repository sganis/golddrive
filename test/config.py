#!/usr/bin/env python

import sys
import os
DIR = os.path.dirname(os.path.realpath(__file__))
sys.path.append(fr'{DIR}\..\golddrive\app')
import util
util.set_path()

password 	= os.environ.get('GOLDDRIVE_PASS', '')
host 		= os.environ.get('GOLDDRIVE_HOST', '')
user 		= os.environ.get('GOLDDRIVE_USER', '')
port  	 	= os.environ.get('GOLDDRIVE_PORT', '')
assert password  # env var 'GOLDDRIVE_PASS' empty, run setenv.bat

golddrive   = os.environ['GOLDDRIVE']
js 			= util.load_config(fr'{golddrive}\config.json')
assert js

userhost 	= f'{user}@{host}'
sshdir 		= os.path.expandvars("%USERPROFILE%")
appkey 		= util.get_app_key(user)
appkey_bak 	= fr'{appkey}.pytest_backup'
drive 		= 'Y:'


