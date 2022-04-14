from PyQt5.QtCore import *
import os, re, subprocess, errno

from Gui.python.TCP import *

import threading

class TCP_Interface(QObject):
    update = pyqtSignal ( object , object )

    def __init__( self, pPackageFolder,pConfigFile ):
        super(TCP_Interface, self).__init__(  )
        print("PH2_ACF_GUI:\tNew TCP Interface")
        self.server = TCPServer(pPackageFolder)
        self.server.start(pConfigFile)
        self.client = TCPClient()
        self.client.connectClient()
        self.client.tcpAnswer.connect(self.handleAnswer)

    def executeCommand( self , pCmd, pPrintLog = False):
        self.client.sendAndReceivePacket(pCmd)

    def handleAnswer( self, pAnswer ):
        print("answer:",pAnswer)
        if pAnswer is not None:
            self.update.emit("data", pAnswer)
        else:
            self.update.emit("noData", None)

    #To be defined    
    def stopTask( self ):
        pass
