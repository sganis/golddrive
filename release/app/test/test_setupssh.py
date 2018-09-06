#!/usr/bin/env python

import sys
import os
sys.path.append(fr'{os.path.dirname(__file__)}\..')
import setupssh	
import util
import pytest
import getpass


password 	= os.environ.get('GOLDDRIVE_PASS', '')
host 		= os.environ.get('GOLDDRIVE_HOST', '')
user 		= os.environ.get('GOLDDRIVE_USER', '')
port  	 	= os.environ.get('GOLDDRIVE_PORT', '')
assert password  # env var 'GOLDDRIVE_PASS' empty, run setenv.bat
userhost 	= f'{user}@{host}'
seckey 		= 'rsa.sec'
sshdir 		= os.path.expandvars("%USERPROFILE%")
idrsa 		= util.defaultKey(user)
idrsa_bak 	= fr'{idrsa}.pytest_backup'
ssh 		= fr'C:\Program Files\SSHFS-Win\bin\ssh.exe'

def setup_module():
	if os.path.exists(idrsa):		os.rename(idrsa, idrsa_bak)

def teardown_module():
	if os.path.exists(idrsa_bak):	os.rename(idrsa_bak, idrsa)	

# def setup_function():
# 	if os.path.exists(seckey):		os.remove(seckey)	
# 	if os.path.exists(idrsa): 		os.remove(idrsa)	

def teardown_function():
	if os.path.exists(seckey): 		os.remove(seckey)	
	if os.path.exists(idrsa):		os.remove(idrsa)	

def test_testssh_no_key():
	if os.path.exists(seckey):		
		os.remove(seckey)	
	ok = setupssh.testssh(ssh, userhost, seckey, port)
	assert not ok
	
def test_empty_password():
	rb = setupssh.main(ssh, userhost, '', seckey, port)
	assert 'password is empty' in rb.error

def test_wrong_password():
	rb = setupssh.main(ssh, userhost, 'secret', seckey, port)
	assert 'password wrong'  in rb.error

def test_bad_host():
	rb = setupssh.main(ssh, f'{user}@unknown', 'secret', seckey, port)
	assert 'not found' in rb.error

def test_setupssh_with_key():
	rb = setupssh.main(ssh, userhost, password, seckey, port)
	print (rb.output)
	assert 'successfull' in rb.output

	rb = setupssh.main(ssh, userhost, password, seckey, port)
	print (rb.output)
	assert 'authentication is OK' in rb.output

	ok = setupssh.testssh(ssh, userhost, seckey, port)
	assert ok
	os.remove(seckey)

def test_setupssh_default_key():
	rb = setupssh.main(ssh, userhost, password, '', port)
	print (rb.output)
	assert 'successfull' in rb.output

	rb = setupssh.main(ssh, userhost, password, '', port)
	print (rb.output)
	assert 'authentication is OK' in rb.output

	ok = setupssh.testssh(ssh, userhost, '', port)
	assert ok
	os.remove(idrsa)

