#!/usr/bin/python3
# -*- coding: utf-8 -*-
import time
import logging
import getpass
import mounter
import setupssh
import util
from PyQt5.QtCore import QObject, pyqtSignal, pyqtSlot, QThread

logger = logging.getLogger('golddrive')


class BackgroundWorker(QObject):

	workDone = pyqtSignal(str, util.ReturnBox)
	
	def __init__(self, slow=False):
		super().__init__()
		self.slow = slow

	def check_drive(sefl, p):
		rb = util.ReturnBox()
		rb.drive_status = mounter.check_drive(p['drive'], p['userhost'],
												 p['client'])
		if (rb.drive_status == 'CONNECTED' 
			or rb.drive_status == 'DISCONNECTED'):
			rb.returncode = util.ReturnCode.OK
		return rb

	def mount(self, p):
		rb = mounter.mount(p['drive'], p['userhost'], p['appkey'], 
							p['client'], p['port'], p['drivename'], p['args'])
		if rb.returncode == util.ReturnCode.OK	and p['no_host']:
			util.addDriveConfig(**p)
		return rb

	def connect(self, p):
		rb = util.ReturnBox()
		# test drive status
		status = mounter.check_drive(p['drive'], p['userhost'], p['client'])
		if status != 'DISCONNECTED':
			rb.returncode = util.ReturnCode.BAD_DRIVE
			rb.error = f'{status}'
			return rb
		# test ssh with app key
		rb = setupssh.testssh(p['userhost'], p['appkey'], p['port'])
		if rb.returncode == util.ReturnCode.BAD_HOST:
			return rb

		if rb.returncode == util.ReturnCode.BAD_LOGIN:
			# test login with user keys
			current_user = getpass.getuser()
			if not current_user == p['user']:
				return rb
			# user is same as logged in user, try user keys	
			rb = setupssh.testssh( p['userhost'], p['userkey'], p['port'])
			if not rb.returncode == util.ReturnCode.OK:
				rb.returncode = util.ReturnCode.BAD_LOGIN
				rb.error = '' # do not show error, is password required
				return rb
			# setup ssh with user key
			rb = setupssh.main(p['userhost'],'',p['userkey'],p['port'])
			if not rb.returncode == util.ReturnCode.OK:
				rb.returncode = util.ReturnCode.BAD_SSH
				return rb

		assert rb.returncode == util.ReturnCode.OK
		# app keys are ok, mount
		return self.mount(p)

	def connect_login(self, p):
		# test login
		rb = setupssh.testlogin(p['userhost'], p['password'], p['port'])
		if not rb.returncode == util.ReturnCode.OK:
			return rb

		# setup ssh with password
		rb = setupssh.main(p['userhost'],p['password'],'',p['port'])
		if not rb.returncode == util.ReturnCode.OK:
			rb.returncode = util.ReturnCode.BAD_SSH
			return rb
		
		# app keys are ok, mount
		return self.mount(p)


		
	def disconnect(self, p):
		return mounter.unmount(p['drive'], p['client'])
		
	def disconnect_all(self, p):
		return mounter.unmount_all()
		
	def repair(self, p):
		mounter.unmount(p['drive'])
		return mounter.mount(p['drive'], p['userhost'], p['appkey'], 
								p['port'], p['drivename'])
	
	def restart_explorer(self, p):
		util.restart_explorer()
		return util.ReturnBox('Explorer.exe was restarted','')
		
	# slot decorator is optional, used here 
	# for documenting argument's type
	@pyqtSlot(str, dict)
	def work(self, task, param):
		if self.slow:
			time.sleep(1)
		if hasattr(self, task):
			rb = getattr(self, task)(param)
		else:
			rb = util.ReturnBox('','Not implemented')
		self.workDone.emit(task, rb)


class Worker(QObject):
	
	workRequested = pyqtSignal(str, dict)
	workDone = pyqtSignal(str, util.ReturnBox)

	def __init__(self, slow=False):
		super().__init__()
		self.thread = QThread()
		self.worker = BackgroundWorker(slow)
		self.worker.moveToThread(self.thread)
		self.workRequested.connect(self.worker.work)
		self.worker.workDone.connect(self.onWorkDone)
		self.is_working = False
		self.thread.start()
		
	def doWork(self, task, param):
		self.is_working = True
		self.workRequested.emit(task, param)

	def stop(self):	
		self.thread.quit()
		self.thread.wait()

	@pyqtSlot(str, util.ReturnBox)
	def onWorkDone(self, task, rb):
		self.workDone.emit(task, rb)
		self.is_working = False


def test():
	import sys
	from PyQt5.QtCore import QCoreApplication
	# quit with control-c
	import signal
	signal.signal(signal.SIGINT, signal.SIG_DFL)
	app = QCoreApplication([])
	w = Worker(slow=True)
	w.doWork('a task 1', dict())
	w.doWork('a task 2', dict())
	w.doWork('a task 3', dict())
	w.doWork('test_exit', dict())	
	sys.exit(app.exec_())
	


if __name__ == '__main__':
	test()