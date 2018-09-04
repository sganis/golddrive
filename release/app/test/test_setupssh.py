#!/usr/bin/env python

import sys
import os
sys.path.append(fr'{os.path.dirname(__file__)}\..')
import setupssh	
import util
import pytest
import getpass

host 		= '192.168.100.201'
assert os.environ.get('PASS', '') # password empty, set PASS=<password>

user 		= getpass.getuser()
userhost 	= f'{user}@{host}'
seckey 		= 'rsa.sec'
port 		= 22
sshdir 		= os.path.expandvars("%USERPROFILE%")
idrsa 		= util.defaultKey()
idrsa_bak 	= fr'{idrsa}.pytest_backup'
ssh 		= fr'C:\Program Files\SSHFS-Win\bin\ssh.exe'

def setup_module():
	if os.path.exists(idrsa):		os.rename(idrsa, idrsa_bak)

def teardown_module():
	if os.path.exists(idrsa_bak):	os.rename(idrsa_bak, idrsa)	

def setup_function():
	if os.path.exists(seckey):		os.remove(seckey)	
	if os.path.exists(idrsa_bak): 	os.remove(idrsa_bak)	

def teardown_function():
	if os.path.exists(seckey): 		os.remove(seckey)	
	if os.path.exists(idrsa):		os.remove(idrsa)	

def test_testssh_no_key():
	ok = setupssh.testssh(ssh, userhost, seckey, port)
	assert not ok
	
def test_empty_password():
	rb = setupssh.main(ssh, user, host, '', seckey, port)
	assert 'password is empty' in rb.error

def test_wrong_password():
	rb = setupssh.main(ssh, user, host, 'secret', seckey, port)
	assert 'password wrong'  in rb.error

def test_bad_host():
	rb = setupssh.main(ssh, user, 'unknown', 'secret', seckey, port)
	assert 'not found' in rb.error

def test_setupssh_with_key():
	password = os.environ.get('PASS', '')
	assert password
	rb = setupssh.main(ssh, user, host, password, seckey, port)
	print (rb.output)
	assert 'successfull' in rb.output

	rb = setupssh.main(ssh, user, host, password, seckey, port)
	print (rb.output)
	assert 'authentication is OK' in rb.output

	ok = setupssh.testssh(ssh, userhost, seckey, port)
	assert ok

def test_setupssh_default_key():
	password = os.environ.get('PASS', '')
	assert password
	rb = setupssh.main(ssh, user, host, password, '', port)
	print (rb.output)
	assert 'successfull' in rb.output

	rb = setupssh.main(ssh, user, host, password, '', port)
	print (rb.output)
	assert 'authentication is OK' in rb.output

	ok = setupssh.testssh(ssh, userhost, '', port)
	assert ok
