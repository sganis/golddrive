#!/usr/bin/python3
# -*- coding: utf-8 -*-
import sys
import os
import time
import subprocess
import getpass
import logging
import util
from worker import Worker
from PyQt5.QtCore import (QObject, pyqtSlot, QFile, Qt,
	QThread, QSize, QSettings, QIODevice, QTextStream,
	QFileSystemWatcher)
from PyQt5.QtGui import QIcon
from PyQt5.QtWidgets import (QWidget, QLabel,
	QComboBox, QApplication, QMainWindow, QMenu)

VERSION = '1.0.0'

DIR 	= os.path.abspath(os.path.dirname(__file__))

from ui.app_ui import Ui_MainWindow

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
		self.pbConnect.setProperty("css", True)
		self.pageHost.setMain(self)
		self.pageHost.connectPressed.connect(self.onConnectHost)
		self.pageHost.cancelPressed.connect(self.showPage)
		self.pageLogin.setMain(self)
		self.pageLogin.connectPressed.connect(self.onConnectLogin)
		self.pageLogin.cancelPressed.connect(self.showPage)
		self.pageAbout.setMain(self)
		self.pageAbout.okPressed.connect(self.showPage)

		self.loaded = False
		self.setWindowTitle('Gold Drive')
		app_icon = QIcon()
		app_icon.addFile(':/assets/icon_16.png', QSize(16,16))
		app_icon.addFile(':/assets/icon_32.png', QSize(32,32))
		app_icon.addFile(':/assets/icon_64.png', QSize(64,64))
		app_icon.addFile(':/assets/icon_128.png', QSize(128,128))
		QApplication.setWindowIcon(app_icon)
		stream = QFile(':/assets/style.css')
		if stream.open(QIODevice.ReadOnly | QFile.Text):
			self.setStyleSheet(QTextStream(stream).readAll())

		# initial values from settings
		self.settings = QSettings("sganis", "golddrive")
		if self.settings.value("geometry"):
			self.restoreGeometry(self.settings.value("geometry"))
			self.restoreState(self.settings.value("windowState"))
		self.progressBar.setVisible(False)	
		self.lblMessage.setText("")
		# self.widgetPassword.setVisible(False)
		self.returncode = util.ReturnCode.NONE

		# read config.json
		self.configfile = fr'{DIR}\..\config.json'
		self.param = {}
		
		self.config = util.loadConfig(self.configfile)
		path = os.environ['PATH']
		sshfs_path = self.config.get('sshfs_path','')
		os.environ['PATH'] = fr'{sshfs_path};{path}'	

		self.updateCombo(self.settings.value("cboParam",0))	
		self.fillParam()
		self.lblUserHostPort.setText(self.param['userhostport'])

		menu = Menu(self.pbHamburger)
		menu.addAction('&Connect', self.mnuConnect)
		menu.addAction('&Disconnect', self.mnuDisconnect)
		menu.addAction('Disconnect &all drives', self.mnuDisconnectAll)
		menu.addAction('&Open program location', self.mnuOpenProgramLocation)
		menu.addAction('Open &log file', self.mnuOpenLogFile)
		menu.addAction('&Restart Explorer.exe', self.mnuRestartExplorer)
		menu.addAction('&About...', self.mnuAbout)
		self.pbHamburger.setMenu(menu)

		# worker for background commands
		self.worker = Worker()
		self.worker.workDone.connect(self.onWorkDone)
		
		# worker for watching config.json changes
		self.watcher = QFileSystemWatcher()
		self.watcher.addPaths([self.configfile])
		self.watcher.fileChanged.connect(self.onConfigFileChanged)

		self.checkDriveStatus()
		self.showPage(util.Page.MAIN)
		self.lblMessage.linkActivated.connect(self.on_lblMessage_linkActivated)
		self.loaded = True

	def start(self, message):	
		self.showPage(util.Page.MAIN)
		self.returncode = util.ReturnCode.NONE
		self.pbConnect.setEnabled(False)
		self.progressBar.setVisible(True)
		self.showMessage(message)
		
	def end(self, message=''):
		self.showPage(util.Page.MAIN)
		self.progressBar.setVisible(False)
		self.pbConnect.setEnabled(True)
		self.showMessage(message)
		
	def showMessage(self, message):
		is_error = (self.returncode != util.ReturnCode.OK 
			and self.returncode != util.ReturnCode.NONE) 
		self.lblMessage.setText(message)
		self.lblMessage.setProperty("error", is_error);
		self.lblMessage.style().unpolish(self.lblMessage);
		self.lblMessage.style().polish(self.lblMessage);

	def setPbConnectText(self, status):
		self.pbConnect.setText('&CONNECT')
		if status == 'CONNECTED':
			self.pbConnect.setText('&DISCONNECT')
		elif status == 'BROKEN':
			self.pbConnect.setText('&REPAIR')
		elif status == 'IN USE':
			self.pbConnect.setEnabled(False)

	@pyqtSlot(str, util.ReturnBox)
	def onWorkDone(self, task, rb):

		self.returncode = rb.returncode

		if task == 'check_drive':
			if rb.drive_status == 'CONNECTED':
				drive = self.param['drive']				
				link = util.makeHyperlink('open_drive', 'Open')
				msg = util.richText(f"{rb.drive_status}\n\n{link}")
			else:
				msg = rb.drive_status
				if rb.error:
					msg = rb.error
			self.end(msg)

		elif task == 'connect':
			
			if rb.returncode == util.ReturnCode.BAD_DRIVE:
				self.end(rb.error)
				
			elif rb.returncode == util.ReturnCode.BAD_HOST:
				self.showPage(util.Page.HOST)
				self.pageHost.showError(rb.error)
				self.progressBar.setVisible(False)

			elif rb.returncode == util.ReturnCode.BAD_LOGIN:
				self.showPage(util.Page.LOGIN)
				self.pageLogin.showError(rb.error)
				self.progressBar.setVisible(False)

			elif rb.returncode == util.ReturnCode.BAD_MOUNT:
				self.end(rb.error)

			else:
				assert rb.returncode == util.ReturnCode.OK
				msg = (f"CONNECTED\n\n{ util.makeHyperlink('open_drive', 'Open') }")
				self.end(util.richText(msg))

		elif task == 'connect_login':
			
			if not rb.returncode == util.ReturnCode.OK:
				self.pageLogin.showError(rb.error)
				self.progressBar.setVisible(False)

			elif rb.drive_status == 'CONNECTED':
				link = util.makeHyperlink('open_drive', 'Open')
				msg = util.richText(f"{rb.drive_status}\n\n{link}")
									
				self.end(msg)
			else:
				msg = rb.drive_status
				if rb.error:
					msg = rb.error
				self.end(msg)

		
		elif (task == 'disconnect' or task == 'repair'):
			self.end(rb.drive_status)

		elif task == 'restart_explorer':
			link = util.makeHyperlink('open_drive', 'Open')
			msg = util.richText(f"{rb.output}\n\n{link}")
			self.end(msg)

		else:
			msg = rb.output 
			if rb.error:
				msg = rb.error
			self.end(msg)
		self.setPbConnectText(rb.drive_status)

	def onConfigFileChanged(self, path):
		logger.error('Config file has changed, reloading...')
		self.config = util.loadConfig(path)
		self.updateCombo(self.settings.value("cboParam",0))
		self.fillParam()
		self.lblUserHostPort.setText(self.param['userhostport'])
		self.checkDriveStatus()

	def updateCombo(self, currentIndex):
		items = []
		drives = self.config.get('drives','')
		for d in drives:
			items.append(f"   {d}   {drives[d].get('drivename','GOLDDRIVE')}")
		self.cboParam.blockSignals(True)
		self.cboParam.clear()
		self.cboParam.addItems(items)
		if currentIndex > len(items)-1:
			currentIndex = 0
		self.cboParam.setCurrentIndex(currentIndex)
		self.cboParam.blockSignals(False)
		
	def fillParam(self):
		p = self.param
		p['editor'] = self.config.get('editor')
		p['logfile'] = self.config.get('logfile')
		p['configfile'] = self.configfile
		p['user'] = getpass.getuser()		
		p['appkey'] = util.getAppKey(p['user'])
		p['userkey'] = util.getUserKey()
		p['userhostport'] = ''
		p['port'] = 22
		p['drive'] = ''
		p['drivename'] = 'GOLDDRIVE'
		drives = self.config.get('drives')
		p['no_host'] = not bool(drives)
		if not drives:
			return
		currentText = self.cboParam.currentText();
		if not currentText:
			return
		drive = currentText.split()[0].strip()
		d = drives[drive]
		p['drive'] = drive
		p['drivename'] = d.get('drivename', 'GOLDDRIVE').replace(' ','-')
		p['host'] = d.get('hosts','localhost')[0]
		p['port'] = d.get('port', 22)
		p['user'] = d.get('user', getpass.getuser())		
		p['userhost'] = f"{p['user']}@{p['host']}"
		p['userhostport'] = f"{p['userhost']}:{p['port']}"
		p['appkey'] = util.getAppKey(p['user'])
		
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
			self.checkDriveStatus()

	def checkDriveStatus(self):
		if not self.param.get('drive'):
			return
		self.start(f"Checking {self.param['drive']}...")
		self.worker.doWork('check_drive', self.param)
	
	def onConnectHost(self, drive, userhostport):
		self.progressBar.setVisible(True)
		self.param['drive'] = drive
		userhost = userhostport
		host = userhostport
		self.param['userhost'] = userhost
		self.param['host'] = host
		self.param['port'] = 22
		if ':' in userhostport:
			userhost, port = userhostport.split(':')
			self.param['port'] = port
			self.param['host'] = userhost
			host = userhost
		if '@' in userhost:
			user, host = userhost.split('@')			
			self.param['user'] = user
			self.param['host'] = host
		else:
			user = self.param['user']
		self.param['userhost'] = f'{user}@{host}'
		self.param['appkey'] = util.getAppKey(user)

		# print(f"user: {self.param['user']}")
		# print(f"host: {self.param['host']}")
		# print(f"port: {self.param['port']}")
		# print(f"userhost: {self.param['userhost']}")
		self.worker.doWork('connect', self.param)
	
	def onConnectLogin(self, password):
		self.progressBar.setVisible(True)
		self.param['password'] = password
		self.worker.doWork('connect_login', self.param)

	def mnuConnect(self):
		if self.param['no_host']:
			self.showPage(util.Page.HOST)
		else:
			self.start('Connecting drive...')
			self.worker.doWork('connect', self.param)

	def mnuDisconnect(self):
		self.start('Disconnecting drive...')
		self.worker.doWork('disconnect', self.param)

	def mnuDisconnectAll(self):
		self.start('Disconnecting all...')
		self.worker.doWork('disconnect_all', self.param)

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
	
	def mnuAbout(self):
		self.showPage(util.Page.ABOUT)
		self.pageAbout.showAbout()

	def closeEvent(self, event):	
		self.settings.setValue("geometry", self.saveGeometry())
		self.settings.setValue("windowState", self.saveState())			
		QMainWindow.closeEvent(self, event)
		self.worker.stop()
		# self.thread.quit()
		# self.thread.wait()

	def on_pbConnect_released(self):
		task = self.pbConnect.text().lower().replace('&','')
		if task == 'connect':
			self.mnuConnect()
		else:
			self.mnuDisconnect()

	def on_lblSettings_linkActivated(self, link):
		editor = self.param["editor"]
		if not os.path.exists(editor):
			editor = 'C:\\windows\\notepad.exe'
		cmd = fr'start /b "" "{editor}" "{DIR}\..\config.json'
		util.run(cmd, shell=True)

	def on_lblMessage_linkActivated(self, link):
		drive = self.param['drive']
		cmd = fr'start /b c:\windows\explorer.exe {drive}'
		util.run(cmd, shell=True)

	def showPage(self, page):
		if page == util.Page.HOST:
			self.pageHost.init()
		if page == util.Page.LOGIN:
			self.pageLogin.init()
		self.setTopEnabled(page == util.Page.MAIN)
		self.stackedWidget.setCurrentIndex(page.value)
		print(f'panel visible: {page}')

	def setTopEnabled(self, enable):
		self.pbHamburger.setEnabled(enable)
		self.cboParam.setEnabled(enable)
		self.lblSettings.setVisible(enable)

	def keyPressEvent(self, event):
		if event.key() == Qt.Key_Return or event.key() == Qt.Key_Enter:
			print('enter pressed in app')
			self.on_pbConnect_released()
			event.accept()


def run():
	os.environ['GOLDDRIVE'] = os.path.realpath(fr'{DIR}\..')
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
