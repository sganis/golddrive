#!/usr/bin/python3
# -*- coding: utf-8 -*-
import time
import logging
from util import ReturnBox
from PyQt5.QtCore import QObject, pyqtSignal, pyqtSlot, QThread

logger = logging.getLogger('ssh-drive')


class BackgroundWorker(QObject):

	workDone = pyqtSignal(str, ReturnBox)
	
	def __init__(self, slow=False):
		super().__init__()
		self.slow = slow

	def get_status(sefl, p):
		rb = ReturnBox()
		if p['drive'] == 'Z:':
			rb.drive_state = 'CONNECTED'
		if p['drive'] == 'X:':
			rb.drive_state = 'UNSTABLE STATE'
			rb.error = rb.drive_state
		rb.output = f"{rb.drive_state}"
		return rb

	def testssh(self, p):
		import setupssh	
		ok = setupssh.testssh(p['ssh'], p['userhost'], 
							p['seckey'], p['port'])
		rb = ReturnBox()
		if ok:
			rb.output = "SSH authentication is OK"
		return rb

	def setupssh(self, p):
		import setupssh	
		return setupssh.main(p['ssh'], p['userhost'], p['password'],
							p['seckey'], p['port'])

	def restart_explorer(self, p):
		import mount
		mount.restart_explorer()
		return ReturnBox('Explorer.exe was restarted.','')

	def connect(self, p):
		import mount		
		return mount.mount(p['sshfs'],p['ssh'], p['drive'],
			 				p['userhost'], p['seckey'], p['port'], 
							p['drivename'].replace(' ','-'))
		
	def disconnect(self, p):
		import mount		
		return mount.unmount(p['drive'])
		
	def reconnect(self, p):
		# import remount		
		# remount.main(sshfs, drive, userhost, drivename)
		msg =  'Drive is connected'
		return ReturnBox('','Not implemented')

	# slot decorator is optional, used here 
	# for documenting argument's type
	@pyqtSlot(str, dict)
	def work(self, task, param):
		if self.slow:
			time.sleep(1)
		if hasattr(self, task):
			rb = getattr(self, task)(param)
		else:
			rb = ReturnBox('','Not implemented')
		self.workDone.emit(task, rb)


class Worker(QObject):
	
	workRequested = pyqtSignal(str, dict)
	workDone = pyqtSignal(str, ReturnBox)

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
		# print(f'work requested: {task}, param: {param}')
		self.workRequested.emit(task, param)

	def stop(self):	
		self.thread.quit()
		self.thread.wait()
		print('worker was stopped.')

	@pyqtSlot(str, ReturnBox)
	def onWorkDone(self, task, rb):
		# print(f'{task}, result: {rb.output}, error: {rb.error}')
		self.workDone.emit(task, rb)
		self.is_working = False
		if task == 'test_exit':
			self.stop()
			from PyQt5.QtCore import QCoreApplication
			print('exiting application...')
			QCoreApplication.quit()

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