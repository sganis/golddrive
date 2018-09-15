#!/usr/bin/env python

import sys
import os
import pytest
import logging
sys.path.append(fr'{os.path.dirname(__file__)}\..')
import util
from config import *

logging.basicConfig(level=logging.INFO)

def test_dependencies():
	r = util.run('where subst reg cmd notepad explorer wmic taskkill ssh sshfs',
		capture=True)
	assert not r.stderr 