# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file '..\assets\ui\host.ui'
#
# Created by: PyQt5 UI code generator 5.11.2
#
# WARNING! All changes made in this file will be lost!

from PyQt5 import QtCore, QtGui, QtWidgets

class Ui_Host(object):
    def setupUi(self, Host):
        Host.setObjectName("Host")
        Host.resize(313, 208)
        self.verticalLayout = QtWidgets.QVBoxLayout(Host)
        self.verticalLayout.setSpacing(10)
        self.verticalLayout.setObjectName("verticalLayout")
        self.label = QtWidgets.QLabel(Host)
        self.label.setObjectName("label")
        self.verticalLayout.addWidget(self.label)
        self.widget = QtWidgets.QWidget(Host)
        self.widget.setObjectName("widget")
        self.horizontalLayout = QtWidgets.QHBoxLayout(self.widget)
        self.horizontalLayout.setContentsMargins(0, 0, 0, 0)
        self.horizontalLayout.setObjectName("horizontalLayout")
        self.cboDrive = QtWidgets.QComboBox(self.widget)
        self.cboDrive.setObjectName("cboDrive")
        self.horizontalLayout.addWidget(self.cboDrive)
        spacerItem = QtWidgets.QSpacerItem(184, 20, QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Minimum)
        self.horizontalLayout.addItem(spacerItem)
        self.verticalLayout.addWidget(self.widget)
        self.label_2 = QtWidgets.QLabel(Host)
        self.label_2.setObjectName("label_2")
        self.verticalLayout.addWidget(self.label_2)
        self.txtHost = QtWidgets.QLineEdit(Host)
        self.txtHost.setObjectName("txtHost")
        self.verticalLayout.addWidget(self.txtHost)
        self.widget_8 = QtWidgets.QWidget(Host)
        self.widget_8.setObjectName("widget_8")
        self.horizontalLayout_6 = QtWidgets.QHBoxLayout(self.widget_8)
        self.horizontalLayout_6.setContentsMargins(0, 0, 0, 0)
        self.horizontalLayout_6.setSpacing(10)
        self.horizontalLayout_6.setObjectName("horizontalLayout_6")
        spacerItem1 = QtWidgets.QSpacerItem(40, 20, QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Minimum)
        self.horizontalLayout_6.addItem(spacerItem1)
        self.pbConnectHost = QtWidgets.QPushButton(self.widget_8)
        self.pbConnectHost.setMaximumSize(QtCore.QSize(100, 16777215))
        self.pbConnectHost.setObjectName("pbConnectHost")
        self.horizontalLayout_6.addWidget(self.pbConnectHost)
        self.pbCancelHost = QtWidgets.QPushButton(self.widget_8)
        self.pbCancelHost.setMaximumSize(QtCore.QSize(100, 16777215))
        self.pbCancelHost.setObjectName("pbCancelHost")
        self.horizontalLayout_6.addWidget(self.pbCancelHost)
        self.verticalLayout.addWidget(self.widget_8)
        self.lblMessage = QtWidgets.QLabel(Host)
        sizePolicy = QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Preferred, QtWidgets.QSizePolicy.MinimumExpanding)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.lblMessage.sizePolicy().hasHeightForWidth())
        self.lblMessage.setSizePolicy(sizePolicy)
        self.lblMessage.setAlignment(QtCore.Qt.AlignLeading|QtCore.Qt.AlignLeft|QtCore.Qt.AlignTop)
        self.lblMessage.setWordWrap(True)
        self.lblMessage.setObjectName("lblMessage")
        self.verticalLayout.addWidget(self.lblMessage)

        self.retranslateUi(Host)
        QtCore.QMetaObject.connectSlotsByName(Host)

    def retranslateUi(self, Host):
        _translate = QtCore.QCoreApplication.translate
        Host.setWindowTitle(_translate("Host", "Host"))
        self.label.setText(_translate("Host", "Drive"))
        self.label_2.setText(_translate("Host", "Server"))
        self.txtHost.setPlaceholderText(_translate("Host", "Hostname or IP address"))
        self.pbConnectHost.setText(_translate("Host", "CONNECT"))
        self.pbCancelHost.setText(_translate("Host", "CANCEL"))
        self.lblMessage.setText(_translate("Host", "Hostname can be [user@]host[:port]"))

