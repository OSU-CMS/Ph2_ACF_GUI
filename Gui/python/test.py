import Peltier
print(hex(ord('0')))
def convertToHex(val):
        if type(val) != list:
            return hex(val) 
        for i, item in enumerate(val):
            val[i] = hex(ord(item))
        return val

def checksum(command):
        command = convertToHex(command)
        total = '0'
        for item in command:
            total = hex(int(total, 16) + int(item, 16))
        
        return [total[-2], total[-1]]

a = checksum(['0','0','2','d','0','0','0','0','0','0','0','1'])
print(a)
