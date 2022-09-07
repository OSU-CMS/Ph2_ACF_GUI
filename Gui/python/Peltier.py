import serial
from Gui.GUIutils.settings import *
from PyQt5.QtWidgets import QMessageBox
import time
class PeltierController:
    def __init__(self, port, baud, timeout=1):
        self.error = False
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
                            } 
        self.buffer = [0,0,0,0,0,0,0,0,0,0,0,0] # Used to read the messages from peltier
        
        try:
            self.ser = serial.Serial(port , baud, timeout=5) # Setting up the connection to peltier
            self.setupConnection()
            self.sendCommand(self.createCommand('Power On/Off Write', ['0','0','0','0','0','0','0','0']))
        except Exception as e:
            print(e)
            self.error = True
            self.mesg = QMessageBox()
            self.mesg.setText("Can't open port, check connection")
            self.mesg.exec()


    def readTemperature(self):
        command = ['*','0','0','0','1','0','0','0','0','0','0','0','0','4','1','\r']
        self.sendCommand(command)
        message, passed = self.recieveMessage()
        if passed:
            message = message[1:9]
            message = "".join(message) # Converts the list of digits to single string
            return int(message,16)/100
        else:
            print("Couldn't read temperature")
            return


    # Used to create the message that will set the "desire control setting"
    def setTemperature(self, temp):
        try:
            # self.sendCommand(self.createCommand('Fixed Desired Control Setting Write', ['']))
            # stx = ['*']
            # aa = ['0','0']
            # cc = self.commandDict['Fixed Desired Control Setting Write']
            # etx = ['\r']

            # Check if the Peltier is already turned on
            # self.sendCommand(self.createCommand('Power On/Off Read', ['0','0','0','0','0','0','0','0']))
            # if self.recieveMessage()[0][-4] == '0':
              #   command = self.createCommand('Power On/Off Write', ['0','0','0','0','0','0','0','1'])
                # if self.recieveMessage()[0][-4] == '0':
                  #   print("Peltier did not turn on after given command")

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
            command = ['*', '0', '0', '1', 'c','f','f','f','f','f','f','6','a','e','f','\r'] #Just sets temp to -1.50 
        #    command = self.createCommand('Fixed Desired Control Setting Write', value)
            self.sendCommand(command)
            # ss = self.checksum(aa + cc + value)
            # message = stx + aa + cc + value + ss + etx
            # self.sendCommand(message)
            #print(f"Set temp to {tempDec}")
            #print('This is the value sent (hex values): ',value)
            message , passed = self.recieveMessage()
            print("This is the message recieved", message)
        except Exception as e:
            print("Exception while trying to set temperature: " ,e)
            self.error = True
            return
    def readSetTemperature(self):
        command = self.createCommand('Fixed Desired Control Setting Read',['0','0','0','0','0','0','0','0'])
        self.sendCommand(command)
        message, passed = self.recieveMessage()
        print("This is the message recieved for the set Temp", message)

    # Finds the twos compliment necessary for negative temperatures
    def twosCompliment(self, num):
        return pow(2,32) - abs(num)

    def stringToList(self, string):
        return [letter for letter in string]

    def possibleCommands(self):
        return self.commandDict.keys()

    # Sets the peltier to have a computer communicated set value
    def setupConnection(self):
        command = self.createCommand('Set Type Define Write', ['0','0','0','0','0','0','0','0'])
        self.sendCommand(command)
        buff, passed = self.recieveMessage()
        if passed:
            return buff
        else:
            print("Failed to setup connection")
            return
        # message = ['*','0','0','2','9','0','0','0','0','0','0','0','0','4','b','\r']
        # for bit in message:
          #   self.ser.write(bit.encode())
        # for i, bit in enumerate(buff):
            #print(self.ser.read(1))
          #   buff[i] = self.ser.read(1).decode('utf-8')
        # if buff == ['*','0','0','0','0','0','0','0','0','8','0','^']:
          #   print('Complete')
        # else:
          #   print(buff)

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
        print(type(total[0]))        
        checksum = [total[-2], total[-1]]
        return checksum

# Used for all other commands that are not setTemp
# Currently you need to format dd yourself which is the input value you want to send
    def createCommand(self, command, dd):
        stx = ['*']
        aa = ['0','0']
        cc = self.commandDict[command]
        print(cc)
        check = aa + cc + dd
        ss = self.checksum(aa + cc + dd)
        etx = ['/r']
        command = stx + aa + cc + dd + ss + etx
        print(command)

        return command
    
    def sendCommand(self, command):
        for bit in command:
            self.ser.write(bit.encode())
        # print("Command Sent to Peltier")

    # Will recieve message but will only check if the command gave an error, will not decode the message
    def recieveMessage(self):
        time.sleep(0.004)
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
            print('buff', buff)
            return buff, connection

if __name__ == "__main__":
    # If your port and/or baud rate are different change these parameters
    pelt = PeltierController('/dev/ttyUSB0', 9600)
    pelt.setTemperature(20)
    while True:
        time.sleep(1)
        print(pelt.readTemperature())
