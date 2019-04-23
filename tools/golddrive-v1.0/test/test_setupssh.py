#!/usr/bin/env python

import sys
import os
import pytest
import logging
from config import *
import util
import setupssh	

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

def test_login():
	rb = setupssh.testlogin(userhost, password, port)
	assert rb.returncode == util.ReturnCode.OK

def test_login_no_password():
	rb = setupssh.testlogin(userhost, '', port)
	assert rb.returncode == util.ReturnCode.BAD_LOGIN

def test_wrong_password():
	rb = setupssh.testlogin(userhost, 'badpass', port)
	assert rb.returncode == util.ReturnCode.BAD_LOGIN

def test_login_bad_host():
	rb = setupssh.testlogin(userhost+'bad', password, port)
	assert rb.returncode == util.ReturnCode.BAD_HOST
	
def test_login_bad_port():
	rb = setupssh.testlogin(userhost+'bad', password, 3333)
	assert rb.returncode == util.ReturnCode.BAD_HOST
	
def test_testssh():
	rb = setupssh.testssh(userhost, appkey, port)
	assert rb.returncode == util.ReturnCode.BAD_LOGIN
	
def test_testssh_invalid_key():
	if os.path.exists(appkey):		
		os.remove(appkey)	
	rb = setupssh.testssh(userhost, appkey, port)
	assert rb.returncode == util.ReturnCode.BAD_LOGIN

def test_setupssh_passord():
	if os.path.exists(appkey):		
		os.remove(appkey)	
	rb = setupssh.main(userhost, password, port)
	assert rb.returncode == util.ReturnCode.OK
	assert 'successfull' in rb.output

	rb = setupssh.testssh(userhost, appkey, port)
	assert rb.returncode == util.ReturnCode.OK

# @pytest.mark.skip(reason="no way of currently testing this")
# def test_setupssh_user_key():
# 	if os.path.exists(appkey):		
# 		os.remove(appkey)	
# 	rb = setupssh.testssh(userhost, appkey, port)
# 	assert rb.returncode == util.ReturnCode.BAD_LOGIN
# 	rb = setupssh.main(userhost, '', userkey, port)
# 	assert rb.returncode == util.ReturnCode.OK
# 	assert 'successfull' in rb.output
# 	rb = setupssh.testssh(userhost, appkey, port)
# 	assert rb.returncode == util.ReturnCode.OK

def test_generate_keys():
	rb = setupssh.generate_keys(appkey, userhost)
	assert rb.output

def test_has_app_keys():
	rb = setupssh.generate_keys(appkey, userhost)
	assert rb.output
	assert setupssh.has_app_keys(user)
	os.remove(appkey)
	assert not setupssh.has_app_keys(user)

def test_host():
	rb = setupssh.testhost(userhost, port)
	assert rb.returncode == util.ReturnCode.OK
	rb = setupssh.testhost('user@badhost', port)
	assert rb.returncode == util.ReturnCode.BAD_HOST
