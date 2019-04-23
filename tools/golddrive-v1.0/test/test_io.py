# #!/usr/bin/env python

# import sys
# import os
# import pytest
# import logging
# import time
# import hashlib
# import multiprocessing as mp 
# import asyncio

# sys.path.append(fr'{os.path.dirname(__file__)}\..')
# import util
# # import setupssh
# # import mounter	
# from config import *

# logging.basicConfig(level=logging.INFO)

# files = {}
# MB = 1024*1024
# SMALL_FILES = 10
# SIZE = (100*MB+SMALL_FILES*MB)/1024/1024

# # drive = 'v:'

# def create_file(fname, size):
# 	with open(fname, 'wb') as w:
# 		w.write(os.urandom(size))
	
# def md5sum(fname):
# 	hash_md5 = hashlib.md5()
# 	with open(fname, "rb") as f:
# 		for chunk in iter(lambda: f.read(4096), b""):
# 			hash_md5.update(chunk)
# 	return hash_md5.hexdigest()

# def setup_module():
# 	# create 10 small files
# 	folder = fr'{DIR}\temp'
# 	if os.path.exists(folder):
# 		util.run(f'rd /s /q {folder}')
		
# 	os.makedirs(folder)
# 	for i in range(SMALL_FILES):
# 		fname = f'file_{i}.bin'
# 		fpath = fr'{folder}\{fname}'
# 		create_file(fpath, 1*MB)
# 		files[fname] = md5sum(fpath)

# 	# create 1 big file 
# 	fname = f'file_big.bin'
# 	fpath = fr'{folder}\{fname}'
# 	# create_file(fpath, 1024*MB)
# 	create_file(fpath, 100*MB)
# 	files[fname] = md5sum(fpath) 
# 	for fname in files:
# 		print(fname, files[fname])

# def teardown_module():
# 	pass
# 	# for fname in files:
# 	# 	os.remove(fr'{drive}\tmp\{fname}')
# 	# 	os.remove(fr'{DIR}\temp\{fname}')

# # def setup_function():
# # 	pass

# # def teardown_function():
# # 	pass

# def copy(args):
# 	util.run(f'copy {args[0]} {args[1]}')

# def test_put_and_get():

# 	if not os.path.exists(fr'{drive}\tmp'):
# 		os.makedirs(fr'{drive}\tmp')
# 	tasks1 = []
# 	tasks2 = []
# 	t = time.time()
# 	for fname in files:
# 		task = [fr'{DIR}\temp\{fname}', fr'{drive}\tmp\{fname}']
# 		tasks1.append(task)
# 		copy(task)
		
# 	elapsed = time.time()-t
# 	speed = round(SIZE/elapsed)
# 	print(f'put: {speed} MB/s')

# 	for fname in files:
# 		src = fr'{DIR}\temp\{fname}'
# 		os.remove(src)
		
# 	t = time.time()
# 	for fname in files:
# 		task = [fr'{drive}\tmp\{fname}', fr'{DIR}\temp\{fname}']
# 		tasks2.append(task)
# 		copy(task)

# 	elapsed = time.time()-t
# 	speed = round(SIZE/elapsed)
# 	print(f'get: {speed} MB/s')
	
# 	for fname in files:
# 		assert md5sum(fr'{DIR}\temp\{fname}') == files[fname]


# 	pool = mp.Pool()
# 	t = time.time()
# 	pool.map(copy, tasks1)
# 	elapsed = time.time()-t
# 	speed = round(SIZE/elapsed)
# 	print(f'put parallel: {speed} MB/s')

# 	t = time.time()
# 	pool.map(copy, tasks2)
# 	elapsed = time.time()-t
# 	speed = round(SIZE/elapsed)
# 	print(f'get parallel: {speed} MB/s')

	

# 	# async def factorial(name, number):
# 	# 	f = 1
# 	# 	for i in range(2, number+1):
# 	# 		print("Task %s: Compute factorial(%s)..." % (name, i))
# 	# 		await asyncio.sleep(1)
# 	# 		f *= i
# 	# 	print("Task %s: factorial(%s) = %s" % (name, number, f))

# 	# loop = asyncio.get_event_loop()
# 	# loop.run_until_complete(asyncio.gather(
# 	# 	copy(tasks1),
# 	# 	factorial("B", 3),
# 	# 	factorial("C", 4),
# 	# ))
# 	# loop.close()


# 	for fname in files:
# 		assert md5sum(fr'{DIR}\temp\{fname}') == files[fname]


