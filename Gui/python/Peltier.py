import serial
from Gui.siteSettings import *
from PyQt5 import QtCore
from PyQt5.QtCore import *
from PyQt5 import QtSerialPort
from PyQt5.QtWidgets import QMessageBox
import time
import logging

# Class used to send and read commands from the Peltier controller
class Signals(QtCore.QObject):
    finishedSignal = pyqtSignal()
    passedSignal = pyqtSignal(bool)
    messageSignal = pyqtSignal(list)
    powerSignal = pyqtSignal(int)
    tempSignal = pyqtSignal(float)


class PeltierSignalGenerator():
    def __init__(self):
        self.ser = serial.Serial(defaultPeltierPort, defaultPeltierBaud)
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
        self.checksumError = ['*','X','X','X','X','X','X','X','X','c','0','^']
        self.buffer = [0,0,0,0,0,0,0,0,0,0,0,0]

    def twosCompliment(self, num):
        return pow(2,32) - abs(num)

    def stringToList(self, string):
        return [letter for letter in string]

    def possibleCommands(self):
        return self.commandDict.keys()

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
        print("SEND: " , command)
        for bit in command:
            self.ser.write(bit.encode())
        message, passed = self.recieveMessage()
        print("REC: " , message)
        return message, passed

    # Will recieve message but will only check if the command gave an error, will not decode the message
    def recieveMessage(self):
        connection = True
        buff = self.buffer.copy()
        for i in range(len(buff)):
            buff[i] = self.ser.read(1).decode('utf-8')
        if buff == self.checksumError:
            connection = False
            return buff, connection
        else:
            time.sleep(0.1)
            return buff, connection

# Worker that will be used to send commands to the Peltier
class signalWorker(QRunnable, PeltierSignalGenerator):
    def __init__(self, command, message):
        super().__init__()
        self.signal = Signals()
        self.command = command
        self.message = message

    def run(self):
        recievedMessage, passed = self.sendCommand(self.createCommand(self.command, self.message))
        try:
            self.signal.messageSignal.emit(recievedMessage)
            self.signal.finishedSignal.emit()
        except RuntimeError:
            pass



#Used to read power and temperature constantly
class tempPowerReading(QRunnable, PeltierSignalGenerator):
    def __init__(self):
        super().__init__()
        self.readTemp = True
        self.signal = Signals()

    def run(self):
        while self.readTemp:
            temperature, passTemp = self.sendCommand(self.createCommand('Input1', ['0','0','0','0','0','0','0','0']))
            power, passPower = self.sendCommand(self.createCommand('Power On/Off Read' ,['0','0','0','0','0','0','0','0']))
            temp = "".join(temperature[1:9])
            temp = int(temp,16)/100
            power = int(power[8])

            try:
                self.signal.powerSignal.emit(power)
                self.signal.tempSignal.emit(temp)
            except RuntimeError:
                self.readTemp=False
                print("Temperature and power are no longer being read")
            time.sleep(0.5)
        print(self.readTemp, "Stop power and temp reading")


class startupWorker(QRunnable, PeltierSignalGenerator):
    def __init__(self):
        super().__init__()
        self.signal = Signals()

    def run(self):
        print("Running thread")
        self.sendCommand(self.createCommand('Set Type Define Write', ['0','0','0','0','0','0','0','1']))
        self.sendCommand(self.createCommand('Power On/Off Write' ,['0','0','0','0','0','0','0','0']))
        self.signal.finishedSignal.emit()
        message, passed = self.sendCommand(self.createCommand('Control Output Polarity Read', ['0','0','0','0','0','0','0','0']))
        self.signal.messageSignal.emit(message)


class PeltierController():
    def __init__(self, timeout=1):
        super().__init__()
        self.error = False

    def checkPower(self):
        message, passed = self.sendCommand(self.createCommand('Power On/Off Read', ['0','0','0','0','0','0','0','0']))
        if not passed:
            return
        return message[-4]

    def readTemperature(self):
        command = self.createCommand('Input1', ['0','0','0','0','0','0','0','0'])
        message, passed =self.sendCommand(command)
        if passed:
            message = message[1:9]
            message = "".join(message) # Converts the list of digits to single string
        #    a, b = self.sendCommand(self.createCommand('Control Type Read', ['0','0','0','0','0','0','0','0']))
        #    print(a)

        #    Peltier will return two's compliment of negative numbers.
        #    Hopefully, 500C is a temperature that is never encountered in the lab, therefore if  the temp is greater than 500
        #    take the twos compliment
            message = int(message,16)/100
            if message > 1000:
                return -1 * self.twosCompliment(message)
            else:
                return message
        else:
            print("Couldn't read temperature")
            return

    def powerController(self, power):
        _,_ = self.sendCommand(self.createCommand('Power On/Off Write', ['0','0','0','0','0','0','0',str(power)]))

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
            return value
            #command = self.createCommand('Fixed Desired Control Setting Write', value)
            #_,_ = self.sendCommand(command)
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
        print("CMD:",command)
        return command
    
    def sendCommand(self, command):
        for bit in command:
            self.ser.write(bit.encode())
        message, passed = self.recieveMessage()
        print("REC:",message)
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

    @QtCore.pyqtSlot()
    def receive(self):
        while self.serial.canReadLine():
            try:
                text = self.serial.readLine().data().decode("utf-8","ignore")
                print(text)

            except Exception as err:
                logger.error("{0}".format(err))





if __name__ == "__main__":
    # If your port and/or baud rate are different change these parameters
    pelt = PeltierController('/dev/ttyUSB0', 9600)
    pelt.setTemperature(defaultPeltierSetTemp)
    while True:
        time.sleep(1)
        print(pelt.readTemperature())
        print(pelt.readSetTemperature())
