import logging

# Customize the logging configuration
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
    filename="my_project.log",  # Specify a log file
    filemode="w",  # 'w' for write, 'a' for append
)

logger = logging.getLogger(__name__)

from PyQt5 import QtCore
from Gui.GUIutils.DBConnection import GetTrimClass
from PyQt5.QtCore import *
from PyQt5.QtWidgets import (
    QCheckBox,
    QComboBox,
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QPushButton,
    QHBoxLayout,
    QWidget,
)

import sys

from Gui.python.Firmware import *
from Gui.GUIutils.settings import *
from Gui.GUIutils.FirmwareUtil import *
from Gui.QtGUIutils.QtFwCheckDetails import *

from Gui.python.logging_config import logger


class ModuleBox(QWidget):
    typechanged = pyqtSignal()

    def __init__(self,connection):
        super(ModuleBox, self).__init__()
        self.connection=connection
        self.mainLayout = QGridLayout()
        self.createRow()
        self.setLayout(self.mainLayout)
        self.VDDDmap = {}
        self.VDDAmap = {}

    def createRow(self):
        SerialLabel = QLabel("SerialNumber:")
        self.SerialEdit = QLineEdit()

        FMCLabel = QLabel("FMC:")
        self.FMCEdit = QLineEdit()

        IDLabel = QLabel("FMC port:")
        self.IDEdit = QLineEdit()
        

        TypeLabel = QLabel("Type:")
        self.TypeCombo = QComboBox()
        self.TypeCombo.addItems(ModuleType.values())
        TypeLabel.setBuddy(self.TypeCombo)

        #hide the combo box at above when it is connected
        if self.connection == "Offline":
            
            self.mainLayout.addWidget(TypeLabel, 0, 6, 1, 1)
            self.mainLayout.addWidget(self.TypeCombo, 0, 7, 1, 1)
        
        else:
            self.OnlineButton = QPushButton("confirm")
            self.mainLayout.addWidget(self.OnlineButton, 0, 7, 1, 1)

        self.mainLayout.addWidget(SerialLabel, 0, 0, 1, 1)
        self.mainLayout.addWidget(self.SerialEdit, 0, 1, 1, 1)
        self.mainLayout.addWidget(FMCLabel, 0, 2, 1, 1)
        self.mainLayout.addWidget(self.FMCEdit, 0, 3, 1, 1)
        self.mainLayout.addWidget(IDLabel, 0, 4, 1, 1)
        self.mainLayout.addWidget(self.IDEdit, 0, 5, 1, 1)
        

    def setType(self):
        #this method is created to set moudle type under online mode and comboBox is hidden
        if self.SerialEdit.text().startswith("RH"):
            chipType = "CROC 1x2"
            self.TypeCombo.setCurrentText(chipType)

        if self.SerialEdit.text().startswith("SH"):
            chipType = "TFPX CROC Quad"

            self.TypeCombo.setCurrentText(chipType)

        

        

    def getSerialNumber(self):
        return self.SerialEdit.text()

    def getFMCID(self):
        return self.FMCEdit.text()

    def getID(self):
        return self.IDEdit.text()

    def getType(self):
        return self.TypeCombo.currentText()

    def getVDDD(self, pChipID):
        return self.VDDD[pChipID]


class ChipBox(QWidget):
    chipchanged = pyqtSignal(int, int)
    #adding default value to serialNumber="RH0009" can prevent ChipBox from crashing under online mode
    def __init__(self, master,pChipType,serialNumber="RH0009"):
        super().__init__()
        logger.debug("Inside ChipBox")
        self.master = master
        self.connection = self.master.connection
        self.serialNumber=serialNumber
        self.chipType = pChipType
        self.mainLayout = QHBoxLayout()
        self.ChipList = [] #chip id list for a single module
        #self.initList()
        self.createList()
        self.VDDAMap = {}
        self.VDDDMap = {}
        self.ChipGroupBoxDict = {}
        
        
        #get trim values from DB
        GetTRimFromDB=GetTrimClass()
        GetTRimFromDB.connection=self.connection


        if GetTRimFromDB.connection == "Offline" or GetTRimFromDB.connection == []:
            for chipid in self.ChipList:
                self.ChipGroupBoxDict[chipid] = self.makeChipBox(chipid)


        
        else:
            if self.serialNumber != []:
                
                sorted_VDDAlist,sorted_VDDDlist= GetTRimFromDB.GetTrim(self.serialNumber)     

                #right now there is no data for ZH-module on the database. So the trim values will be set as the default value
                #case for can't find anything on database
                if sorted_VDDAlist == [] or sorted_VDDDlist == []:
                    if self.serialNumber.startswith("ZH"):
                        for chipid in self.ChipList:
                            VDDA = "16"
                            VDDD = "16"
                            self.ChipGroupBoxDict[chipid] = self.makeChipBoxWithDB(chipid,VDDA,VDDD)

                    else:

                        print(f"I can't find the data for module {self.serialNumber}")
                        

                        for chipid in self.ChipList:
                            VDDA = "can't find the value"
                            VDDD = "can't find the value"
                            self.ChipGroupBoxDict[chipid] = self.makeChipBoxWithDB(chipid,VDDA,VDDD)
                
                

                if sorted_VDDAlist != [] or sorted_VDDDlist != []:
                    i = 0
                    for chipid in self.ChipList:
                        VDDA = str(sorted_VDDAlist[i][1])
                        VDDD = str(sorted_VDDDlist[i][1])
                        self.ChipGroupBoxDict[chipid] = self.makeChipBoxWithDB(chipid,VDDA,VDDD)
                        i += 1
            
        
        self.makeChipGroupBox(self.ChipGroupBoxDict)
    
        self.setLayout(self.mainLayout)

    
    def initList(self):
        self.module = ModuleBox(self.connection)

    # Makes a list of chips for a given module
    def createList(self):
        if self.connection == "Offline" or self.connection == []:
            for lane in ModuleLaneMap[self.chipType]:
                self.ChipList.append(ModuleLaneMap[self.chipType][lane])
        
        else:
            
            if self.serialNumber.startswith("RH"):
                self.chipType = "CROC 1x2"
                for lane in ModuleLaneMap[self.chipType]:
                    self.ChipList.append(ModuleLaneMap[self.chipType][lane])

            elif self.serialNumber.startswith("SH"):
                self.chipType = "TFPX CROC Quad"
                for lane in ModuleLaneMap[self.chipType]:
                    self.ChipList.append(ModuleLaneMap[self.chipType][lane])
            
            elif self.serialNumber.startswith("ZH"):
                self.chipType = "TFPX Quad"
                for lane in ModuleLaneMap[self.chipType]:
                    self.ChipList.append(ModuleLaneMap[self.chipType][lane])
            
            else:

                print("current module can't be found on database")



           


    #get trim values from DB
    def makeChipBoxWithDB(self, pChipID,VDDA,VDDD):
        self.ChipID = pChipID
        self.ChipLabel = QCheckBox("Chip ID: {0}".format(self.ChipID))
        self.ChipLabel.setChecked(True)
        self.ChipLabel.setObjectName("ChipStatus_{0}".format(pChipID))
        self.ChipVDDDLabel = QLabel("VDDD:")
        self.ChipVDDDEdit = QLineEdit()
        self.ChipVDDDEdit.setObjectName("VDDDEdit_{0}".format(pChipID))
        
        if not self.ChipVDDDEdit.text():
            print("no VDDD text")
        self.ChipVDDALabel = QLabel("VDDA:")
        self.ChipVDDAEdit = QLineEdit() 
        self.ChipVDDDEdit.setText(VDDD)
        self.ChipVDDAEdit.setText(VDDA)
        self.ChipVDDAEdit.setObjectName("VDDAEdit_{0}".format(pChipID))

        self.VChipLayout = QGridLayout()
        self.VChipLayout.addWidget(self.ChipLabel, 0, 0, 1, 2)
        self.VChipLayout.addWidget(self.ChipVDDDLabel, 1, 0, 1, 1)
        self.VChipLayout.addWidget(self.ChipVDDDEdit, 1, 1, 1, 1)
        self.VChipLayout.addWidget(self.ChipVDDALabel, 2, 0, 1, 1)
        self.VChipLayout.addWidget(self.ChipVDDAEdit, 2, 1, 1, 1)

        return self.VChipLayout




    def makeChipBox(self, pChipID):    
        self.ChipID = pChipID
        self.ChipLabel = QCheckBox("Chip ID: {0}".format(self.ChipID))
        self.ChipLabel.setChecked(True)
        self.ChipLabel.setObjectName("ChipStatus_{0}".format(pChipID))
        self.ChipVDDDLabel = QLabel("VDDD:")
        self.ChipVDDDEdit = QLineEdit()
        self.ChipVDDDEdit.setObjectName("VDDDEdit_{0}".format(pChipID))
        self.ChipVDDALabel = QLabel("VDDA:")
        self.ChipVDDAEdit = QLineEdit()
        
        if "CROC" in self.chipType:
            self.ChipVDDDEdit.setText("8")
            self.ChipVDDAEdit.setText("8")
        else:
            self.ChipVDDDEdit.setText("16")
            self.ChipVDDAEdit.setText("16")
        
        self.ChipVDDAEdit.setObjectName("VDDAEdit_{0}".format(pChipID))
        
        self.VChipLayout = QGridLayout()
        self.VChipLayout.addWidget(self.ChipLabel, 0, 0, 1, 2)
        self.VChipLayout.addWidget(self.ChipVDDDLabel, 1, 0, 1, 1)
        self.VChipLayout.addWidget(self.ChipVDDDEdit, 1, 1, 1, 1)
        self.VChipLayout.addWidget(self.ChipVDDALabel, 2, 0, 1, 1)
        self.VChipLayout.addWidget(self.ChipVDDAEdit, 2, 1, 1, 1)

        return self.VChipLayout

    # def createChipBox(self, pChipID):
    #   self.ChipID = pChipID
    #   self.ChipGroupBox = QGroupBox()
    #   self.ChipLabel = QCheckBox('Chip ID: {0}'.format(self.ChipID))
    #   self.ChipLabel.setChecked(True)
    #   self.ChipVDDDLabel = QLabel('VDDD:')
    #   self.ChipVDDDEdit = QLineEdit()
    #   self.ChipVDDDEdit.setText('8')
    #   self.ChipVDDALabel = QLabel('VDDA:')
    #   self.ChipVDDAEdit = QLineEdit()
    #   self.ChipVDDAEdit.setText('8')
    #   #self.ChipVDDAEdit.textChanged.connect(self.chipchanged)
    #   #self.ChipVDDDEdit.textChanged.connect(self.on_ChipChanged(pChipID,self.ChipVDDDEdit.text()))

    #   self.VChipLayout = QGridLayout()
    #   self.VChipLayout.addWidget(self.ChipLabel,0,0,1,2)
    #   self.VChipLayout.addWidget(self.ChipVDDDLabel,1,0,1,1)
    #   self.VChipLayout.addWidget(self.ChipVDDDEdit,1,1,1,1)
    #   self.VChipLayout.addWidget(self.ChipVDDALabel,2,0,1,1)
    #   self.VChipLayout.addWidget(self.ChipVDDAEdit,2,1,1,1)
    # self.mainLayout.addLayout(self.VChipLayout)

    def makeChipGroupBox(self, pChipGroupBoxDict):
        for key in pChipGroupBoxDict.keys():
            self.mainLayout.addLayout(pChipGroupBoxDict[key])

    def getVDDA(self, pChipID):
        VDDAthing = self.findChild(QLineEdit, "VDDAEdit_{0}".format(pChipID))
        return VDDAthing.text()

    def getVDDD(self, pChipID):
        VDDDthing = self.findChild(QLineEdit, "VDDDEdit_{0}".format(pChipID))
        return VDDDthing.text()

    def getChipStatus(self, pChipID):
        ChipCheckBox = self.findChild(QCheckBox, "ChipStatus_{0}".format(pChipID))
        ChipStatus = ChipCheckBox.isChecked()
        return ChipStatus


class BeBoardBox(QWidget):
    changed = pyqtSignal()

    def __init__(self, master ,firmware):
        super(BeBoardBox, self).__init__()
        self.master = master #pass DB connection variable to chipBox
        self.firmware = firmware
        self.ModuleList = []
        self.ChipWidgetDict = {}  # maps module to the chipbox object
        self.mainLayout = QGridLayout()

        self.initList()
        self.createList()

        self.setLayout(self.mainLayout)

    def initList(self):
        ModuleRow = ModuleBox(self.master.connection)
        if self.master.connection == "Offline":
            ModuleRow.TypeCombo.currentTextChanged.connect(self.updateList)
        else:
            ModuleRow.OnlineButton.clicked.connect(self.updateList)
            ModuleRow.OnlineButton.clicked.connect(ModuleRow.setType)      
        self.ModuleList.append(ModuleRow)
    

    def createList(self):
        self.ListBox = QGroupBox()

        self.ListLayout = QGridLayout()
        self.ListLayout.setVerticalSpacing(0)

        self.updateList()

        self.ListBox.setLayout(self.ListLayout)
        self.mainLayout.addWidget(self.ListBox, 0, 0)

    def deleteList(self):
        self.ListBox.deleteLater()
        self.mainLayout.removeWidget(self.ListBox)

    def updateList(self):
        [columns, rows] = [self.ListLayout.columnCount(), self.ListLayout.rowCount()]

        for i in range(columns):
            for j in range(rows):
                item = self.ListLayout.itemAtPosition(j, i)
                if item:
                    widget = item.widget()
                    self.ListLayout.removeWidget(widget)
                    widget.setParent(
                        None
                    )  # Weird graphical glitches can occur without this

        for index, module in enumerate(self.ModuleList):
            # module.setMaximumWidth(500)
            self.ChipWidgetDict[module] = ChipBox(self.master,module.getType(),module.getSerialNumber())
            module.setMaximumHeight(50)
            self.ListLayout.addWidget(module, index, 0, 1, 1)
            self.ListLayout.addWidget(self.ChipWidgetDict[module], index + 3, 0, 1, 1)
            if index > 0:
                RemoveButton = QPushButton("remove")
                RemoveButton.setMaximumWidth(150)
                RemoveButton.clicked.connect(lambda x=index: self.removeModule(x))
                self.ListLayout.addWidget(RemoveButton, index, 1, 1, 1)
        # ModuleLayout = QFormLayout()
        # ModuleItem = ModuleBox()

        # ModuleItem.destroy.connect(partial(self.removeModule,ModuleItem))
        # ModuleLayout.addRow(ModuleBox())

        NewButton = QPushButton("add")
        NewButton.setMaximumWidth(150)
        NewButton.clicked.connect(self.addModule)
        self.ListLayout.addWidget(NewButton, len(self.ModuleList), 1, 1, 1)
        self.update()

    def removeModule(self, index):
        self.ModuleList.pop(index)
        if str(sys.version).startswith("3.8"):
            self.deleteList()
            self.createList()
        elif str(sys.version).startswith(("3.7", "3.9")):
            self.updateList()
        else:
            self.updateList()
        self.changed.emit()

    def addModule(self):
        module = ModuleBox()
        module.TypeCombo.currentTextChanged.connect(self.updateList)
        self.ModuleList.append(module)
        if str(sys.version).startswith("3.8"):
            self.deleteList()
            self.createList()
        elif str(sys.version).startswith(("3.7", "3.9")):
            self.updateList()
        else:
            self.updateList()
        self.changed.emit()

    def getModules(self):
        return self.ModuleList

    def getFirmwareDescription(self, **kwargs):
        for index, module in enumerate(self.ModuleList):
            FwModule = QtModule()
            FwModule.setModuleID(module.getID())
            FwModule.setFMCID(module.getFMCID())
            FwModule.setModuleName(module.getSerialNumber())
            for chip in ModuleLaneMap[module.getType()].values():
                print("type debug : " + str(module.getType()) )
                print("Sereial number debug : " + str(module.getSerialNumber()))
                print("chip number debug : " + str(chip))#find an extra issue. chip number is 0, it should be 12 for first chip of Croc 1*2 module. This is fine under offline mode
                print("chip vdda debug :" + str(self.ChipWidgetDict[module].getVDDA(chip))) #it cause bug at here, it is none
                FwModule.setChipStatus(
                    chip, self.ChipWidgetDict[module].getChipStatus(chip)
                )
                FwModule.setChipVDDA(chip, self.ChipWidgetDict[module].getVDDA(chip))
                FwModule.setChipVDDD(chip, self.ChipWidgetDict[module].getVDDD(chip))

            # FwModule.setOpticalGroupID(module.getID())
            FwModule.setModuleType(module.getType())
            self.firmware.addModule(index, FwModule)
        return self.firmware

    # def getVDDA(self, module):
    #   VDDAdict = {}
    #   for key in self.ChipWidgetDict.keys():
    #           VDDAdict[key] = self.ChipWidgetDict[key].getVDDA()
    #   return VDDAdict


class StatusBox(QWidget):
    def __init__(self, verbose, index=0):
        super(StatusBox, self).__init__()
        self.index = index
        self.verbose = verbose
        self.mainLayout = QGridLayout()
        self.createBody()
        self.checkFwPar()
        self.setLayout(self.mainLayout)

    def createBody(self):
        FEIDLabel = QLabel("ID: {}".format(self.index))
        FEIDLabel.setStyleSheet("font-weight:bold")

        self.LabelList = []
        self.EditList = []

        for i, (key, value) in enumerate(self.verbose.items()):
            Label = QLabel()
            Label.setText(str(key) + ":")
            Edit = QLineEdit()
            Edit.setText(str(value))
            Edit.setDisabled(True)
            self.LabelList.append(Label)
            self.EditList.append(Edit)

        self.CheckLabel = QLabel()

        self.mainLayout.addWidget(FEIDLabel, 0, 0, 1, 1)

        for index in range(len(self.LabelList)):
            self.mainLayout.addWidget(self.LabelList[index], index + 1, 0, 1, 1)
            self.mainLayout.addWidget(self.EditList[index], index + 1, 1, 1, 1)

    def checkFwPar(self):
        return True
        """
                self.CheckLabel.setStyleSheet("color:red")
                PowerMode = str(self.PowerModeCombo.currentText())
                if not str(self.ANLVoltEdit.text()) or not str(self.DIGVoltEdit.text()) or not str(self.ANLAmpEdit.text()) or not str(self.DIGAmpEdit.text()):
                        self.CheckLabel.setText("V/I measure is missing")
                        return False

                comment = ''
                try:
                        if float(str(self.ANLVoltEdit.text())) < FEPowerUpVA[PowerMode][0] or float(str(self.ANLVoltEdit.text())) > FEPowerUpVA[PowerMode][1]:
                                comment +=  "Analog Voltage range: {}".format(FEPowerUpVA[PowerMode])
                        if float(str(self.DIGVoltEdit.text())) < FEPowerUpVD[PowerMode][0] or float(str(self.DIGVoltEdit.text())) > FEPowerUpVD[PowerMode][1]:
                                comment +=  "Digital Voltage range: {}".format(FEPowerUpVA[PowerMode])
                        #if math.fabs( float(str(self.AmpEdit.text())) - float(str(self.ANLAmpEdit.text())) - float(str(self.DIGAmpEdit.text())) ) > 0.1:
                        #       comment += "Current from Source deviated from measured current in module"
                        ChipAmp = float(str(self.ANLAmpEdit.text())) + float(str(self.DIGAmpEdit.text()))
                        if ChipAmp  < FEPowerUpAmp[PowerMode][0] or ChipAmp > FEPowerUpAmp[PowerMode][1]:
                                comment +=  "Amp range: {}".format(FEPowerUpAmp[PowerMode])
                except ValueError:
                        comment = "Not valid input"

                if comment == '':
                        comment = "Ok"
                        self.CheckLabel.setText(comment)
                        self.CheckLabel.setStyleSheet("color:green")
                        return True
                else:
                        self.CheckLabel.setText(comment)
                        return False
                """


class SimpleModuleBox(QWidget):
    typechanged = pyqtSignal()
    textchanged = pyqtSignal()
    destroy = pyqtSignal()

    def __init__(self):
        super(SimpleModuleBox, self).__init__()
        self.SerialString = None
        self.mainLayout = QGridLayout()
        self.createRow()
        self.setLayout(self.mainLayout)

    def createRow(self):
        SerialLabel = QLabel("SerialNumber:")
        # prefix = defaultModuleType if type(defaultModuleType) ==  str else ""
        # SerialLabel = QLabel("{} SerialNumber:".format(prefix))
        self.SerialEdit = QLineEdit()
        #self.SerialEdit.returnPressed.connect(self.on_textChange) #original code
        self.SerialEdit.returnPressed.connect(self.on_editing_finished)
        # self.SerialEdit.setMinimumWidth(120)
        # self.SerialEdit.setMaximumWidth(200)

        FMCLabel = QLabel("FMC:")
        # self.FMCEdit = QLineEdit()
        self.FMCEdit = defaultFMC
        # self.FMCEdit.setMinimumWidth(120)
        # self.FMCEdit.setMaximumWidth(200)

        CableIDLabel = QLabel("Cable ID:")
        self.CableIDEdit = QLineEdit()
        # self.IDEdit.setMinimumWidth(120)
        # self.IDEdit.setMaximumWidth(200)
        self.CableIDEdit.textChanged.connect(self.on_TypeChanged)
        self.CableIDEdit.setReadOnly(True)

        TypeLabel = QLabel("Type:")
        self.Type = defaultModuleType

        self.mainLayout.addWidget(SerialLabel, 0, 0, 1, 1)
        self.mainLayout.addWidget(self.SerialEdit, 0, 1, 1, 1)
        self.mainLayout.addWidget(CableIDLabel, 1, 0, 1, 1)
        self.mainLayout.addWidget(self.CableIDEdit, 1, 1, 1, 1)

    def setSerialNumber(self, serial):
        self.SerialEdit.setText(serial)

    def getSerialNumber(self):
        if not self.SerialEdit.text(): #case for nothing is inside serial box
            return None
        else:
            return self.SerialEdit.text()

    def getFMCID(self):
        return defaultFMC

    def setID(self, laneId):
        self.CableIDEdit.setText(str(laneId))

    def getID(self):
        return self.CableIDEdit.text()

    def setType(self, typeStr):
        self.Type = typeStr

    def getType(self, SerialNumber):
        if "ZH" in SerialNumber:
            self.Type = "TFPX Quad"
        elif "SCC" in SerialNumber:
            self.Type = "SingleSCC"
        return self.Type

    @QtCore.pyqtSlot()
    def on_TypeChanged(self):
        self.typechanged.emit()

    @QtCore.pyqtSlot()
    def on_textChange(self):
        self.SerialString = self.SerialEdit.text()
        ## Add Parser
        self.textchanged.emit()
    
    @pyqtSlot()
    def on_editing_finished(self):
        self.SerialString = self.SerialEdit.text()
        self.textchanged.emit()
    



class SimpleBeBoardBox(QWidget):
    changed = pyqtSignal()

    def __init__(self, firmware):
        super(SimpleBeBoardBox, self).__init__()
        self.firmware = firmware
        self.ModuleList = []
        self.FilledModuleList = []
        self.BufferBox = None
        self.mainLayout = QGridLayout()
        self.currentModule = -1

        self.initList()
        self.createList()

        self.setLayout(self.mainLayout)

    def initList(self):
        ModuleRow = SimpleModuleBox()
        self.ModuleList.append(ModuleRow)
        self.ModuleList[-1].SerialEdit.setFocus()

    def createList(self):
        self.ListBox = QGroupBox()

        self.ListLayout = QGridLayout()
        self.ListLayout.setVerticalSpacing(0)

        self.updateList()

        self.ListBox.setLayout(self.ListLayout)
        self.mainLayout.addWidget(self.ListBox, 0, 0)

    def deleteList(self):
        self.ListBox.deleteLater()
        self.mainLayout.removeWidget(self.ListBox)

    def updateList(self):
        [columns, rows] = [self.ListLayout.columnCount(), self.ListLayout.rowCount()]

        for i in range(columns):
            for j in range(rows):
                item = self.ListLayout.itemAtPosition(j, i)
                if item:
                    widget = item.widget()
                    self.ListLayout.removeWidget(widget)

        for index, module in enumerate(self.ModuleList):
            # module.setMaximumWidth(500)
            module.setMaximumHeight(50)
            module.typechanged.connect(self.on_TypeChanged)
            module.textchanged.connect(self.on_ModuleFilled)
            module.setID(index)
            self.ListLayout.addWidget(module, index, 0, 1, 1)
            # Add "remove" botton
            # if index > 0:
            #   RemoveButton = QPushButton("remove")
            #   RemoveButton.setMaximumWidth(150)
            #   RemoveButton.clicked.connect(lambda x = index: self.removeModule(x))
            #   self.ListLayout.addWidget(RemoveButton,index,1,1,1)
        # ModuleLayout = QFormLayout()
        # ModuleItem = ModuleBox()

        # ModuleItem.destroy.connect(partial(self.removeModule,ModuleItem))
        # ModuleLayout.addRow(ModuleBox())

        NewButton = QPushButton("add")
        NewButton.setMaximumWidth(150)
        NewButton.clicked.connect(self.addModule)

        # For QR code, remove all modules
        ClearButton = QPushButton("Clear")
        ClearButton.setMaximumWidth(150)
        ClearButton.clicked.connect(self.clearModule)

        # self.ListLayout.addWidget(NewButton,len(self.ModuleList),1,1,1)
        self.ListLayout.addWidget(ClearButton, len(self.ModuleList), 0, 1, 1)
        self.update()

    def removeModule(self, index):
        # For Manual change
        self.ModuleList.pop(index)

        # For QR SCAN
        # self.ModuleList = []
        # self.initList()

        if str(sys.version).startswith("3.8"):
            self.deleteList()
            self.createList()
        elif str(sys.version).startswith(("3.7", "3.9")):
            self.updateList()
        else:
            self.updateList()
        self.changed.emit()

    def clearModule(self):
        self.ModuleList = [SimpleModuleBox()]
        self.FilledModuleList = []
        self.firmware.removeAllModule()

        if str(sys.version).startswith("3.8"):
            self.deleteList()
            self.createList()
        elif str(sys.version).startswith(("3.7", "3.9")):
            self.updateList()
        else:
            self.updateList()

        self.ModuleList[-1].SerialEdit.setFocus()

    def addModule(self):
        self.ModuleList.append(SimpleModuleBox())
        # For QR Scan
        self.BufferBox = SimpleModuleBox()

        if str(sys.version).startswith("3.8"):
            self.deleteList()
            self.createList()
        elif str(sys.version).startswith(("3.7", "3.9")):
            self.updateList()
        else:
            self.updateList()
        self.changed.emit()

        self.ModuleList[-1].SerialEdit.setFocus()

    def getModules(self):
        return self.FilledModuleList

    def getFirmwareDescription(self, **kwargs):
        for index, module in enumerate(self.FilledModuleList):
            if module.getSerialNumber() == "":
                continue
            FwModule = QtModule()
            FwModule.setModuleID(module.getID())
            FwModule.setFMCID(module.getFMCID())
            FwModule.setModuleName(module.getSerialNumber())
            fwType = module.getType(module.getSerialNumber())
            if fwType in ModuleType.values():
                FwModule.__moduleType = fwType
            else:
                FwModule.__moduleType = "SingleSCC"

            for i in ModuleLaneMap[FwModule.__moduleType].keys():
                FEChip = QtChip()
                LaneID = str(i)
                chipNumber = ModuleLaneMap[FwModule.__moduleType][LaneID]
                FEChip.setID(ModuleLaneMap[FwModule.__moduleType][LaneID])
                FEChip.setLane(LaneID)

                # TODO these values should be set so that they read in from the database
                FEChip.setVDDA(16)
                FEChip.setVDDD(16)
                FEChip.setStatus(1)
                FwModule.addChip(i, FEChip)

            self.firmware.addModule(index, FwModule)
        return self.firmware

    @QtCore.pyqtSlot()
    def on_TypeChanged(self):
        self.changed.emit()

    @QtCore.pyqtSlot()
    def on_ModuleFilled(self):
        self.FilledModuleList.append(self.ModuleList[-1])
        self.addModule()
