#!/usr/bin/python3
# -*- coding: utf-8 -*-
import sys
import os
import time
from PyQt5 import QtCore, QtGui
from PyQt5.QtWidgets import (QWidget, QLabel, QComboBox, 
	QApplication, QMainWindow, QMenu)
from app_ui import Ui_MainWindow

DIR = os.path.abspath(os.path.dirname(__file__))

class Worker(QtCore.QObject):

	workDone = QtCore.pyqtSignal(str)

	def restart_explorer(self, param):
		import unmount
		unmount.restart_explorer()
		return 'Explorer.exe was restarted.'

	def connect(self, param):
		import mount		
		# mount.main(sshfs, drive, userhost, drivename)
		return 'Drive is connected'

	def disconnect(self, param):
		import mount		
		# mount.main(sshfs, drive, userhost, drivename)
		return 'Drive is connected'

	def reconnect(self, param):
		import mount		
		# mount.main(sshfs, drive, userhost, drivename)
		return 'Drive is connected'

	# def setupssh(self, param):
	# 	import mount		
	# 	# mount.main(sshfs, drive, userhost, drivename)
	# 	return 'Drive is connected'

	def work(self, task, param):
		time.sleep(1)
		if hasattr(self, task):
			result = getattr(self, task)(param)
		else:
			result = 'Not implemented.'
		self.workDone.emit(result)


class Window(Ui_MainWindow, QMainWindow):
	
	workRequested = QtCore.pyqtSignal(str, str)

	def __init__(self):
		super().__init__()
		self.loaded = False
		self.setupUi(self)
		self.setWindowTitle('SSH Drive')
		app_icon = QtGui.QIcon()
		app_icon.addFile(f'{DIR}/images/icon_16.png', QtCore.QSize(16,16))
		app_icon.addFile(f'{DIR}/images/icon_32.png', QtCore.QSize(32,32))
		app_icon.addFile(f'{DIR}/images/icon_48.png', QtCore.QSize(48,48))
		QApplication.setWindowIcon(app_icon)
		# widgets = self.findChildren(QWidget)
		# for w in widgets:
		# 	w.setProperty('css',True)
		os.chdir(f'{DIR}')
		with open(f'{DIR}/style.css') as r:
			self.setStyleSheet(r.read())

		self.settings = QtCore.QSettings("sganis", "ssh-drive")
		if self.settings.value("geometry"):
			self.restoreGeometry(self.settings.value("geometry"))
			self.restoreState(self.settings.value("windowState"))
		self.cboParam.addItems(["   S:  Simulation","   I:  IT","   X:  Exploration"])
		self.cboParam.setCurrentIndex(self.settings.value("cboParam",0))
		self.progressBar.setVisible(False)	
		self.lblStatus.setVisible(False)

		# worker thread
		self.thread = QtCore.QThread()
		self.worker = Worker()
		self.worker.moveToThread(self.thread)
		self.workRequested.connect(self.worker.work)
		self.worker.workDone.connect(self.onWorkDone)
		self.thread.start()
		self.loaded = True
		
		menu = QMenu()
		menu.addAction('Setup SSH keys', self.mnuSetupSsh)
		menu.addAction('Reconnect', self.mnuReconnect)
		menu.addAction('Open settings folder', self.mnuSettings)
		menu.addAction('Show log file', self.mnuShowLog)
		menu.addAction('Restart Explorer.exe', self.mnuRestartExplorer)
		self.pbHamburger.setMenu(menu)

	def mnuSetupSsh(self):
		self.start('Setting up SSH keys...')
		self.workRequested.emit('setupssh','')

	def mnuReconnect(self):
		self.start('Reconnecting drive...')
		self.workRequested.emit('reconnect','')

	def mnuSettings(self):
		self.start('Open settings folder...')
		self.workRequested.emit('settings','')

	def mnuShowLog(self):
		self.start('Show log file...')
		self.workRequested.emit('show_log','')
	
	def mnuRestartExplorer(self):
		self.start('Restarting explorer...')
		self.workRequested.emit('restart_explorer','')
		
	def closeEvent(self, event):
		
		self.settings.setValue("geometry", self.saveGeometry())
		self.settings.setValue("windowState", self.saveState())
		self.settings.setValue("cboParam", self.cboParam.currentIndex())	
		QMainWindow.closeEvent(self, event)
		self.thread.quit()
		self.thread.wait()

	def start(self, message):

		self.progressBar.setVisible(True)
		self.lblStatus.setText(message)
		self.lblStatus.setVisible(True)
		self.pbConnect.setEnabled(False)

	def end(self, message='Done.'):

		self.lblStatus.setText(message)
		self.progressBar.setVisible(False)
		self.pbConnect.setEnabled(True)

	@QtCore.pyqtSlot(int) # only trigger once, not in @QtCore.pyqtSlot(str)
	def on_cboParam_currentIndexChanged(self, e):
		if self.loaded:
			print(f'index changed: {e}')
			param = self.cboParam.currentText();
			self.start('Checking...')
			self.workRequested.emit('get_drive_status', param)
	
	def on_pbConnect_released(self):
		param = self.cboParam.currentText();
		self.start('Checking...')
		self.workRequested.emit('connect', param)

	def onWorkDone(self, result):
		print(f'work done, result: {result}')
		self.end(result)

	def on_lblSettings_linkActivated(self, link):
		os.system(fr'start /b c:\windows\explorer.exe "{DIR}\.."')

def run():

    app = QApplication(sys.argv)
    window = Window()
    window.show()
    sys.exit(app.exec_())



if __name__ == '__main__':
    run()    
