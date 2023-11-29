import pyvisa as visa
import importlib
import subprocess
import logging
import os 


from Gui.GUIutils.settings import *
from Configuration.XMLUtil import *
from Gui.python.TCP_Interface import *
from Gui.siteSettings import *
from keysightE3633A import KeysightE3633A
from keithley2410 import  Keithley2410
from Gui.python.logging_config import logger


if GPIB_DebugMode:
    visa.log_to_screen()

class PowerSupply():
    def __init__(self,model = "Keithley", boardnumber = 0, primaryaddress = 24, powertype = "HV",serverIndex = 0):
        self.Model = model
        self.Status = "OFF"
        self.deviceMap = {}
        self.Instrument = None
        self.PowerType = powertype
        self.PoweringMode = None
        self.ModuleType = None
        self.CompCurrent = 0.0
        self.XMLConfig = None
        self.Port = None
        self.DeviceNode = None
        self.Answer = None
        self.maxTries = 10
        self.ServerIndex = serverIndex
        self.setResourceManager()

    
    def setPowerType(self,powertype):
        if powertype != "HV" and powertype != "LV":
            logger.error("Power Type: {} not supported".format(powertype))
        else:
            self.PowerType = powertype

    def isHV(self):
        return self.PowerType == "HV"

    def isLV(self):
        return self.PowerType == "LV"

    def setPowerModel(self, model):
        self.Model = model

    def setPoweringMode(self, powermode="Direct"):
        self.PoweringMode = powermode
        
    def setCompCurrent(self, compcurrent = 1.05):
        self.CompCurrent = compcurrent

    def setModuleType(self, moduletype = None):
        self.ModuleType = moduletype
        
    def setResourceManager(self):
        os.environ["PYVISA_LIBRARY"] = '@py'
        self.ResourcesManager = visa.ResourceManager('@py')

    def listResources(self):
        try:
            self.ResourcesList = self.ResourcesManager.list_resources()
            self.getDeviceName()
            return list(self.deviceMap.keys())
        except Exception as err:
            logger.error("Failed to list all resources: {}".format(err))
            self.ResourcesList = ()
            return self.ResourcesList

    def setInstrument(self,resourceName):
        try:
            print(resourceName)
            self.setResourceManager()
            print(self.deviceMap.keys())

            if "USBLV" in resourceName:
                self.Instrument = KeysightE3633A(resourceName, reset_on_init=False, off_on_close=True)
            elif "USBHV" in resourceName:
                self.Instrument = Keithley2410(resourceName, reset_on_init=False, ramp_down_on_close=True)

            if resourceName in self.deviceMap.keys():
                self.Port = self.deviceMap[resourceName].lstrip("ASRL").rstrip("::INSTR")
                
            self.Instrument.__enter__()

        except Exception as err:
                logger.error("Failed to open resource {0}: {1}".format(resourceName,err))


    def hwUpdate(self, pHead, pAnswer):
        if pAnswer is not None:
            self.Answer = pAnswer
            logger.info("TCP: PowerSupply {} - {}:{}".format(self.PowerType, pHead,pAnswer))
        else:
            self.Answer = None

    def getDeviceName(self):
        self.deviceMap = {}
        for device in self.ResourcesList:
            try:
                pipe = subprocess.Popen(['udevadm', 'info', ' --query', 'all', '--name', device.lstrip("ASRL").rstrip("::INSTR"), '--attribute-walk'], stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
                raw_output = pipe.communicate()[0]
                vendor_list = [info for info in  raw_output.splitlines() if b'ATTRS{idVendor}' in info or b'ATTRS{vendor}' in info]
                product_list = [info for info in  raw_output.splitlines() if b'ATTRS{idProduct}' in info or b'ATTRS{product}' in info]
                idvendor = vendor_list[0].decode("UTF-8").split("==")[1].lstrip('"').rstrip('"').replace('0x','')
                idproduct = product_list[0].decode("UTF-8").split("==")[1].lstrip('"').rstrip('"').replace('0x','')
                deviceId = "{}:{}".format(idvendor,idproduct)
                pipeUSB = subprocess.Popen(['lsusb','-d',deviceId], stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
                usbInfo = pipeUSB.communicate()[0]
                deviceName = usbInfo.decode("UTF-8").split(deviceId)[-1].lstrip(' ').rstrip('\n')
                if deviceName == None:
                    logger.warning("No device name found for {}:".format(device))
                    self.deviceMap[device] = device
                else:
                    self.deviceMap[device] = deviceName
            except Exception as err:
                logger.error("Error found:{}".format(err))
                self.deviceMap[device] = device

    def getInfo(self):
        try:
            info = self.Instrument.query("IDENTIFIER")
            return info
        except Exception as err:
            logging.error("Failed to get instrument information:{}".format(err))
            return "No valid device"

    def startRemoteCtl(self):
        pass   

    def InitialDevice(self):
        if self.PowerType == "LV":
            pass
        if self.PowerType == "HV":
            self.Instrument.reset()
            self.Instrument.set('SOURCE', 'VOLT')
            self.Instrument.set('VOLTAGE_MODE', 'FIX')
            self.Instrument.set('SENSE_CURRENT_RANGE', 10e-6)

    def TurnOn(self):
        try:
            self.InitialDevice()
            Voltage = 0.0
            Current = 0.0

            if self.PowerType ==  "LV" and self.PoweringMode == "SLDO":
                Voltage = ModuleVoltageMapSLDO[self.ModuleType]
                Current = ModuleCurrentMap[self.ModuleType]
            elif self.PowerType ==  "LV" and self.PoweringMode == "Direct":
                Voltage = ModuleVoltageMap[self.ModuleType]
                Current = ModuleCurrentMap[self.ModuleType]

            if self.PowerType == "LV":
                self.Instrument.set('VOLTAGE1', Voltage)
                self.Instrument.set('CURRENT1', Current)
                self.Instrument.on(1)

            if self.PowerType == "HV":
                try:
                    HVstatus = self.Instrument.status() 
                    print(HVstatus)
                    if '1' in str(HVstatus):
                        print('found HV status {0}'.format(HVstatus))#debug
                        self.Instrument.off()
                    self.Instrument.set("VOLTAGE", 0)
                    self.Instrument.on() 
                except Exception as err:
                    logging.error("Failed to turn on the sourceMeter:{}".format(err))

        except Exception as err:
            logging.error("Failed to turn on the sourceMeter:{}".format(err))

        # TurnOnLV is only used for SLDO Scan
    def TurnOnLV(self):
        try:
            self.Instrument.on(1)
        except Exception as err:
            logging.error("Failed to turn on the sourceMeter:{}".format(err))

# This will also disconnect the power supplies, so if you turn off the power supply but want to turn on the PS with the same object, it will break
    def TurnOff(self):
        try:
            if self.PowerType == "LV":
                self.Instrument.off(1)
            elif self.PowerType == "HV":
                self.Instrument.off()

        except Exception as err:
            logging.error("Failed to turn off the sourceMeter:{}".format(err))

    def ReadOutputStatus(self):
        try:
            HVoutputstatus = self.Instrument.status()
            return str(HVoutputstatus)
        except Exception as err:
            return None
        
    def ReadVoltage(self):
        try:
            if self.PowerType == 'LV':
                voltage = self.Instrument.query('VOLTAGE1')
            elif self.PowerType == 'HV':
                voltage = self.Instrument.query('VOLTAGE')
            return voltage
        except Exception as err:
            return None

    def SetVoltage(self, voltage = 0.0):
        if self.PowerType == "HV":
            return

        try:
            self.Instrument.set('VOLTAGE1', voltage)
        except Exception as err:
            logging.err("Failed to set {} voltage to {}".format(self.PowerType, voltage))

    def SetRange(self, voltRange):
        if not self.isLV():
            logging.info("Try to setVoltage for non-LV power supply")
            return

        try:
            self.Instrument.set('OVP1',voltRange)
        except Exception as err:
            logging.error("Failed to set range for the sourceMeter:{}".format(err))
            return None

    def SetCurrent(self, current = 0.0):
        if self.PowerType == "HV":
            return
        try:
            self.Instrument.set('CURRENT1',current)
        except Exception as err:
            logging.error("Failed to set {} current to {}".format(self.PowerType, current))

    def ReadCurrent(self):
        try:
            if self.PowerType == "LV":
                current = self.Instrument.query('CURRENT1')
                return current
            elif self.PowerType == "HV": 
                #self.Instrument.reset()
                self.Instrument.set("SOURCE", 'VOLT')
                self.Instrument.set("VOLTAGE_MODE", 'FIX')
                #self.Instrument.set("SENSE_FUNCTION", 'CURR')
                self.Instrument.set("OUTPUT", 'ON')

                # This returns a comma seperated string of 5 numbers, looking at the keithley, the second entry is the actual current
                current = float(self.Instrument.query('READ').split(",")[1])

                return current
        except Exception as err:
            print(err)
            pass
        
    def TurnOnHV(self):
        print("TurnOnHV() starts")#debug
        if not self.isHV():
            logging.info("Try to turn on non-HV as high voltage")
            return

        try:
            HVstatus = self.Instrument.status() 
            print(HVstatus)
            if '1' in str(HVstatus):
                print('found HV status {0}'.format(HVstatus))
                self.Instrument.off()
            self.Instrument.set("VOLTAGE", 0)
            self.Instrument.on() 
        except Exception as err:
            logging.error("Failed to turn on the sourceMeter:{}".format(err))
            return None

    def TurnOffHV(self):
            print("inside TurnOFFHV") #debug
            if not self.isHV():
                    logging.info("Try to turn off non-HV as high voltage")
                    return
            try:
                HVstatus = self.ReadOutputStatus()
                if '0' in HVstatus:
                    return
                currentVoltage = float(self.ReadVoltage()) #does issue is here?
                print("current voltage in TurnoffHV() is: "+ str(currentVoltage))#debug
                stepLength = 3
                if 0 < currentVoltage:
                    stepLength = -3

                for voltage in range(int(currentVoltage),0,stepLength):
                    self.SetHVVoltage(voltage)
                    time.sleep(0.3)
                self.SetHVVoltage(0)
                self.TurnOff()
            except Exception as err:
                logging.error("Failed to turn off the sourceMeter:{}".format(err))
                return None           

    def SetHVRange(self, voltRange):
        if not self.isHV():
            logging.info("Try to setVoltage for non-HV power supply")
            return
        try:
            self.Instrument.set('VOLTAGE_RANGE', voltRange)
        except Exception as err:
            logging.error("Failed to set range for the sourceMeter:{}".format(err))
            return None

    def SetHVVoltage(self, voltage):
        if not self.isHV():
            logging.info("Try to setVoltage for non-HV power supply")
            return

        try:
            print("Using python interface")
            print("set Voltage :"+str(voltage))
            self.Instrument.set("VOLTAGE", voltage)
        except Exception as err:
            logging.error("Failed to set HV target the sourceMeter:{}".format(err))
            return None

    def SetHVComplianceLimit(self, compliance):
        if not self.isHV():
            logging.info("Try to setVoltage for non-HV power supply")
            return

        try:
            self.Instrument.set('COMPLIANCE_CURRENT',compliance)
        except Exception as err:
            logging.error("Failed to set compliance limit for the sourceMeter:{}".format(err))
            return None

    def RampingUp(self, hvTarget = 0.0, stepLength = 0.0):
        if self.isHV():
            try:
                print("RampingUp")#debug
                HVstatus = self.ReadOutputStatus()
                if '1' in HVstatus:
                    self.TurnOffHV()
                    print("doing turnOffHV in rampingUP") #debug
                #self.Instrument.set('SENSE_CURRENT_RANGE', 10e-6)
                #self.Instrument.set('VOLTAGE_MODE', 'FIX')
                self.SetHVComplianceLimit(defaultHVCurrentCompliance)
                self.SetHVVoltage(0)
                self.TurnOn()
                
                currentVoltage = float(self.ReadVoltage())
                if hvTarget < currentVoltage:
                    stepLength = -abs(stepLength)
                
                for voltage in range(int(currentVoltage), int(hvTarget), int(stepLength)):
                    self.SetHVVoltage(voltage)
                    print("doing SetHVVoltage")#debug
                    time.sleep(0.3)
                self.SetHVVoltage(hvTarget)
                    
            except Exception as err:
                            logging.error("Failed to ramp the voltage to {0}:{1}".format(hvTarget,err))
        else:
                        logging.info("Not a HV power supply, abort")

    def customized(self,cmd):
        if "Keith" in self.Model:
            cmd = "K2410:" + cmd
        

    def Status(self):
        if not "KeySight" in self.Model:
            return -1

        try:
            reply = self.Instrument.status()
            return 1
        except Exception as err:
            logging.error("Failed to get the status code:{}".format(err))
            return None

    def Reset(self):
        if not "KeySight" in self.Model:
            return -1
        try:
            reply = self.Instrument.reset() 
            return 1
        except Exception as err:
            logging.error("Failed to get the status code:{}".format(err))
            return None

if __name__ == "__main__":
    # This allows icicle to use the right pyvisa library without changing the code. 
    LVpowersupply = PowerSupply(powertype = "LV")
    LVpowersupply.setPoweringMode()

    #HVpowersupply.setPowerModel("ahhh")
    LVpowersupply.setInstrument("ASRL/dev/ttyUSBLV::INSTR")
    LVpowersupply.setCompCurrent()
    LVpowersupply.TurnOn()
    time.sleep(10)
    LVpowersupply.TurnOff()
