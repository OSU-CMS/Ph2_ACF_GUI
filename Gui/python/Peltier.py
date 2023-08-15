#!/usr/bin/env python3

# This file contains the functions that help facilitate communication with the Peltier controller

import serial
import Gui.Config.siteSettings as ss
from PyQt5 import QtCore
from PyQt5.QtCore import *
from PyQt5 import QtSerialPort
from PyQt5.QtWidgets import QMessageBox
import time
import logging


class PeltierSignalGenerator:
    def __init__(self):
        self.ser = serial.Serial(ss.defaultPeltierPort, ss.defaultPeltierBaud)
        self.commandDict = {
            "Input1": ["0", "1"],
            "Desired Control Value": ["0", "3"],
            "Input2": ["0", "6"],
            "Set Type Define Write": ["2", "9"],
            "Set Type Define Read": ["4", "2"],
            "Control Type Write": ["2", "b"],
            "Control Type Read": ["4", "4"],
            "Power On/Off Write": ["2", "d"],
            "Power On/Off Read": ["4", "6"],
            "Fixed Desired Control Setting Write": ["1", "c"],
            "Fixed Desired Control Setting Read": ["5", "0"],
            "Proportional Bandwidth Write": ["1", "d"],
            "Proportional Bandwidth Read": ["5", "1"],
            "Choose Temperature Units Write": ["3", "2"],
            "Choose Temperature Units Read": ["4", "b"],
            "Over Current Count Compare Write": ["0", "e"],
            "Over Current Count Compare Read": ["5", "e"],
            "Over Current Continuous Write": ["3", "5"],
            "Over Current Continuous Read": ["4", "d"],
            "Control Output Polarity Write": ["2", "c"],
            "Control Output Polarity Read": ["4", "5"],
        }
        self.checksumError = [
            "*",
            "X",
            "X",
            "X",
            "X",
            "X",
            "X",
            "X",
            "X",
            "c",
            "0",
            "^",
        ]
        self.buffer = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]

    def twosCompliment(self, num: int) -> int:
        return pow(2, 32) - abs(num)

    def stringToList(self, string):
        return [letter for letter in string]

    def possibleCommands(self):
        return self.commandDict.keys()

    def convertToHex(self, val):
        if type(val) != list:
            return hex(val)
        for i, item in enumerate(val):
            val[i] = hex(ord(item))
        return val

    def convertHexToDec(self, hex):
        if type(hex) != list:
            return int(hex, 16)
        else:
            for i, val in enumerate(hex):
                hex[i] = int(val, 16)
            return hex

    # Calculates the cheksum value that's necessary when sending a message
    def checksum(self, command):
        command = self.convertToHex(command)
        total = "0"
        for item in command:
            total = hex(int(total, 16) + int(item, 16))
        checksum = [total[-2], total[-1]]
        return checksum

    # Used for all other commands that are not setTemp
    # Currently you need to format dd yourself which is the input value you want to send
    def createCommand(self, command, dd):
        stx = ["*"]
        aa = ["0", "0"]
        cc = self.commandDict[command]
        check = aa + cc + dd
        ss = self.checksum(aa + cc + dd)
        etx = ["\r"]
        command = stx + aa + cc + dd + ss + etx
        return command

    def sendCommand(self, command):
        try:
            for bit in command:
                self.ser.write(bit.encode())
            message, passed = self.recieveMessage()
            return message, passed
        except:
            return None, False

    # Will recieve message but will only check if the command gave an error, will not decode the message
    def recieveMessage(self):
        connection = True
        buff = self.buffer.copy()
        for i in range(len(buff)):
            buff[i] = self.ser.read(1).decode("utf-8")
        if buff == self.checksumError:
            connection = False
            return buff, connection
        else:
            time.sleep(0.1)
            return buff, connection

    def convertSetTempListToValue(self, temp: list[str]) -> float:
        """
        Use to convert the message returned when reading the
        temperature from the Peltier thermistor. The message will be a list
        and this function will convert this value back to a human readable temperature.
        """
        temp = temp[1:9]
        temp = "".join(temp)
        temp = int(temp, 16) / 100

        # This is to deal with negative temperatures, the controller will
        # return the twos compliment if it encounters a negative value which
        # in turn produces an unnaturally large value
        if temp > 1000:
            temp = -1 * self.twosCompliment(temp)
        return temp

    def convertSetTempValueToList(self, temp: float) -> list:
        """
        Use to convert a float to a list that can be interpreted by the
        Peltier controller. This function is used to set the temp of the Peltier.
        """
        value = ["0", "0", "0", "0", "0", "0", "0", "0"]
        temp *= 100
        temp = int(temp)
        if temp < 0:
            temp = self.twosCompliment(temp)
        temp = self.convertToHex(temp)
        temp = self.stringToList(temp)
        cutoff = temp.index("x")
        temp = temp[cutoff + 1 :]
        for i, _ in enumerate(temp):
            value[-(i + 1)] = temp[-(i + 1)]
        return value
