#!/usr/bin/python3
# -*- coding: utf-8 -*-

import os
from PyQt5.QtCore import pyqtSignal, QUrl, QThread
from PyQt5.QtGui import QDesktopServices
from PyQt5.QtWidgets import QWidget
from ui.about_ui import Ui_About
from app import VERSION
import util
import re


class AboutWorker(QThread):
	
	workDone = pyqtSignal(str)
	
	def __init__(self, main):
		QThread.__init__(self)
		self.main = main
	
	def run(self):
		config = self.main.config
		ssh = ''
		sshfs = ''
		winfsp = ''
		
		r = util.run(f'{config["ssh"]} -V', capture=True)
		if r.returncode == 0:
			m = re.match(r'(OpenSSH[^,]+),[\s]*(OpenSSL[\s]?[\w.]+)', r.stderr)
			if m:
				ssh = f'{m.group(1)}\n{m.group(2)}'
		
		r = util.run(f'{config["sshfs"]} -V', capture=True)
		if r.returncode == 0:
			sshfs = f'{r.stdout}'
		
		wmic = fr'c:\windows\system32\wbem\wmic.exe'
		p86 = os.path.expandvars('%ProgramFiles(x86)%')
		cmd =f"{wmic} datafile where name='{p86}\\WinFsp\\bin\\winfsp-x64.dll' get version /format:list"
		r = util.run(cmd.replace('\\','\\\\'), capture=True)
		if r.returncode == 0 and '=' in r.stdout:
			winfsp_ver = r.stdout.split("=")[-1]
			winfsp = f'WINFSP {winfsp_ver}'

		result = f'{ssh}\n{sshfs}\n{winfsp}'
		self.workDone.emit(result)


class About(QWidget, Ui_About):

	okPressed = pyqtSignal(util.Page)

	def __init__(self, parent=None):
		QWidget.__init__(self, parent)
		self.setupUi(self)
		self.pbAboutOk.setProperty("css", True)
		self.lblAbout.setText("")
		self.worker = None
		self.main = None
		self.about = (f"Golddrive {VERSION}\n"
				"Map drive to ssh server\n"
				"<a href='http://github.com/sganis/golddrive'>http://github.com/sganis/golddrive</a>\n")

	def showAbout(self):
		if self.worker is None:
			self.worker = AboutWorker(self.main)
			self.worker.workDone.connect(self.onWorkDone)
		self.lblAbout.setText(util.richText(self.about))
		self.pbAboutOk.setVisible(False)
		self.worker.start()
	
	def onWorkDone(self, result):
		msg = (f"{self.about}\n{result}")
		self.lblAbout.setText(util.richText(msg))
		self.pbAboutOk.setVisible(True)
		
	def setMain(self, main):
		self.main = main

	def on_lblAbout_linkActivated(self, link):
		print(link)
		QDesktopServices.openUrl(QUrl(link))

	def on_pbAboutOk_released(self):
		self.okPressed.emit(util.Page.MAIN)
