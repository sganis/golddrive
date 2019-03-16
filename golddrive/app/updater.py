#!/usr/bin/python3
# -*- coding: utf-8 -*-
import time
import logging
import util
from PyQt5.QtCore import QThread, pyqtSignal

logger = logging.getLogger('golddrive')

def download_latest():
	# download new version
	logger.info('Getting new app version...')

	# extract

def get_latest():
	return None

class UpdateChecker(QThread):	
	
	checkUpdateDone = pyqtSignal(util.ReturnBox)

	def run(self):
		# check for new update, download and extract
		logger.info('Checking for updates...')
		time.sleep(5)
		rb = util.ReturnBox()
		current = util.get_app_version()
		latest = get_latest()
		if latest and latest > current:
			logger.info(f'Version {latest} available.')
			download_latest()
		
		self.checkUpdateDone.emit(rb)


def update():
	'''update to latest version'''
	logger.info('Updating...')
	latest = get_latest() 
	# util.run(os.path.expandvars(fr'%GOLDDRIVE%\..\{latest}\bin\update.bat', detach=True)





# def test():
# 	import sys
# 	from PyQt5.QtCore import QCoreApplication
# 	# quit with control-c
# 	import signal
# 	signal.signal(signal.SIGINT, signal.SIG_DFL)
# 	app = QCoreApplication([])
# 	u = Updater()
# 	u.run()
# 	sys.exit(app.exec_())
	


# if __name__ == '__main__':
# 	test()

