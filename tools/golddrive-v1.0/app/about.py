#!/usr/bin/python3
# -*- coding: utf-8 -*-

import os
from PyQt5.QtCore import pyqtSignal, QUrl, QThread
from PyQt5.QtGui import QDesktopServices
from PyQt5.QtWidgets import QWidget
from about_ui import Ui_About
import util
import re


class AboutWorker(QThread):
	
	workDone = pyqtSignal(str)
	
	def __init__(self):
		QThread.__init__(self)
	
	def run(self):
		result = util.get_versions()
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
		link = util.make_hyperlink(link, link)
		version = util.get_app_version()
		if not version:
			version = 'N/A'
		self.about = (f"Golddrive {version}\n"
				"Map drive to ssh server\n"
				f"{link}\n")

	def showAbout(self):
		if self.worker is None:
			self.worker = AboutWorker()
			self.worker.workDone.connect(self.onWorkDone)
		self.lblAbout.setText(util.rich_text(self.about))
		self.pbAboutOk.setVisible(False)
		self.worker.start()
	
	def onWorkDone(self, result):
		msg = (f"{self.about}\n{result}")
		self.lblAbout.setText(util.rich_text(msg))
		self.pbAboutOk.setVisible(True)
		
	def setMain(self, main):
		self.main = main

	def on_lblAbout_linkActivated(self, link):
		print(link)
		QDesktopServices.openUrl(QUrl(link))

	def on_pbAboutOk_released(self):
		self.okPressed.emit(util.Page.MAIN)
