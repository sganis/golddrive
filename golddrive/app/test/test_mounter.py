#!/usr/bin/env python

import sys
import os
import pytest
import logging
import time
sys.path.append(fr'{os.path.dirname(__file__)}\..')
import util
import setupssh
import mounter	
from config import *

logging.basicConfig(level=logging.INFO)

def setup_module():
	rb = setupssh.main(userhost, password, '', port)
	assert 'successfull' or 'OK' in rb.output
	rb = mounter.unmount(drive)
	assert rb.drive_status == 'DISCONNECTED'
	assert setupssh.has_app_keys(user)
	# create virtual drive
	util.run('subst x: c:\\temp')

def teardown_module():
	rb = mounter.unmount(drive)
	assert rb.drive_status == 'DISCONNECTED'
	util.run('subst x: /d')

# def setup_function():
# 	pass
# def teardown_function():
# 	pass

def mount():
	rb = mounter.mount(drive, userhost, appkey, port)
	assert rb.drive_status == 'CONNECTED' and not rb.error
	assert mounter.check_drive(drive, userhost) == 'CONNECTED'

def unmount():
	rb = mounter.unmount(drive)
	assert rb.drive_status == 'DISCONNECTED'
	assert mounter.check_drive(drive, userhost) == 'DISCONNECTED'

def test_mount_and_unmount():
	mount()
	unmount()

def test_mount_without_ssh():
	if os.path.exists(appkey):		
		os.remove(appkey)		
	rb = mounter.mount(drive, userhost, appkey, port)
	assert 'Connection reset by peer' in rb.error
	setup_module()

def test_mount_connected():
	mount()
	time.sleep(2)
	rb = mounter.mount(drive, userhost, appkey, port)
	assert 'CONNECTED' in rb.error
	unmount()


def test_drive_works():
	assert not mounter.drive_works(drive, userhost)
	mount()
	assert mounter.drive_works(drive, userhost)
	unmount()
	assert not mounter.drive_works(drive, userhost)
	
def test_check_drive():
	assert 'NOT SUPPORTED' == mounter.check_drive('C:', userhost)
	assert 'IN USE' == mounter.check_drive('X:', userhost)
	assert 'DISCONNECTED' == mounter.check_drive(drive, userhost)
	mount()
	assert 'CONNECTED' == mounter.check_drive(drive, userhost)
	unmount()
	assert 'DISCONNECTED' == mounter.check_drive(drive, userhost)
	