#!/usr/bin/env python

import sys
import os
import pytest
import logging
import time
import hashlib
import multiprocessing as mp 
import asyncio
from config import *
import util
import mounter
import setupssh

logging.basicConfig(level=logging.ERROR)

files = {}
BYTES = 1024*1024*100
remot_dir = fr'{drive}\tmp'
local_dir = fr'{DIR}\temp'

def create_file(fname, size):
	with open(fname, 'wb') as w:
		w.write(os.urandom(size))
	
def md5sum(fname):
	hash_md5 = hashlib.md5()
	with open(fname, "rb") as f:
		for chunk in iter(lambda: f.read(4096), b""):
			hash_md5.update(chunk)
	return hash_md5.hexdigest()

def setup_module():
	rb = setupssh.main(userhost, password, '', port)
	assert 'successfull' or 'OK' in rb.output
	rb = mounter.unmount(drive)
	assert rb.drive_status == 'DISCONNECTED'
	assert setupssh.has_app_keys(user)
	rb = mounter.mount(drive, userhost, appkey, port)
	assert rb.drive_status == 'CONNECTED' and not rb.error
	time.sleep(1)
	assert mounter.check_drive(drive, userhost) == 'CONNECTED'

	if os.path.exists(local_dir):
		util.run(f'rd /s /q {local_dir}')
	os.makedirs(local_dir)
	fname = f'file_big.bin'
	fpath = fr'{local_dir}\{fname}'
	create_file(fpath, BYTES)
	files[fname] = md5sum(fpath) 
	for fname in files:
		print(fname, files[fname])

def teardown_module():
	for fname in files:
		os.remove(fr'{local_dir}\{fname}')
	os.rmdir(local_dir)
	
	rb = mounter.unmount(drive)
	assert rb.drive_status == 'DISCONNECTED'


# def robocopy(args):
# 	# util.run(f'copy {args[0]} {args[1]}', timeout=300)
# 	util.run(f'robocopy {os.path.dirname(args[0])} {os.path.dirname(args[1])} file_big.bin /njh /njs /ndl /nc /ns', timeout=300)

def copy(args):
	util.run(f'copy {args[0]} {args[1]}', timeout=300)
	
def test_copy():

	# if not os.path.exists(remot_dir):
	# 	os.makedirs(remot_dir)

	t = time.time()
	for fname in files:
		copy([fr'{local_dir}\{fname}', fr'{remot_dir}\{fname}'])
	print(f'write: { round(BYTES/1024/1024/(time.time()-t)) } MB/s')

	for fname in files:
		os.remove(fr'{local_dir}\{fname}')
		
	t = time.time()
	for fname in files:
		copy([fr'{remot_dir}\{fname}', fr'{local_dir}\{fname}'])
	print(f'read : { round(BYTES/1024/1024/(time.time()-t)) } MB/s')
	
	for fname in files:
		assert md5sum(fr'{local_dir}\{fname}') == files[fname]
		os.remove(fr'{remot_dir}\{fname}')

	# t = time.time()
	# for fname in files:
	# 	robocopy([fr'{local_dir}\{fname}', fr'{remot_dir}\{fname}'])
	# print(f'write: { round(BYTES/1024/1024/(time.time()-t)) } MB/s')

	# for fname in files:
	# 	os.remove(fr'{local_dir}\{fname}')
		
	# t = time.time()
	# for fname in files:
	# 	robocopy([fr'{remot_dir}\{fname}', fr'{local_dir}\{fname}'])
	# print(f'read : { round(BYTES/1024/1024/(time.time()-t)) } MB/s')
	
	# for fname in files:
	# 	assert md5sum(fr'{local_dir}\{fname}') == files[fname]
	# 	os.remove(fr'{remot_dir}\{fname}')


	# pool = mp.Pool()
	# t = time.time()
	# pool.map(copy, tasks1)
	# elapsed = time.time()-t
	# speed = round(SIZE/elapsed)
	# print(f'write parallel: {speed} BYTES/s')

	# t = time.time()
	# pool.map(copy, tasks2)
	# elapsed = time.time()-t
	# speed = round(SIZE/elapsed)
	# print(f'read parallel : {speed} BYTES/s')

	

	# async def factorial(name, number):
	# 	f = 1
	# 	for i in range(2, number+1):
	# 		print("Task %s: Compute factorial(%s)..." % (name, i))
	# 		await asyncio.sleep(1)
	# 		f *= i
	# 	print("Task %s: factorial(%s) = %s" % (name, number, f))

	# loop = asyncio.get_event_loop()
	# loop.run_until_complete(asyncio.gather(
	# 	copy(tasks1),
	# 	factorial("B", 3),
	# 	factorial("C", 4),
	# ))
	# loop.close()

