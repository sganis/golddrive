# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file '..\assets\ui\login.ui'
#
# Created by: PyQt5 UI code generator 5.11.2
#
# WARNING! All changes made in this file will be lost!

from PyQt5 import QtCore, QtGui, QtWidgets

class Ui_Login(object):
    def setupUi(self, Login):
        Login.setObjectName("Login")
        Login.resize(311, 161)
        self.verticalLayout = QtWidgets.QVBoxLayout(Login)
        self.verticalLayout.setSpacing(10)
        self.verticalLayout.setObjectName("verticalLayout")
        self.label = QtWidgets.QLabel(Login)
        self.label.setObjectName("label")
        self.verticalLayout.addWidget(self.label)
        self.txtPassword = QtWidgets.QLineEdit(Login)
        self.txtPassword.setEchoMode(QtWidgets.QLineEdit.Password)
        self.txtPassword.setObjectName("txtPassword")
        self.verticalLayout.addWidget(self.txtPassword)
        self.widget_8 = QtWidgets.QWidget(Login)
        self.widget_8.setObjectName("widget_8")
        self.horizontalLayout_6 = QtWidgets.QHBoxLayout(self.widget_8)
        self.horizontalLayout_6.setContentsMargins(0, 0, 0, 0)
        self.horizontalLayout_6.setSpacing(10)
        self.horizontalLayout_6.setObjectName("horizontalLayout_6")
        spacerItem = QtWidgets.QSpacerItem(40, 20, QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Minimum)
        self.horizontalLayout_6.addItem(spacerItem)
        self.pbConnectLogin = QtWidgets.QPushButton(self.widget_8)
        self.pbConnectLogin.setObjectName("pbConnectLogin")
        self.horizontalLayout_6.addWidget(self.pbConnectLogin)
        self.pbCancelLogin = QtWidgets.QPushButton(self.widget_8)
        self.pbCancelLogin.setMaximumSize(QtCore.QSize(100, 16777215))
        self.pbCancelLogin.setObjectName("pbCancelLogin")
        self.horizontalLayout_6.addWidget(self.pbCancelLogin)
        self.verticalLayout.addWidget(self.widget_8)
        self.lblMessage = QtWidgets.QLabel(Login)
        sizePolicy = QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Preferred, QtWidgets.QSizePolicy.MinimumExpanding)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.lblMessage.sizePolicy().hasHeightForWidth())
        self.lblMessage.setSizePolicy(sizePolicy)
        self.lblMessage.setAlignment(QtCore.Qt.AlignLeading|QtCore.Qt.AlignLeft|QtCore.Qt.AlignTop)
        self.lblMessage.setWordWrap(True)
        self.lblMessage.setObjectName("lblMessage")
        self.verticalLayout.addWidget(self.lblMessage)

        self.retranslateUi(Login)
        QtCore.QMetaObject.connectSlotsByName(Login)

    def retranslateUi(self, Login):
        _translate = QtCore.QCoreApplication.translate
        Login.setWindowTitle(_translate("Login", "Login"))
        self.label.setText(_translate("Login", "Authorization required:"))
        self.txtPassword.setPlaceholderText(_translate("Login", "Password"))
        self.pbConnectLogin.setText(_translate("Login", "CONNECT"))
        self.pbCancelLogin.setText(_translate("Login", "CANCEL"))
        self.lblMessage.setText(_translate("Login", "Error"))

