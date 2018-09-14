#!/usr/bin/python3
# -*- coding: utf-8 -*-

from PyQt5.QtCore import pyqtSignal
from PyQt5.QtWidgets import QWidget
from ui.login_ui import Ui_Login
import util

class Login(QWidget, Ui_Login):
	
	loginPressed = pyqtSignal(str, str)
	cancelPressed = pyqtSignal(util.Page)

	def __init__(self, parent=None):
		QWidget.__init__(self, parent)
		self.setupUi(self)
		self.pbLogin.setProperty("css", True)
		self.pbCancel.setProperty("css", True)
		self.error = ''
		self.txtHost.setVisible(False)

	def init(self, host=''):
		self.txtHost.setText(host)
		self.showError('')
		self.lblMessage.setText('Autentication required:')
		self.txtPassword.setText('')
		self.txtPassword.setFocus()
		self.enable(True)

	def enable(self, enable):
		self.setEnabled(enable)
		self.pbLogin.setEnabled(enable)
		self.pbCancel.setEnabled(enable)
	
	def on_pbLogin_released(self):
		if self.validate():
			host = str(self.txtHost.text())
			password = str(self.txtPassword.text())
			self.enable(False)
			self.loginPressed.emit(host, password)
		else:
			self.showError(self.error)
	
	def on_txtPassword_returnPressed(self):
		self.on_pbLogin_released()

	def on_pbCancel_released(self):
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
		