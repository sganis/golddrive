#!/usr/bin/python3
# -*- coding: utf-8 -*-

from PyQt5.QtWidgets import (QWidget, QLabel, 
    QComboBox, QApplication, QMainWindow)
import sys
from app_ui import Ui_MainWindow

class Window(Ui_MainWindow, QMainWindow):
    
    def __init__(self):
        super().__init__()
        self.setupUi(self)
        self.show()
        
    # def initUI(self):      

    #     self.lbl = QLabel("Ubuntu", self)

    #     combo = QComboBox(self)
    #     combo.addItem("Ubuntu")
    #     combo.addItem("Mandriva")
    #     combo.addItem("Fedora")
    #     combo.addItem("Arch")
    #     combo.addItem("Gentoo")

    #     combo.move(50, 50)
    #     self.lbl.move(50, 150)

    #     combo.activated[str].connect(self.onActivated)        
         
    #     self.setGeometry(300, 300, 300, 200)
    #     self.setWindowTitle('QComboBox')
    #     self.show()

        
def run():
    
    app = QApplication(sys.argv)
    ex = Window()
    sys.exit(app.exec_())



if __name__ == '__main__':
    run()    
