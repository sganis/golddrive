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
from PyQt5.QtCore import (QObject, pyqtSlot, QFile, Qt,
	QThread, QSize, QSettings, QIODevice, QTextStream,
	QFileSystemWatcher)
from PyQt5.QtGui import QIcon
from PyQt5.QtWidgets import (QWidget, QLabel,
	QComboBox, QApplication, QMainWindow, QMenu)

VERSION = '0.1'

from ui.app_ui import Ui_MainWindow

DIR = os.path.abspath(os.path.dirname(__file__))
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
		self.pageLogin.loginPressed.connect(self.onLogin)
		self.pageLogin.cancelPressed.connect(self.showPage)
		self.pageAbout.okPressed.connect(self.showPage)

		self.loaded = False
		self.setWindowTitle('Gold Drive')
		app_icon = QIcon()
		app_icon.addFile(':/assets/icon_16.png', QSize(16,16))
		app_icon.addFile(':/assets/icon_32.png', QSize(32,32))
		app_icon.addFile(':/assets/icon_64.png', QSize(64,64))
		app_icon.addFile(':/assets/icon_128.png', QSize(128,128))
		app_icon.addFile(':/assets/icon_256.png', QSize(256,256))
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
		self.command_state = 'ok' # or error

		# read config.json
		self.configfile = fr'{DIR}\..\config.json'
		self.config = {}
		self.param = {}
		self.getConfig(self.configfile)
		self.updateCombo(self.settings.value("cboParam",0))	
		self.fillParam()
		self.lblUserHostPort.setText(self.param['userhostport'])

		menu = Menu(self.pbHamburger)
		menu.addAction('&Setup SSH keys', self.mnuSetupSsh)
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

	def keyPressEvent(self, event):
		if event.key() == Qt.Key_Return:
			print('entre pressed')
			self.on_pbConnect_released()
			event.accept()

	def start(self, message):	
		self.showPage(util.Page.MAIN)
		self.command_state = 'ok'
		self.pbConnect.setEnabled(False)
		self.progressBar.setVisible(True)
		self.showMessage(message)
		
	def end(self, message=''):
		self.showPage(util.Page.MAIN)
		self.progressBar.setVisible(False)
		self.pbConnect.setEnabled(True)
		self.showMessage(message)
		
	def showMessage(self, message):
		is_error = self.command_state == 'error'
		self.lblMessage.setText(message)
		self.lblMessage.setProperty("error", is_error);
		self.lblMessage.style().unpolish(self.lblMessage);
		self.lblMessage.style().polish(self.lblMessage);
		if is_error:
			logger.error(message)
		else:
			logger.info(message)

	def setPbConnectText(self, status):
		self.pbConnect.setText('&CONNECT')
		if status == 'CONNECTED':
			self.pbConnect.setText('&DISCONNECT')
		elif status == 'BROKEN':
			self.pbConnect.setText('&REPAIR')
		elif status == 'KEYS_WRONG':
			self.pbConnect.setText('&SETUP KEYS')
		elif status == 'IN USE':
			self.pbConnect.setEnabled(False)
			

	@pyqtSlot(str, util.ReturnBox)
	def onWorkDone(self, task, rb):
		if rb.error:
			self.command_state = 'error'
		if task == 'check_keys':
			if rb.work_status == util.WorkStatus.SUCCESS:
				self.end('SSH keys are good')
			else:
				self.end()
				self.showPage(util.Page.LOGIN)

		elif task == 'setup_keys':
			self.progressBar.setVisible(False)
			if rb.error:
				self.pageLogin.showError(rb.error)
			else:
				self.end(rb.output)

		elif task == 'connect' or task == 'check_drive':
			if rb.drive_status == 'CONNECTED':
				drive = self.param['drive']				
				msg = util.richText(f"{rb.drive_status}\n\n"
						f"<a href='open_drive'>Open</a>")
			else:
				msg = rb.drive_status
				if rb.error:
					msg = rb.error
			self.end(msg)
		
		elif (task == 'disconnect' or task == 'repair'):
			self.end(rb.drive_status)

		elif task == 'restart_explorer':
			msg = util.richText(f"{rb.output}\n\n"
					f"<a href='open_drive'>Open</a>")
			self.end(msg)

		else:
			msg = rb.output 
			if rb.error:
				msg = rb.error
			self.end(msg)
		self.setPbConnectText(rb.drive_status)

	def onConfigFileChanged(self, path):
		logger.error('Config file has changed, reloading...')
		self.getConfig(path)
		self.updateCombo(self.settings.value("cboParam",0))
		self.fillParam()
		self.lblUserHostPort.setText(self.param['userhostport'])
		self.checkDriveStatus()

	def getConfig(self, path):
		try:
			with open(path) as f:
				self.config = json.load(f)
			path = os.environ['PATH']
			sshfs_path = os.path.expandvars(self.config['sshfs_path'])
			os.environ['PATH'] = fr'{sshfs_path};{path}'	
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
		p['seckey'] = util.getAppKey(p['user'])
		
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
		self.start(f"Checking {self.param['drive']}...")
		self.worker.doWork('check_drive', self.param)

	def mnuSetupSsh(self):
		user = self.param['user']
		host = self.param['host']
		port = self.param['port']
		self.start(f'Checking ssh keys...')
		self.worker.doWork('check_keys', self.param)
		
	def onLogin(self, host, password):
		self.progressBar.setVisible(True)
		self.param['host'] = host
		self.param['password'] = password
		self.worker.doWork('setup_keys', self.param)

	def mnuConnect(self):
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
		
	def closeEvent(self, event):	
		self.settings.setValue("geometry", self.saveGeometry())
		self.settings.setValue("windowState", self.saveState())			
		QMainWindow.closeEvent(self, event)
		self.worker.stop()
		# self.thread.quit()
		# self.thread.wait()

	def on_pbConnect_released(self):
		drive = self.param['drive']
		self.start(f'Checking {drive} ...')
		task = self.pbConnect.text().lower().replace('&','')
		if task == 'setup keys':
			self.mnuSetupSsh()
		else:
			self.worker.doWork(task, self.param)

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
		if page == util.Page.LOGIN:
			self.pageLogin.init(self.param['host'])
		self.widget_1_top.setEnabled(page == util.Page.MAIN)
		self.widget_2_combo.setEnabled(page == util.Page.MAIN)
		self.stackedWidget.setCurrentIndex(page.value)



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
