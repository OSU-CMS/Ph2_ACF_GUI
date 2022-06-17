import serial
from Gui.GUIutils.settings import *
from PyQt5.QtWidgets import QMessageBox
import random

class PeltierController:
    def __init__(self, port, baud, timeout=1):
        self.error = False
        # What the commands do can be found in the TC-36--25 RS232 manual 
        self.commandDict = {'Input1':['0','1'],
                            'Desired Control Value':['0','3'],
                            'Input2':['0','6'],
                            'Set Type Define Write': ['2','9'],
                            'Set Type Define Read': ['4','2'],
                            'Power On/Off Write': ['2','d'],
                            'Power On/Off Read': ['4','6'],
                            'Fixed Desired Control Setting Write':['1','c'],
                            'Fixed Desired Control Setting Read' : ['5','0'],
                            'Proportional Bandwidth Write': ['1','d'],
                            'Proportional Bandwidth Read' : ['5','1'],
                            'Choose Temperature Units Write':['3','2'],
                            'Choose Temperature Units Read' : ['4','b'],
                            } 
        self.buffer = [0,0,0,0,0,0,0,0,0,0,0,0] # Used to read the messages from peltier
        
        try:
            self.ser = serial.Serial(port, baud, timeout=1) # Setting up the connection to peltier
            self.setupConnection()
        except Exception:
            self.error = True
            self.mesg = QMessageBox()
            self.mesg.setText("Can't open port, check connection")
            self.mesg.exec()

        
    def readTemperature(self):
        command = ['*','0','0','0','1','0','0','0','0','0','0','0','0','4','1','\r']
        self.sendCommand(command)
        message = self.recieveMessage
        message = message[1:9]
        message = "".join(message) # Converts the list of digits to single string 
        return int(message,16)/100 


    # Used to create the message that will set the "desire control setting"
    def setTemperature(self, temp):
        try:
            stx = ['*']
            aa = ['0','0']
            cc = self.commandDict['Fixed Desired Control Setting Write']
            etx = ['\r']
            value = ['0','0','0','0','0','0','0','0']
            temp *= 100
            temp = int(temp)
            if temp < 0:
                temp = self.twosCompliment(temp)
            temp = self.convertToHex(temp)
            temp = self.stringToList(temp)
            cutoff = temp.index('x')
            temp = temp[cutoff+1:]
            for i, _ in enumerate(temp):
                value[-(i+1)] = temp[-(i+1)]
            ss = self.checksum(aa + cc + value)
            message = stx + aa + cc + value + ss + etx
            self.sendCommand(message)
            print(f"Set temp to {temp}")
        except Exception:
            self.error = True
            pass

    # Finds the twos compliment necessary for negative temperatures
    def twosCompliment(self, num):
        return pow(2,32) - abs(num)

    def stringToList(self, string):
        return [letter for letter in string]

    def possibleCommands(self):
        return self.commandDict.keys()

    # Sets the peltier to have a computer communicated set value
    def setupConnection(self):
        buff = self.buffer.copy()
        message = ['*','0','0','2','9','0','0','0','0','0','0','0','0','4','b','\r']
        for bit in message:
            self.ser.write(bit)
        for i, bit in enumerate(buff):
            buff[i] = self.ser.read(1)
        if buff == ['*','0','0','0','0','0','0','0','0','8','0','^']:
            print('Complete')
        else:
            print('Issue with reading or writing message from/to controller')
        self.buffer_reset()    
        return buff
    
    def convertToHex(self,val):
        if type(val) != list:
            return hex(val) 
        for i, item in enumerate(val):
            val[i] = hex(ord(item))
        return val

    def convertHexToDec(self,hex):
        if type(hex) != list:
            return int(hex,16)
        else:
            for i, val in enumerate(hex):
                hex[i] = int(val,16)
            return hex
                
    # Calculates the cheksum value that's necessary when sending a message
    def checksum(self, command):
        command = self.convertToHex(command)
        total = '0'
        for item in command:
            total = hex(int(total, 16) + int(item, 16))
        
        return [total[-2], total[-1]]

# Used for all other commands that are not setTemp
# Currently you need to format dd yourself which is the input value you want to send 
    def createCommand(self, command, dd):
        stx = ['*']
        aa = ['0','0']
        cc = self.commandDict[command]
        check = aa + cc + dd
        ss = self.checksum(aa + cc + dd)
        etx = ['/r']
        return stx + aa + cc + dd + ss + etx
    
    def sendCommand(self, command):
        for bit in command:
            self.ser.write(bit)
    
    def recieveMessage(self):
        buff = self.buffer.copy()
        for i in range(len(buff)):
            buff[i] = self.ser.read(1)
        if buff == ['*','X','X','X','X','X','X','X','X','c','0','^']:
            self.badChecksumMessage = QMessageBox()
            self.badChecksumMessage.setText("Bad Checksum Error")
            self.badChecksumMessage.setIcon(3) # Sets the icon to critical error icon
            self.badChecksumMessage.exec()
        else:
            return buff

if __name__ == "__main__":
    # If your port and/or baud rate are different change these parameters
    pelt = PeltierController(peltierPort,peltierBaud)

