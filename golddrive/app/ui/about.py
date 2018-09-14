#!/usr/bin/python3
# -*- coding: utf-8 -*-

from PyQt5.QtCore import pyqtSignal
from PyQt5.QtWidgets import QWidget
from ui.about_ui import Ui_About
from app import VERSION
import util

class About(QWidget, Ui_About):

	okPressed = pyqtSignal(util.Page)

	def __init__(self, parent=None):
		QWidget.__init__(self, parent)
		self.setupUi(self)
		self.pbAboutOk.setProperty("css", True)
		msg = (f"Golddrive {VERSION}\n"
				"Map drive to ssh server\n"
				"Comments: <a href='support'>sganis@gmail.com</a>")
		self.lblAbout.setText(util.richText(msg))
		self.main = None

	def setMain(self, main):
		self.main = main

	def on_lblAbout_linkActivated(self, link):
		print(link)

	def on_pbAboutOk_released(self):
		self.okPressed.emit(util.Page.MAIN)
