#!/usr/bin/env python
import sys
import os
import pytest
import logging
import re
sys.path.append(fr'{os.path.dirname(__file__)}\..')
import util

logging.basicConfig(level=logging.INFO)

ipaddr = r'(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])'
hostname = r'(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\-]*[a-zA-Z0-9])\.)*([A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9\-]*[A-Za-z0-9])'
pattern_host = fr'^{hostname}|{ipaddr}$'
regex = re.compile(pattern_host)

def validate(text):
	userhostport = text
	userhost = text
	host = text
	port = None
	user = None
	if ':' in text:
		userhost, port = text.split(':')
		host = userhost
	if '@' in userhost:
		user, host = userhost.split('@')
	
	if regex.match(host):
		return 'bad host'

	if user:
		pass
	if port:
		try:
			port = int(port)
			if port < 0 or port > 65635:
				raise
		except:
			return 'bad port'

def test_remove():

	assert regex.match("192.168.1.1")
	assert regex.match("localhost")
	assert regex.match("localhost.com")
	assert regex.match("localhost.com.cc")
	assert regex.match("localhost:22")
	assert regex.match("localhost:2222")
	assert regex.match("user@localhost:22")
	assert regex.match("user@localhost.com:655354")



