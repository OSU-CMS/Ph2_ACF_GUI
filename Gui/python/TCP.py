
import logging

# Customize the logging configuration
logging.basicConfig(
   level=logging.INFO,
   format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
   filename='my_project.log',  # Specify a log file
   filemode='w'  # 'w' for write, 'a' for append
)

logger = logging.getLogger(__name__)

import ctypes
import subprocess, os, signal
import time, datetime
#g++ -std=c++1y -fPIC -shared *.cc -o NetworkUtils.so -lm -lboost_regex
#from PySide2.QtCore import QThreadPool, Signal, QObject
from PyQt5.QtCore import *

class TCPServer():
    def __init__( self, pFolder = None, index = 0):
        print("Ph2_ACF_GUI:\tNew TCP Server")
        self.index = index
        self.folder = pFolder
        #self.serverSubprocess = None
        self.config = None
        #self.killAnyPowerSupplyProcess()

        self.serverProcess = None
        self.logFile = open(os.environ.get('GUI_dir')+"/Gui/.tmp/" + str(self.index) + ".txt", "w")

    def start( self , pConfigFile ) :
        self.config = pConfigFile

        if self.folder is not None:
            print("Ph2_ACF_GUI:\tTCP Server: Start TCP Device Server at " + self.folder + " with config file " + self.config)
            if not self.isRunning():
                print("Ph2_ACF_GUI:\tStarting TCP server ...")
                cmd = "source setup.sh && " + os.environ.get('GUI_dir')+ "/power_supply/bin/PowerSupplyController -c " + self.config + " -p " + str( 7000 + self.index) 
                self.serverProcess = subprocess.Popen(cmd, stdout=self.logFile, stderr=subprocess.PIPE, encoding = 'UTF-8', shell= True, executable="/bin/bash", cwd=self.folder)
                self.isRunning()
                return True
            else:
                print("Ph2_ACF_GUI:\tTCP Device Server already running")
                return False
        else:
            print("Ph2_ACF_GUI:\tNo folder given for TCP Device Server, abort!")
            return False

    def checkDeviceSubProcess ( self ):
        search = subprocess.Popen(['ps', '-A'], stdout=subprocess.PIPE)
        output, error = search.communicate()
        target_process = "PowerSupplyCont"

        for line in output.splitlines():
            #print(str(line))
            if target_process in str(line):
                pid = int(line.split(None, 1)[0])
                #print("PID:" + str(pid))


        if self.serverProcess is not None:
            response = self.serverProcess.poll()
            #print( "Response: " + str( response) )
            print("Ph2_ACF_GUI:\tCheck subprocess of device " + self.config + ": is running")
            return  response is None
        else:
            print("Ph2_ACF_GUI:\tCheck subprocess of device " + self.config + ": is not running")
            return False   

        

    def killAnyPowerSupplyProcess( self ):
        search = subprocess.Popen(['ps', '-A'], stdout=subprocess.PIPE)
        output, error = search.communicate()
        target_process = "PowerSupplyCont"

        for line in output.splitlines():
            #print(str(line))
            if target_process in str(line):
                pid = int(line.split(None, 1)[0])
                #print("PID")
                #print(pid)

                os.kill(pid, 9)
                return True
        return False
    def stop ( self ):
        print("TCP Server: Kill TCP Device Server at " + self.folder + " with config file " + self.config)

        return self.killAnyPowerSupplyProcess()
        #os.kill(self.serverSubprocess.pid, signal.SIGHUP) 
        #os.kill(self.serverSubprocess.pid, signal.SIGTERM)

    def isRunning( self):
        return self.checkDeviceSubProcess()


class TCPClient(QObject):
    tcpAnswer = pyqtSignal(object)

    def __init__( self,index = 0 ):
        super(TCPClient, self).__init__(  )
        self.index = index
        print("Ph2_ACF_GUI:\tNew TCP Client " + str( self.index ))

        self.lib = ctypes.cdll.LoadLibrary(os.environ.get('GUI_dir') + '/power_supply/NetworkUtils/NetworkUtils.so')

        self.lib.TCPClient_new.argtypes = [ctypes.c_char_p,ctypes.c_int]
        self.lib.TCPClient_new.restype = ctypes.c_void_p

        self.lib.sendAndReceivePacket_new.argtypes =[ctypes.c_void_p,ctypes.c_char_p]
        self.lib.sendAndReceivePacket_new.restype = ctypes.c_char_p

        #self.lib.connect_new.restype = ctype.c_void_p
        self.lib.connect_new.restype = ctypes.c_bool

        self.lib.disconnect_new.restype = ctypes.c_bool

        #self.client = self.lib.TCPClient_new(b'127.0.0.1',7000 + self.index)
        self.client = self.lib.TCPClient_new(b'0.0.0.0',7000 + self.index)

        self.timeOut = 100  #Milliseconds
        self.lastPoll = datetime.datetime.now()
        self.lastData = None


    def isRunning( self ):
        return True

    def connectClient ( self ):
        self.lib.connect_new(self.client,2)# CLient and number of retries

    def disconnectClient( self ):
        self.lib.disconnect_new(self.client)


    def sendAndReceivePacket( self, pBuffer ):
        #There is only one TCP Client if last call is not long ago, just return most recent value to avoid polling too often
        #result = self.lib.sendAndReceivePacket_new( self.client, pBuffer.encode() ).decode()
        result = self.lib.sendAndReceivePacket_new(self.client, pBuffer.encode() )
        result = result.decode()
        if result == "-1" or result == '':
            result = None
        self.tcpAnswer.emit(result)


    def decodeStatus( self, pStatus ):
        statusList = pStatus.split(',')

        timestamp = None
        statusDict = {}


        splitTimestamp = statusList[0].split(':')
        timestamp = splitTimestamp[1] + ":" + splitTimestamp[2] + ":" + splitTimestamp[3]

        for entry in statusList[1:]:
            device = entry.split('_')[0]
            statusDict[device] = {}

        for entry in statusList[1:]:
            device = entry.split('_')[0]
            channel = entry.split('_')[1]
            statusDict[device][channel] = {}

        for entry in statusList[1:]:
            device = entry.split('_')[0]
            channel = entry.split('_')[1]
            meas, value = entry.split('_')[2].split(':')
            statusDict[device][channel][meas]=value

        return timestamp, statusDict
