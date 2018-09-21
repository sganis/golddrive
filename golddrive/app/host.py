#!/usr/bin/python3
# -*- coding: utf-8 -*-

from PyQt5.QtCore import pyqtSignal, Qt
from PyQt5.QtWidgets import QWidget
from host_ui import Ui_Host
import util

class Host(QWidget, Ui_Host):
	
	connectPressed = pyqtSignal(str, str)
	cancelPressed = pyqtSignal(util.Page)

	def __init__(self, parent=None):
		QWidget.__init__(self, parent)
		self.setupUi(self)
		self.pbConnectHost.setProperty("css", True)
		self.pbCancelHost.setProperty("css", True)
		self.error = ''
		self.main = None
		self.drives = [f'  {d}' for d in 'W:,X:,Y:,Z:'.split(',')]
		self.cboDrive.addItems(self.drives)
		self.cboDrive.setCurrentIndex(len(self.drives)-1)
		
	def setMain(self, main):
		self.main = main

	def keyPressEvent(self, event):
		if event.key() == Qt.Key_Return or event.key() == Qt.Key_Enter:
			self.on_pbConnectHost_released()
			event.accept()

	def init(self, host=''):
		self.showError('')
		self.txtHost.setFocus()
		self.enable(True)

	def enable(self, enable):
		self.setEnabled(enable)
		self.pbConnectHost.setEnabled(enable)
		self.pbCancelHost.setEnabled(enable)
		
	def on_pbConnectHost_released(self):
		if self.validate():
			drive = str(self.cboDrive.currentText()).strip().upper()
			host = str(self.txtHost.text())
			self.enable(False)
			self.lblMessage.setText('')
			self.connectPressed.emit(drive, host)
		else:
			self.showError(self.error)
	
	# def on_txtHost_returnPressed(self):
	# 	self.on_pbConnectHost_released()

	def on_pbCancelHost_released(self):
		self.cancelPressed.emit(util.Page.MAIN)

	def validate(self):
		self.error = ''
		if len(str(self.txtHost.text()).strip()) == 0:
			self.error = 'Hostname is empty'
			self.txtHost.setFocus()
		return self.error == ''

	def showError(self, error):
		self.enable(True)
		self.error = error
		self.lblMessage.setText(self.error)
		self.lblMessage.setProperty("error", self.error != '');
		self.lblMessage.style().unpolish(self.lblMessage);
		self.lblMessage.style().polish(self.lblMessage);	
		self.txtHost.setFocus()
		