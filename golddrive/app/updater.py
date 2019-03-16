#!/usr/bin/python3
# -*- coding: utf-8 -*-
import os
import time
import re
import logging
import util
from PyQt5.QtCore import QObject, pyqtSignal, pyqtSlot, QThread

logger = logging.getLogger('golddrive')

class BackgroundWorker(QObject):	
	
	checkDone = pyqtSignal(bool)
	updateDone = pyqtSignal(bool)

	def __init__(self, update_url):
		super().__init__()
		self.update_url = update_url
		self.new_version = None

	def download(self):
		# download new version
		logger.info('Getting new app version...')
		root = f'{os.environ["GOLDDRIVE"]}\\..'
		zipfile = f'golddrive-v{ self.new_version }-x64.zip'
		file = f'{ self.update_url }\\{zipfile}'
		r = util.run(f'copy /y {file} {root}\\{zipfile}', capture=True)
		if r.stderr: return False

		# extract
		output = f'{root}\\v{ self.new_version }'
		r = util.run(f'7za x -y {root}\\{zipfile} -o{output}', capture=True)
		if r.stderr: return False

		# delete zip
		r = util.run(f'del /q {root}\\{zipfile}', capture=True)
		if r.stderr: return False
		
		return True

	def getLatest(self):
		versions = []
		if not os.path.isdir(self.update_url):
			return []
		for file in os.listdir(self.update_url):
			m = re.match(r'golddrive-v([0-9\.]+)-x64\.zip', file)
			if m:
				version = m.group(1)
				versions.append(version)
		versions = sorted(versions)
		if versions:
			self.new_version = versions[-1]
			return self.new_version
		return None

	def isAlreadyDownloaded(self):
		root = f'{os.environ["GOLDDRIVE"]}\\..'
		path = f'{root}\\v{self.new_version}'
		return os.path.exists(path)

	def check(self):
		# check for new update, download and extract
		logger.info('Checking for updates...')
		result = False
		current = util.get_app_version()
		self.new_version = self.getLatest()
		if self.new_version and self.new_version > current:
			logger.info(f'Version {self.new_version} available.')
			if not self.isAlreadyDownloaded():
				r = self.download()
				if not r:
					logger.error(f'Cannot downlowd new version: {r}')	
				else:
					result = True	# new vesion available		
			else:
				logger.info('New version already downloaded')
				result = True			
		time.sleep(1)
		self.checkDone.emit(result)

	def update(self):
		# update version file and restart app
		logger.info(f'Updating to { self.new_version }...')
		root = f'{os.environ["GOLDDRIVE"]}\\..'
		version_file = f'{root}\\version.txt'
		with open(version_file, 'wt') as w:
			w.write(self.new_version)
		util.run(fr'cmd /c {root}\v{str(self.new_version).strip()}\bin\update.bat', detach=True)


class Updater(QObject):	
	
	checkRequested = pyqtSignal()
	updateRequested = pyqtSignal()
	checkDone = pyqtSignal(bool)
	updateDone = pyqtSignal(bool)

	def __init__(self, update_url):
		super().__init__()
		self.update_url = update_url
		self.thread = QThread()
		self.worker = BackgroundWorker(update_url)
		self.worker.moveToThread(self.thread)
		self.checkRequested.connect(self.worker.check)
		self.updateRequested.connect(self.worker.update)
		self.worker.checkDone.connect(self.onCheckDone)
		self.worker.updateDone.connect(self.onUpdateDone)
		self.is_working = False
		self.thread.start()
	
	def stop(self):	
		self.thread.quit()
		self.thread.wait()

	def doCheck(self):
		self.is_working = True
		self.checkRequested.emit()

	@pyqtSlot(bool)
	def onCheckDone(self, result):
		self.checkDone.emit(result)
		self.is_working = False

	def doUpdate(self):
		'''update to latest version'''
		self.is_working = True
		self.updateRequested.emit()

	@pyqtSlot(bool)
	def onUpdateDone(self, result):
		self.updateDone.emit(result)
		self.is_working = False




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

