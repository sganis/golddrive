#!/usr/bin/env python

import sys
import os
import pytest
import logging
sys.path.append(fr'{os.path.dirname(__file__)}\..')
import util
import setupssh	
from config import *

logging.basicConfig(level=logging.INFO)

def setup_module():
	if os.path.exists(appkey):		
		os.rename(appkey, appkey_bak)

def teardown_module():
	if os.path.exists(appkey_bak):	
		os.rename(appkey_bak, appkey)	

# def setup_function():
# 	if os.path.exists(appkey):		os.remove(appkey)	
# 	if os.path.exists(appkey): 		os.remove(appkey)	

def teardown_function():
	if os.path.exists(appkey): 		
		os.remove(appkey)	
	if os.path.exists(appkey):		
		os.remove(appkey)	

def test_login_password():
	r = setupssh.login_password(ssh, userhost, password, port)
	assert r == 0

def test_login_password_no_password():
	r = setupssh.login_password(ssh, userhost, '', port)
	assert r == 1

def test_wrong_password():
	r = setupssh.login_password(ssh, userhost, 'badpass', port)
	assert r == 1

def test_login_bad_host():
	r = setupssh.login_password(ssh, userhost+'bad', password, port)
	assert r == 2
	
def test_login_bad_port():
	r = setupssh.login_password(ssh, userhost+'bad', password, 3333)
	assert r == 2
	
def test_testssh():
	r = setupssh.testssh(ssh, userhost, appkey, port)
	assert r == 1
	
def test_testssh_invalid_key():
	if os.path.exists(appkey):		
		os.remove(appkey)	
	r = setupssh.testssh(ssh, userhost, appkey, port)
	assert r == 1

def test_setupssh_passord():
	if os.path.exists(appkey):		
		os.remove(appkey)	
	rb = setupssh.main(ssh, userhost, password, '', port)
	assert 'successfull' in rb.output

	r = setupssh.testssh(ssh, userhost, appkey, port)
	assert r == 0

def test_setupssh_user_key():
	if os.path.exists(appkey):		
		os.remove(appkey)	
	r = setupssh.testssh(ssh, userhost, appkey, port)
	assert r == 1
	rb = setupssh.main(ssh, userhost, '', userkey, port)
	assert 'successfull' in rb.output
	r = setupssh.testssh(ssh, userhost, appkey, port)
	assert r == 0

def test_generate_keys():
	rb = setupssh.generate_keys(appkey, userhost)
	assert rb.output

def test_has_app_keys():
	rb = setupssh.generate_keys(appkey, userhost)
	assert rb.output
	assert setupssh.has_app_keys(user)
	os.remove(appkey)
	assert not setupssh.has_app_keys(user)
	