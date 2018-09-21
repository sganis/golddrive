#!/usr/bin/python3
# -*- coding: utf-8 -*-

from PyQt5.QtCore import pyqtSignal, Qt
from PyQt5.QtWidgets import QWidget
from login_ui import Ui_Login
import util

class Login(QWidget, Ui_Login):
	
	connectPressed = pyqtSignal(str)
	cancelPressed = pyqtSignal(util.Page)

	def __init__(self, parent=None):
		QWidget.__init__(self, parent)
		self.setupUi(self)
		self.pbConnectLogin.setProperty("css", True)
		self.pbCancelLogin.setProperty("css", True)
		self.error = ''
		self.main = None

	def keyPressEvent(self, event):
		if event.key() == Qt.Key_Return or event.key() == Qt.Key_Enter:
			self.on_pbConnectLogin_released()
			event.accept()

	def setMain(self, main):
		self.main = main

	def init(self):
		self.showError('')
		self.lblMessage.setText('Autentication required:')
		self.txtPassword.setText('')
		self.txtPassword.setFocus()
		self.enable(True)

	def enable(self, enable):
		self.setEnabled(enable)
		self.pbConnectLogin.setEnabled(enable)
		self.pbCancelLogin.setEnabled(enable)
	
	def on_pbConnectLogin_released(self):
		if self.validate():
			password = str(self.txtPassword.text())
			self.enable(False)
			self.lblMessage.setText('')
			self.connectPressed.emit(password)
		else:
			self.showError(self.error)
	
	# def on_txtPassword_returnPressed(self):
	# 	self.on_pbConnectLogin_released()

	def on_pbCancelLogin_released(self):
		self.cancelPressed.emit(util.Page.MAIN)

	def validate(self):
		self.error = ''
		if len(str(self.txtPassword.text()).strip()) == 0:
			self.error = 'Password is empty'
			self.txtPassword.setFocus()
		return self.error == ''

	def showError(self, error):
		self.enable(True)
		self.error = error
		self.lblMessage.setText(self.error)
		self.lblMessage.setProperty("error", self.error != '');
		self.lblMessage.style().unpolish(self.lblMessage);
		self.lblMessage.style().polish(self.lblMessage);	
		self.txtPassword.setFocus()
		