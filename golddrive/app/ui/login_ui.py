# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'login.ui'
#
# Created by: PyQt5 UI code generator 5.11.2
#
# WARNING! All changes made in this file will be lost!

from PyQt5 import QtCore, QtGui, QtWidgets

class Ui_Login(object):
    def setupUi(self, Login):
        Login.setObjectName("Login")
        Login.resize(265, 221)
        self.verticalLayout = QtWidgets.QVBoxLayout(Login)
        self.verticalLayout.setObjectName("verticalLayout")
        self.lblMessage = QtWidgets.QLabel(Login)
        self.lblMessage.setWordWrap(True)
        self.lblMessage.setObjectName("lblMessage")
        self.verticalLayout.addWidget(self.lblMessage)
        self.txtHost = QtWidgets.QLineEdit(Login)
        self.txtHost.setObjectName("txtHost")
        self.verticalLayout.addWidget(self.txtHost)
        self.txtPassword = QtWidgets.QLineEdit(Login)
        self.txtPassword.setEchoMode(QtWidgets.QLineEdit.Password)
        self.txtPassword.setObjectName("txtPassword")
        self.verticalLayout.addWidget(self.txtPassword)
        self.widget_8 = QtWidgets.QWidget(Login)
        self.widget_8.setStyleSheet("border: 0px")
        self.widget_8.setObjectName("widget_8")
        self.horizontalLayout_6 = QtWidgets.QHBoxLayout(self.widget_8)
        self.horizontalLayout_6.setContentsMargins(0, 0, 0, 0)
        self.horizontalLayout_6.setSpacing(10)
        self.horizontalLayout_6.setObjectName("horizontalLayout_6")
        spacerItem = QtWidgets.QSpacerItem(40, 20, QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Minimum)
        self.horizontalLayout_6.addItem(spacerItem)
        self.pbLogin = QtWidgets.QPushButton(self.widget_8)
        self.pbLogin.setMaximumSize(QtCore.QSize(100, 16777215))
        self.pbLogin.setObjectName("pbLogin")
        self.horizontalLayout_6.addWidget(self.pbLogin)
        self.pbCancel = QtWidgets.QPushButton(self.widget_8)
        self.pbCancel.setMaximumSize(QtCore.QSize(100, 16777215))
        self.pbCancel.setObjectName("pbCancel")
        self.horizontalLayout_6.addWidget(self.pbCancel)
        self.verticalLayout.addWidget(self.widget_8)
        spacerItem1 = QtWidgets.QSpacerItem(20, 40, QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Expanding)
        self.verticalLayout.addItem(spacerItem1)

        self.retranslateUi(Login)
        QtCore.QMetaObject.connectSlotsByName(Login)

    def retranslateUi(self, Login):
        _translate = QtCore.QCoreApplication.translate
        Login.setWindowTitle(_translate("Login", "Login"))
        self.lblMessage.setText(_translate("Login", "Login:"))
        self.txtHost.setPlaceholderText(_translate("Login", "Remote Host"))
        self.txtPassword.setPlaceholderText(_translate("Login", "Password"))
        self.pbLogin.setText(_translate("Login", "CONNECT"))
        self.pbCancel.setText(_translate("Login", "CANCEL"))

from ui import assets_rc
