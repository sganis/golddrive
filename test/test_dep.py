#!/usr/bin/env python

import sys
import os
import pytest
import logging
from config import *
import util

logging.basicConfig(level=logging.INFO)

def test_dependencies():
	r = util.run('where subst reg cmd notepad explorer wmic taskkill ssh sshfs',
		capture=True)
	assert not r.stderr 