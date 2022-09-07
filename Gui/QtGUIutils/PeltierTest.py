# TE Technology, INC.
# Traverse City, Michigan
# https://tetech.com/
#
# Download TC-36-25-RS232 Manual for complete command set at the website below 
# https://tetech.com/product/tc-36-25-rs232/
# 
# Download the correct FTDI VCP drivers for your system at the website below
# http://www.ftdichip.com/Drivers/VCP.htm
# 
# This basic code is for setting the temperature on the TC-36-25-RS232 temperature controller 
# 
# Only works on Python 2.7 as of this time -- 3:37 PM June 11th, 2018
# 
# You will need to install the pyserial package to communicate through the port
# The line below is the pip install module command
# pip install pyserial

# You MUST switch OUTPUT from OFF to ON using the TC-36-25-RS232 Software
# Otherwise the cooler will not receive power


import serial
# send set temperature of -1.50 C
# see TC-36-25-RS232 Manual for command codes -- edit the line below to change the set temperature
bstc=['*','0','0','1','c','f','f','f','f','f','f','6','a','e','f','\r'] # characters in this line must be in lowercase
# stx relates to '*'
# the controller address is '0','0'
# the control command is '1','c'
# data being transmitted is 'f','f','f','f','f','f','6','a'
# checksum is 'e','f'
# return is '\r'

# read control sensor temp1
bst=['*','0','0','0','1','0','0','0','0','0','0','0','0','4','1','\r'] # characters in this line must be in lowercase
# stx relates to '*'
# the controller address is '0','0'
# the control command is '0','1'
# data being transmitted is '0','0','0','0','0','0','0','0'
# checksum is '4','1'
# return is '\r'
buf=[0,0,0,0,0,0,0,0,0,0,0,0]

def hexc2dec(bufp):
        newval=0
        divvy=pow(16,7)
#sets the word size to DDDDDDDD
        for pn in range (1,7):
                vally=ord(bufp[pn])
                if(vally < 97):
                        subby=48
                else:
                        subby=87
                    # ord() converts the character to the ascii number value
                newval+=((ord(bufp[pn])-subby)*divvy)
                divvy/=16
                if(newval > pow(16,8)/2-1):
                        newval=newval-pow(16,8)
                   #distinguishes between positive and negative numbers
        return newval

# change to the port that is connected to the TC-36-25-RS232 Temperature Controller
# this is a sample application used to demonstrate the TC-36-25-RS232 serial protocol
ser=serial.Serial('/dev/ttyUSB0', 9600, timeout=1)
for pn in range(0,16):
        ser.write((bstc[pn].encode()))
        # customer's have noticed an improved communication from the controller with a 4 millisecond delay
        # this delay is optional, however feel free to attempt it in case of any communication problems
for pn in range(0,12):
        buf[pn]=ser.read(1)
        print(buf[pn])
for pn in range(0,16):
        ser.write((bst[pn].encode()))
for pn in range(0,12):
        buf[pn]=ser.read(1)
        print(buf[pn])
ser.close()
print buf 
temp1=hexc2dec(buf)
print temp1/100.0 #this prints the current read temperature
wait=input("PORT CLOSED")

