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
	rb = setupssh.main(ssh, userhost, password, seckey, port)
	assert 'successfull' or 'OK' in rb.output
	rb = mounter.unmount(drive)
	assert rb.drive_status == 'DISCONNECTED'

def teardown_module():
	if os.path.exists(seckey):		
		os.remove(seckey)	
	rb = mounter.unmount(drive)
	assert rb.drive_status == 'DISCONNECTED'

# def setup_function():
# 	pass
# def teardown_function():
# 	pass

def mount():
	rb = mounter.mount(sshfs, ssh, drive, userhost, seckey, port)
	assert rb.drive_status == 'CONNECTED' and not rb.error

def unmount():
	rb = mounter.unmount(drive)
	assert rb.drive_status == 'DISCONNECTED'

def test_mount_and_unmount():
	mount()
	unmount()

def test_mount_without_ssh():
	if os.path.exists(seckey):		
		os.remove(seckey)		
	rb = mounter.mount(sshfs, ssh, drive, userhost, seckey, port)
	assert 'authetication wrong' in rb.error
	setup_module()

def test_mount_in_use():
	mount()
	time.sleep(1)
	rb = mounter.mount(sshfs, ssh, drive, userhost, seckey, port)
	assert 'Drive in use' in rb.error
	unmount()

def test_drive_in_use():
	mount()
	assert mounter.drive_in_use(drive)
	unmount()
	assert not mounter.drive_in_use(drive)
	
def test_drive_is_golddrive():
	assert not mounter.drive_is_golddrive('C:', user, host)
	assert not mounter.drive_is_golddrive('E:', user, host)
	mount()
	assert mounter.drive_is_golddrive(drive, user, host)
	unmount()
	assert not mounter.drive_is_golddrive(drive, user, host)
	
def test_drive_works():
	assert not mounter.drive_works(drive, user)
	mount()
	assert mounter.drive_works(drive, user)
	unmount()
	assert not mounter.drive_works(drive, user)
	
def test_check_drive():
	# if not drive_in_use(drive):					return 'DISCONNECTED'
	# elif not drive_is_golddrive(drive, userhost):	return 'IN USE'
	# elif not drive_works(drive, user):			return 'BROKEN'
	# else:											return 'CONECTED' 
	assert 'NOT SUPPORTED' == mounter.check_drive('C:', user, host)
	assert 'DISCONNECTED' == mounter.check_drive(drive, user, host)
	mount()
	assert 'CONNECTED' == mounter.check_drive(drive, user, host)
	assert 'IN USE' == mounter.check_drive('E:', user, host)
	unmount()
	assert 'DISCONNECTED' == mounter.check_drive(drive, user, host)
	