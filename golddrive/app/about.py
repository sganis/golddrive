#!/usr/bin/python3
# -*- coding: utf-8 -*-

import os
from PyQt5.QtCore import pyqtSignal, QUrl, QThread
from PyQt5.QtGui import QDesktopServices
from PyQt5.QtWidgets import QWidget
from about_ui import Ui_About
from app import VERSION
import util
import re


class AboutWorker(QThread):
	
	workDone = pyqtSignal(str)
	
	def __init__(self):
		QThread.__init__(self)
	
	def run(self):
		ssh = ''
		sshfs = ''
		winfsp = ''
		
		r = util.run(f'ssh -V', capture=True)
		if r.returncode == 0:
			m = re.match(r'(OpenSSH[^,]+),[\s]*(OpenSSL[\s]?[\w.]+)', r.stderr)
			if m:
				ssh = f'{m.group(1)}\n{m.group(2)}'
		
		r = util.run(f'sshfs -V', capture=True)
		if r.returncode == 0:
			sshfs = f'{r.stdout}'
		
		p86 = os.path.expandvars('%ProgramFiles(x86)%')
		cmd =f"wmic datafile where name='{p86}\\WinFsp\\bin\\winfsp-x64.dll' get version /format:list"
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
		link = 'http://github.com/sganis/golddrive'
		link = util.makeHyperlink(link, link)
		self.about = (f"Golddrive {VERSION}\n"
				"Map drive to ssh server\n"
				f"{link}\n")

	def showAbout(self):
		if self.worker is None:
			self.worker = AboutWorker()
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
