import serial
from Gui.GUIutils.settings import *
from PyQt5.QtWidgets import QMessageBox
from PyQt5.QtCore import QObject, QThread, pyqtSignal
import time

class startupWorker(QObject):
    finished = pyqtSignal()
    progress = pyqtSignal(int)

    def run(self, port, baud):
        ser = serial.Serial(port, baud) # Setup device as IO
        command = ['*', '0','0','2','9','0','0','0','0','0','0','0','0','4','b','\r'] # Run command to Set Type Define to computer communicated

        # Read in the message and check if you recieve the correct message back
        for bit in command:
            ser.write(bit.encode())
        buff =['0','0','0','0','0','0','0','0','0','0','0','0']
        for i in range(len(buff)):
            buff[i] = ser.read(1).decode('utf-8')
        # If you recieve a checksum error display warning message
        if buff == ['*','X','X','X','X','X','X','X','X','c','0','^']:
            self.badChecksumMessage = QMessageBox()
            self.badChecksumMessage.setText("Bad Checksum Error")
            self.badChecksumMessage.setIcon(3) # Sets the icon to critical error icon
            self.badChecksumMessage.exec()

        # Sends the command to turn off the Peltier
        command = ['*', '0','0','2','d','0','0','0','0','0','0','0','0','7','6','\r']
        for bit in command:
            ser.write(bit.encode())
        buff =['0','0','0','0','0','0','0','0','0','0','0','0']
        for i in range(len(buff)):
            buff[i] = ser.read(1).decode('utf-8')
        # If you recieve a bad checksum error give warning message
        if buff == ['*','X','X','X','X','X','X','X','X','c','0','^']:
            self.badChecksumMessage = QMessageBox()
            self.badChecksumMessage.setText("Bad Checksum Error")
            self.badChecksumMessage.setIcon(3) # Sets the icon to critical error icon
            self.badChecksumMessage.exec()
        # Once this thread is finished emit a signal
        self.finished.emit()
        print("thread finished")


class PeltierController:
    def __init__(self, port, baud, timeout=1):
        self.error = False
        self.port = port
        self.baud = baud
        self.ser = serial.Serial(self.port, self.baud)
        # What the commands do can be found in the TC-36--25 RS232 manual 
        self.commandDict = {'Input1':['0','1'],
                            'Desired Control Value':['0','3'],
                            'Input2':['0','6'],
                            'Set Type Define Write': ['2','9'],
                            'Set Type Define Read': ['4','2'],
                            'Control Type Write' : ['2', 'b'],
                            'Control Type Read' : ['4','4'],
                            'Power On/Off Write': ['2','d'],
                            'Power On/Off Read': ['4','6'],
                            'Fixed Desired Control Setting Write':['1','c'],
                            'Fixed Desired Control Setting Read' : ['5','0'],
                            'Proportional Bandwidth Write': ['1','d'],
                            'Proportional Bandwidth Read' : ['5','1'],
                            'Choose Temperature Units Write':['3','2'],
                            'Choose Temperature Units Read' : ['4','b'],
                            'Over Current Count Compare Write':['0', 'e'],
                            'Over Current Count Compare Read' : ['5', 'e'],
                            'Over Current Continuous Write' : ['3', '5'],
                            'Over Current Continuous Read' : ['4','d'],
                            'Control Output Polarity Write' : ['2','c'],
                            'Control Output Polarity Read' : ['4','5']
                            } 
        self.buffer = [0,0,0,0,0,0,0,0,0,0,0,0] # Used to read the messages from peltier
        
        try:
            self.setupConnection()
        except Exception as e:
            print(e)
            self.error = True
            self.mesg = QMessageBox()
            self.mesg.setText("Can't open port, check connection")
            self.mesg.exec()
    # Queries controller for power state of Peltier. Will return 0 if power is off, 1 if on
    def checkPower(self):
        message, passed = self.sendCommand(self.createCommand('Power On/Off Read', ['0','0','0','0','0','0','0','0']))
        print(message)
        if not passed:
            return
        return message[-4]

    def readTemperature(self):
        command = self.createCommand('Input1', ['0','0','0','0','0','0','0','0'])
        message, passed =self.sendCommand(command)
        if passed:
            message = message[1:9]
            message = "".join(message) # Converts the list of digits to single string
            return int(message,16)/100
        else:
            print("Couldn't read temperature")
            return

    def powerController(self, power):
        _,_ = self.sendCommand(self.createCommand('Power On/Off Write', ['0','0','0','0','0','0','0',power]))

    def changePolarity(self):
        # Read in the polarity
        message, passed = self.sendCommand(self.createCommand('Control Output Polarity Read',['0','0','0','0','0','0','0','0']))
        if not passed:
            print("Checksum error while reading polarity")
            return
        if message[-4] == '0':
            _, passed = self.sendCommand(self.createCommand('Control Output Polarity Write',['0','0','0','0','0','0','0','1']))
            if not passed:
                print("Checksum error while trying to change polarity")
            polarity = 'WP1+ and WP2-'
        elif message[-4] == '1':
            _, passed = self.sendCommand(self.createCommand('Control Output Polarity Write', ['0','0','0','0','0','0','0','0']))
            if not passed:
                print("Checksum error while trying to change polarity")
            polarity = 'WP2+ and WP1-'
        else:
            print("Unknown message recieved from the controller while trying to read polarity")
            return
        return polarity

    # Used to create the message that will set the "desire control setting"
    def setTemperature(self, temp):
        try:
            value = ['0','0','0','0','0','0','0','0']
            tempDec = temp
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
            command = self.createCommand('Fixed Desired Control Setting Write', value)
            _,_ = self.sendCommand(command)
        except Exception as e:
            print("Exception while trying to set temperature: " ,e)
            self.error = True
            return
    
    def readSetTemperature(self):
        command = self.createCommand('Fixed Desired Control Setting Read',['0','0','0','0','0','0','0','0'])
        message, passed = self.sendCommand(command)
        message = message[-11:-3]
        message = "".join(message) # Converts the list of digits to single string
        message = int(message,16)/100
        return message

    # Finds the twos compliment necessary for negative temperatures
    def twosCompliment(self, num):
        return pow(2,32) - abs(num)

    def stringToList(self, string):
        return [letter for letter in string]

    def possibleCommands(self):
        return self.commandDict.keys()

    # Sets the peltier to have a computer communicated set value
    def setupConnection(self):
       self.thread = QThread()
       self.worker = startupWorker()
       self.worker.moveToThread(self.thread)
       self.thread.started.connect(lambda: self.worker.run(self.port, self.baud))
       self.worker.finished.connect(self.thread.quit)
       self.worker.finished.connect(self.worker.deleteLater)
       self.thread.finished.connect(self.thread.deleteLater)
       self.thread.start()

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
        checksum = [total[-2], total[-1]]
        return checksum

# Used for all other commands that are not setTemp
# Currently you need to format dd yourself which is the input value you want to send
    def createCommand(self, command, dd):
        stx = ['*']
        aa = ['0','0']
        cc = self.commandDict[command]
        check = aa + cc + dd
        ss = self.checksum(aa + cc + dd)
        etx = ['\r']
        command = stx + aa + cc + dd + ss + etx

        return command
    
    def sendCommand(self, command):
        print("Command sent", command)
        for bit in command:
            self.ser.write(bit.encode())
        message, passed = self.recieveMessage()
        return message, passed

    # Will recieve message but will only check if the command gave an error, will not decode the message
    def recieveMessage(self):
        connection = True
        buff = self.buffer.copy()
        for i in range(len(buff)):
            buff[i] = self.ser.read(1).decode('utf-8')
        if buff == ['*','X','X','X','X','X','X','X','X','c','0','^']:
            connection = False
            self.badChecksumMessage = QMessageBox()
            self.badChecksumMessage.setText("Bad Checksum Error")
            self.badChecksumMessage.setIcon(3) # Sets the icon to critical error icon
            self.badChecksumMessage.exec()
            return buff, connection
        else:
            time.sleep(0.1)
            return buff, connection

if __name__ == "__main__":
    # If your port and/or baud rate are different change these parameters
    pelt = PeltierController('/dev/ttyUSB0', 9600)
    pelt.setTemperature(20)
    while True:
        time.sleep(1)
        print(pelt.readTemperature())
