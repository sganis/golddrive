#!/usr/bin/python3
# -*- coding: utf-8 -*-
import time
import logging
import util
from PyQt5.QtCore import QObject, pyqtSignal, pyqtSlot, QThread

logger = logging.getLogger('ssh-drive')


class BackgroundWorker(QObject):

	workDone = pyqtSignal(str, util.ReturnBox)
	
	def __init__(self, slow=False):
		super().__init__()
		self.slow = slow

	def check_drive(sefl, p):
		rb = util.ReturnBox()
		import mounter	
		rb.drive_status = mounter.check_drive(p['drive'], p['userhost'])
		return rb

	def check_keys(self, p):
		import setupssh	
		ok = setupssh.testssh(p['ssh'], p['userhost'], 
							p['seckey'], p['port'])
		rb = util.ReturnBox()
		if ok:
			rb.work_status = util.WorkStatus.SUCCESS
		return rb

	def setup_keys(self, p):
		import setupssh	
		return setupssh.main(p['ssh'], p['userhost'], p['password'],
							p['seckey'], p['port'])

	def restart_explorer(self, p):
		import mounter
		mounter.restart_explorer()
		return util.ReturnBox('Explorer.exe was restarted','')

	def connect(self, p):
		import mounter		
		return mounter.mount(p['sshfs'],p['ssh'], p['drive'],
			 				p['userhost'], p['seckey'], p['port'], 
							p['drivename'].replace(' ','-'))
		
	def disconnect(self, p):
		import mounter		
		return mounter.unmount(p['drive'])
		
	def disconnect_all(self, p):
		import mounter		
		return mounter.unmount_all()
		
	def repair(self, p):
		import mounter		
		mounter.unmount(p['drive'])
		return mounter.mount(p['sshfs'],p['ssh'], p['drive'],
			 				p['userhost'], p['seckey'], p['port'], 
							p['drivename'].replace(' ','-'))
		
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