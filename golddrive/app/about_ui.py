# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file '..\assets\ui\about.ui'
#
# Created by: PyQt5 UI code generator 5.11.2
#
# WARNING! All changes made in this file will be lost!

from PyQt5 import QtCore, QtGui, QtWidgets

class Ui_About(object):
    def setupUi(self, About):
        About.setObjectName("About")
        About.resize(298, 189)
        self.verticalLayout = QtWidgets.QVBoxLayout(About)
        self.verticalLayout.setObjectName("verticalLayout")
        self.lblIcon = QtWidgets.QLabel(About)
        sizePolicy = QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Preferred, QtWidgets.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.lblIcon.sizePolicy().hasHeightForWidth())
        self.lblIcon.setSizePolicy(sizePolicy)
        self.lblIcon.setText("")
        self.lblIcon.setPixmap(QtGui.QPixmap(":/images/icon_64.png"))
        self.lblIcon.setAlignment(QtCore.Qt.AlignCenter)
        self.lblIcon.setObjectName("lblIcon")
        self.verticalLayout.addWidget(self.lblIcon)
        self.lblAbout = QtWidgets.QLabel(About)
        self.lblAbout.setAlignment(QtCore.Qt.AlignCenter)
        self.lblAbout.setWordWrap(True)
        self.lblAbout.setObjectName("lblAbout")
        self.verticalLayout.addWidget(self.lblAbout)
        self.widget = QtWidgets.QWidget(About)
        self.widget.setObjectName("widget")
        self.horizontalLayout = QtWidgets.QHBoxLayout(self.widget)
        self.horizontalLayout.setObjectName("horizontalLayout")
        self.pbAboutOk = QtWidgets.QPushButton(self.widget)
        self.pbAboutOk.setMaximumSize(QtCore.QSize(100, 16777215))
        self.pbAboutOk.setObjectName("pbAboutOk")
        self.horizontalLayout.addWidget(self.pbAboutOk)
        self.verticalLayout.addWidget(self.widget)
        spacerItem = QtWidgets.QSpacerItem(20, 40, QtWidgets.QSizePolicy.Minimum, QtWidgets.QSizePolicy.Expanding)
        self.verticalLayout.addItem(spacerItem)

        self.retranslateUi(About)
        QtCore.QMetaObject.connectSlotsByName(About)

    def retranslateUi(self, About):
        _translate = QtCore.QCoreApplication.translate
        About.setWindowTitle(_translate("About", "About"))
        self.lblAbout.setText(_translate("About", "About"))
        self.pbAboutOk.setText(_translate("About", "OK"))

import resources_rc
