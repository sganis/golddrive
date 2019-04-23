#!/usr/bin/env python

import sys
import os
import pytest
import logging
from config import *
import util

logging.basicConfig(level=logging.INFO)

def test_dependencies():
	r = util.run('where subst cmd notepad explorer ssh sshfs',
		capture=True)
	assert not r.stderr 