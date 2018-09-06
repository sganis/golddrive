#!/usr/bin/python3
# -*- coding: utf-8 -*-
import sys
import os
import time
import json
import subprocess
import getpass
import logging
import util
from worker import Worker
from PyQt5 import uic
from PyQt5.QtCore import (QObject, pyqtSlot, 
	QThread, QSize, QSettings, QFileSystemWatcher)
from PyQt5.QtGui import QIcon
from PyQt5.QtWidgets import (QWidget, QLabel,
	QComboBox, QApplication, QMainWindow, QMenu)
# from app_ui import Ui_MainWindow


DIR = os.path.abspath(os.path.dirname(__file__))

Ui_MainWindow, QMainWindow = uic.loadUiType(fr'{DIR}\assets\app.ui')

logging.basicConfig(
	level=logging.INFO, 
	filename=fr'{DIR}\..\golddrive.log',
	format='%(asctime)s: %(name)-10s: %(levelname)-7s: %(message)s',
	datefmt='%Y-%m-%d %H:%M:%S')
logger = logging.getLogger('golddrive')


class Menu(QMenu):
	'''Subclass QMenu to align at the right'''
	def __init__(self, button):
		super().__init__()
		self.b = button

	def showEvent(self, e):
		p = self.pos();
		geo = self.b.geometry();
		self.move(p.x()+geo.width()-self.geometry().width(), p.y())


class Window(QMainWindow, Ui_MainWindow):
	
	def __init__(self):
		QMainWindow.__init__(self)
		self.setupUi(self)
		self.loaded = False
		self.setWindowTitle('Gold Drive')
		app_icon = QIcon()
		app_icon.addFile(fr'{DIR}\assets\icon_16.png', QSize(16,16))
		app_icon.addFile(fr'{DIR}\assets\icon_32.png', QSize(32,32))
		app_icon.addFile(fr'{DIR}\assets\icon_48.png', QSize(48,48))
		QApplication.setWindowIcon(app_icon)
		os.chdir(f'{DIR}')
		with open(fr'{DIR}\assets\style.css') as r:
			self.setStyleSheet(r.read())

		# initial values from settings
		self.settings = QSettings("sganis", "golddrive")
		if self.settings.value("geometry"):
			self.restoreGeometry(self.settings.value("geometry"))
			self.restoreState(self.settings.value("windowState"))
		self.progressBar.setVisible(False)	
		self.lblStatus.setText("")
		self.widgetPassword.setVisible(False)
		self.command_state = 'ok' # or error
		self.drive_state = 'disconnected' # or disconnected, or error

		# read config.json
		self.configfile = fr'{DIR}\..\config.json'
		self.config = {}
		self.param = {}
		self.getConfig(self.configfile)
		self.updateCombo(self.settings.value("cboParam",0))	
		self.fillParam()
		self.lblUserHostPort.setText(self.param['userhostport'])

		menu = Menu(self.pbHamburger)
		menu.addAction('Setup SSH keys', self.mnuSetupSsh)
		menu.addAction('Connect', self.mnuConnect)
		menu.addAction('Disconnect', self.mnuDisconnect)
		menu.addAction('Open program location', self.mnuOpenProgramLocation)
		menu.addAction('Open log file', self.mnuOpenLogFile)
		menu.addAction('Restart Explorer.exe', self.mnuRestartExplorer)
		self.pbHamburger.setMenu(menu)

		# worker for background commands
		self.worker = Worker()
		self.worker.workDone.connect(self.onWorkDone)
		
		# worker for watching config.json changes
		self.watcher = QFileSystemWatcher()
		self.watcher.addPaths([self.configfile])
		self.watcher.fileChanged.connect(self.onConfigFileChanged)

		self.getStatus()
		self.loaded = True

	@pyqtSlot(str, util.ReturnBox)
	def onWorkDone(self, task, rb):
		self.drive_state = rb.drive_state
		if rb.error:
			self.command_state = 'error'
		if task == 'testssh':
			if rb.output:
				self.end(rb.output)
			else:
				self.txtPassword.setText('')
				self.txtPassword.setFocus()
				self.widgetPassword.setVisible(True)
				self.end('Authentication required:')				
		elif task == 'setupssh':
			if rb.error:
				if 'password wrong' in rb.error:
					rb.error = 'Wrong password, try again:'
					self.widgetPassword.setVisible(True)
				self.end(rb.error)
			else:
				self.widgetPassword.setVisible(False)
				self.end(rb.output)
		else:
			msg = rb.output 
			if rb.error:
				msg = rb.error
			self.end(msg)

	@pyqtSlot(str)
	def onConfigFileChanged(self, path):
		logger.error('Config file has changed, reloading...')
		self.getConfig(path)
		self.updateCombo(self.settings.value("cboParam",0))
		self.fillParam()
		self.lblUserHostPort.setText(self.param['userhostport'])
		self.getStatus()

	def getConfig(self, path):
		try:
			with open(path) as f:
				self.config = json.load(f)
		except Exception as ex:
			logger.error(f'Cannot read config file: {path}')

	def updateCombo(self, currentIndex):
		items = []
		drives = self.config['drives']
		for d in drives:
			items.append(f"   {d}   {drives[d]['drivename']}")
		self.cboParam.blockSignals(True)
		self.cboParam.clear()
		self.cboParam.addItems(items)
		self.cboParam.setCurrentIndex(currentIndex)
		self.cboParam.blockSignals(False)
		
	def fillParam(self):
		p = self.param
		p['ssh'] = os.path.expandvars(fr"{self.config['sshfs_path']}\ssh.exe")
		p['sshfs'] = os.path.expandvars(fr"{self.config['sshfs_path']}\sshfs.exe")
		p['editor'] = os.path.expandvars(self.config['editor'])
		p['logfile'] = os.path.expandvars(self.config['logfile'])
		currentText = self.cboParam.currentText();
		if not currentText:
			return
		drive = currentText.split()[0].strip()
		d = self.config['drives'][drive]
		p['drive'] = drive
		p['drivename'] = d.get('drivename', 'Golddrive')
		p['host'] = d['hosts'][0]
		p['port'] = d.get('port', 22)
		p['user'] = d.get('user', getpass.getuser())		
		p['userhost'] = f"{p['user']}@{p['host']}"
		p['userhostport'] = f"{p['userhost']}:{p['port']}"
		p['seckey'] = util.defaultKey(p['user'])
		
	# decorator used to trigger only the int overload and not twice
	@pyqtSlot(int)
	def on_cboParam_currentIndexChanged(self, e):
		# print(f'index changed: {e}')
		cbotext = self.cboParam.currentText();
		if not cbotext:
			return
		self.fillParam()
		self.lblUserHostPort.setText(self.param['userhostport'])
		if self.loaded:
			self.settings.setValue("cboParam", 
				self.cboParam.currentIndex())
			self.getStatus()

	def getStatus(self):
		self.start(f"Checking {self.param['drive']}...")
		self.worker.doWork('get_status', self.param)


	def mnuSetupSsh(self):
		user = self.param['user']
		host = self.param['host']
		port = self.param['port']
		self.start(f'Checking ssh keys for {user}...')
		self.worker.doWork('testssh', self.param)
		
	def on_txtPassword_returnPressed(self):
		self.widgetPassword.setVisible(False)
		self.start('Logging in...')
		self.param['password'] = str(self.txtPassword.text())
		self.worker.doWork('setupssh', self.param)

	def mnuConnect(self):
		self.start('Connecting drive...')
		self.worker.doWork('connect', self.param)

	def mnuDisconnect(self):
		self.start('Disconnecting drive...')
		self.worker.doWork('disconnect', self.param)

	def mnuOpenProgramLocation(self):
		editor = self.param["editor"]
		if not os.path.exists(editor):
			editor = 'C:\\windows\\notepad.exe'
		cmd = fr'start /b c:\windows\explorer.exe "{DIR}\.."'
		util.run(cmd, shell=True)

	def mnuOpenLogFile(self):
		editor = self.param["editor"]
		if not os.path.exists(editor):
			editor = 'C:\\windows\\notepad.exe'
		logfile = self.param['logfile']
		cmd = fr'start /b "" "{editor}" "{logfile}"'
		util.run(cmd, shell=True)

	
	def mnuRestartExplorer(self):
		self.start('Restarting explorer...')
		self.worker.doWork('restart_explorer', self.param)
		
	def closeEvent(self, event):	
		self.settings.setValue("geometry", self.saveGeometry())
		self.settings.setValue("windowState", self.saveState())			
		QMainWindow.closeEvent(self, event)
		self.worker.stop()
		# self.thread.quit()
		# self.thread.wait()

	def start(self, message):	
		self.command_state = 'ok'
		self.pbConnect.setEnabled(False)
		self.widgetPassword.setVisible(False)
		self.progressBar.setVisible(True)
		self.lblStatus.setVisible(True)
		self.lblStatus.setProperty("error", False);
		self.lblStatus.style().unpolish(self.lblStatus);
		self.lblStatus.style().polish(self.lblStatus);	
		self.lblStatus.setText(message)
		logger.info(message)
		
	def end(self, message=''):
		self.progressBar.setVisible(False)
		self.lblStatus.setText(message)
		self.pbConnect.setEnabled(True)
		self.lblStatus.setProperty("error", 
			self.command_state == 'error');
		self.lblStatus.style().unpolish(self.lblStatus);
		self.lblStatus.style().polish(self.lblStatus);
		if self.drive_state == 'CONNECTED':
			self.pbConnect.setText('DISCONNECT')
		elif self.drive_state == 'DISCONNECTED':
			self.pbConnect.setText('CONNECT')
		else:
			self.pbConnect.setText('RECONNECT')
		if message:
			if self.command_state == 'error':
				logger.error(message)
			else:
				logger.info(message)
		# self.pbConnect.setText('CONNECT')

	def on_pbConnect_released(self):
		drive = self.param['drive']
		self.start(f'Connecting {drive} ...')
		self.worker.doWork('connect', self.param)

	def on_pbCancel_released(self):
		self.widgetPassword.setVisible(False)
		self.lblStatus.setVisible(True)
		self.txtPassword.setText('')
		self.end()

	def on_lblSettings_linkActivated(self, link):
		editor = self.param["editor"]
		if not os.path.exists(editor):
			editor = 'C:\\windows\\notepad.exe'
		cmd = fr'start /b "" "{editor}" "{DIR}\..\config.json'
		util.run(cmd, shell=True)


def run():
	path = os.environ['PATH']
	os.environ['PATH'] = fr'C:\Program Files\SSHFS-Win\bin;{path}'
	os.environ['GOLDDRIVE_PATH'] = os.path.realpath(fr'{DIR}\..')
	app = QApplication(sys.argv)
	window = Window()
	window.show()
	sys.exit(app.exec_())


if __name__ == '__main__':
	# quit with control-c
	import signal
	signal.signal(signal.SIGINT, signal.SIG_DFL)
	# Remove all handlers associated with the root logger object.
	for handler in logging.root.handlers[:]:
		logging.root.removeHandler(handler)
	# log to console
	logging.basicConfig(
		level=logging.INFO, 
		format='%(asctime)s: %(name)-10s: %(levelname)-7s: %(message)s',
		datefmt='%Y-%m-%d %H:%M:%S')

	run()
