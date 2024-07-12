from PyQt5 import QtCore
from Gui.GUIutils.DBConnection import GetTrimClass
from PyQt5.QtCore import pyqtSignal
from PyQt5.QtWidgets import (
    QCheckBox,
    QComboBox,
    QGridLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QMessageBox,
    QPushButton,
    QHBoxLayout,
    QWidget,
)

import sys

import Gui.siteSettings as site_settings
from Gui.python.Firmware import (
    QtChip,
    QtModule,
    QtOpticalGroup,
)
from Gui.GUIutils.settings import (
    ModuleLaneMap,
    ModuleType,
)
#from Gui.GUIutils.FirmwareUtil import *
#from Gui.QtGUIutils.QtFwCheckDetails import *

from Gui.python.logging_config import logger

class ClickOnlyComboBox(QComboBox):
    def __init__(self, parent=None):
        super().__init__(parent)

    def wheelEvent(self, event):
        event.ignore() 


class ModuleBox(QWidget):
    typechanged = pyqtSignal()

    def __init__(self, firmware):
        super(ModuleBox, self).__init__()
        self.mainLayout = QGridLayout()
        self.firmware = firmware
        self.createRow()
        self.setLayout(self.mainLayout)
        self.VDDDmap = {}
        self.VDDAmap = {}

    def createRow(self):
        SerialLabel = QLabel("SerialNumber:")
        self.SerialEdit = QLineEdit()

        FMCLabel = QLabel("FMC:")
        self.FMCEdit = QLineEdit()
        self.FMCEdit.setText('L12')

        PortLabel = QLabel("FMC port:")
        self.PortEdit = QLineEdit()

        TypeLabel = QLabel("Type:")
        self.TypeCombo = ClickOnlyComboBox()
        self.TypeCombo.addItems(ModuleType.values())
        TypeLabel.setBuddy(self.TypeCombo)

        FC7Label = QLabel("FC7:")
        self.FC7Combo = ClickOnlyComboBox()
        self.FC7Combo.addItems([board.getBoardName() for board in self.firmware])
        if self.FC7Combo.count() == 1:
            self.FC7Combo.setDisabled(True)

        VersionLabel = QLabel("Version:")
        self.ModuleVersionCombo = ClickOnlyComboBox()
        self.ModuleVersionCombo.addItems(["v1", "v2"])
        VersionLabel.setBuddy(self.ModuleVersionCombo)

        self.mainLayout.addWidget(SerialLabel, 0, 0, 1, 1)
        self.mainLayout.addWidget(self.SerialEdit, 0, 1, 1, 1)
        self.mainLayout.addWidget(FMCLabel, 0, 2, 1, 1)
        self.mainLayout.addWidget(self.FMCEdit, 0, 3, 1, 1)
        self.mainLayout.addWidget(FC7Label, 0, 4, 1, 1)
        self.mainLayout.addWidget(self.FC7Combo, 0, 5, 1, 1)
        self.mainLayout.addWidget(PortLabel, 0, 6, 1, 1)
        self.mainLayout.addWidget(self.PortEdit, 0, 7, 1, 1)
        self.mainLayout.addWidget(TypeLabel, 0, 8, 1, 1)
        self.mainLayout.addWidget(self.TypeCombo, 0, 9, 1, 1)
        # self.mainLayout.addWidget(VersionLabel, 0, 10, 1, 1)
        self.mainLayout.addWidget(self.ModuleVersionCombo, 0, 11, 1, 1)
        

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
    
    def getFC7(self):
        return self.FC7Combo.currentText()

    def getFMCPort(self):
        return self.PortEdit.text()

    def getType(self):
        return self.TypeCombo.currentText()
    
    def getVersion(self):
        return self.ModuleVersionCombo.currentText()

    def getVDDD(self, pChipID):
        return self.VDDD[pChipID]


class ChipBox(QWidget):
    chipchanged = pyqtSignal(int, int)
    #adding default value to serialNumber="RH0009" can prevent ChipBox from crashing under online mode
    def __init__(self, master, pChipType, serialNumber="RH0009"):
        super().__init__()
        logger.debug("Inside ChipBox")
        self.master = master
        self.serialNumber = serialNumber
        self.chipType = pChipType
        logger.debug('the chip type passed to the chipbox is {0}'.format(self.chipType))
        self.mainLayout = QHBoxLayout()
        self.ChipList = [] #chip id list for a single module
        #self.initList()
        self.createList()
        self.VDDAMap = {}
        self.VDDDMap = {}
        self.ChipGroupBoxDict = {}
            
        for chipid in self.ChipList:
            self.ChipGroupBoxDict[chipid] = self.makeChipBox(chipid)
            
        self.makeChipGroupBox(self.ChipGroupBoxDict)
    
        self.setLayout(self.mainLayout)

    def initList(self):
        self.module = ModuleBox(self.master.firmware)

    # Makes a list of chips for a given module
    def createList(self):
        for lane in ModuleLaneMap[self.chipType]:
            self.ChipList.append(ModuleLaneMap[self.chipType][lane])

    #get trim values from DB
    # def makeChipBoxWithDB(self, pChipID,VDDA,VDDD):
    #     self.ChipID = pChipID
    #     self.ChipLabel = QCheckBox("Chip ID: {0}".format(self.ChipID))
    #     self.ChipLabel.setChecked(True)
    #     self.ChipLabel.setObjectName("ChipStatus_{0}".format(pChipID))
    #     self.ChipVDDDLabel = QLabel("VDDD:")
    #     self.ChipVDDDEdit = QLineEdit()
    #     self.ChipVDDDEdit.setObjectName("VDDDEdit_{0}".format(pChipID))
        
    #     if not self.ChipVDDDEdit.text():
    #         logger.debug("no VDDD text")
    #     self.ChipVDDALabel = QLabel("VDDA:")
    #     self.ChipVDDAEdit = QLineEdit() 
    #     self.ChipVDDDEdit.setText(VDDD)
    #     self.ChipVDDAEdit.setText(VDDA)
    #     self.ChipVDDAEdit.setObjectName("VDDAEdit_{0}".format(pChipID))

    #     self.VChipLayout = QGridLayout()
    #     self.VChipLayout.addWidget(self.ChipLabel, 0, 0, 1, 2)
    #     self.VChipLayout.addWidget(self.ChipVDDDLabel, 1, 0, 1, 1)
    #     self.VChipLayout.addWidget(self.ChipVDDDEdit, 1, 1, 1, 1)
    #     self.VChipLayout.addWidget(self.ChipVDDALabel, 2, 0, 1, 1)
    #     self.VChipLayout.addWidget(self.ChipVDDAEdit, 2, 1, 1, 1)

    #     return self.VChipLayout

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


from PyQt5.QtWidgets import QWidget, QGridLayout, QGroupBox, QPushButton, QVBoxLayout, QScrollArea

class BeBoardBox(QWidget):
    changed = pyqtSignal()

    def __init__(self, master, firmware):
        super(BeBoardBox, self).__init__()
        self.master = master
        self.firmware = firmware
        self.ModuleList = []
        self.ChipWidgetDict = {}
        self.mainLayout = QVBoxLayout()  # Use QVBoxLayout for vertical layout

        self.initList()
        self.createList()

        scrollArea = QScrollArea()  # Create a scroll area
        scrollContent = QWidget()
        scrollContent.setLayout(self.mainLayout)  # Set mainLayout to scrollable content
        scrollArea.setWidget(scrollContent)
        scrollArea.setWidgetResizable(True)

        mainLayout = QVBoxLayout()
        mainLayout.addWidget(scrollArea)

        self.setLayout(mainLayout)  # Set mainLayout as the layout for BeBoardBox

        self.setGeometry(100, 100, 800, 600)  # Set initial geometry (x, y, width, height)
        self.setMinimumSize(900, 300)  # Set minimum size (width, height)

    def initList(self):
        ModuleRow = ModuleBox(self.firmware)
        ModuleRow.TypeCombo.currentTextChanged.connect(self.updateList)
        self.ModuleList.append(ModuleRow)

    def createList(self):
        self.ListLayout = QGridLayout()
        self.ListLayout.setVerticalSpacing(0)
        self.updateList()

        self.ListBox = QGroupBox()
        self.ListBox.setLayout(self.ListLayout)
        self.mainLayout.addWidget(self.ListBox)

    def deleteList(self):
        self.ListBox.deleteLater()
        self.mainLayout.removeWidget(self.ListBox)

    def updateList(self):
        serialNumberWidgets = []
        chipIDWidgets = []

        for module in self.ModuleList:
            chipBox = ChipBox(self.master, module.getType(), module.getSerialNumber())
            self.ChipWidgetDict[module] = chipBox
            module.setMaximumHeight(50)

            serialNumberWidgets.append(module)
            chipIDWidgets.append(chipBox)

        # Clear existing layout
        for i in reversed(range(self.ListLayout.count())):
            widget = self.ListLayout.itemAt(i).widget()
            if widget:
                self.ListLayout.removeWidget(widget)
                widget.setParent(None)

        # Add serial number widgets
        for index, widget in enumerate(serialNumberWidgets):
            self.ListLayout.addWidget(widget, index, 0, 1, 1)

        # Add chip ID widgets
        for index, widget in enumerate(chipIDWidgets):
            self.ListLayout.addWidget(widget, index + len(serialNumberWidgets), 0, 1, 1)

        # Add remove and add buttons
        for index, module in enumerate(self.ModuleList):
            removeButton = QPushButton("Remove")
            removeButton.setMaximumWidth(150)
            removeButton.clicked.connect(lambda checked, m=module: self.removeModule(m))
            self.ListLayout.addWidget(removeButton, index, 1, 1, 1)

        newButton = QPushButton("Add")
        newButton.setMaximumWidth(150)
        newButton.clicked.connect(self.addModule)
        self.ListLayout.addWidget(newButton, len(self.ModuleList), 1, 1, 1)
        self.update()

    def removeModule(self, module):
        self.ModuleList.remove(module)
        module.deleteLater()
        self.updateList()
        self.changed.emit()

    def addModule(self):
        module = ModuleBox(self.firmware)
        module.TypeCombo.currentTextChanged.connect(self.updateList)
        self.ModuleList.append(module)
        self.updateList()
        self.changed.emit()

    def getModules(self):
        return self.ModuleList

    def getFirmwareDescription(self):
        for module in self.ModuleList:
            #Access the currently selected QtBeBoard object
            BeBoard = None
            for board in self.firmware:
                if board.getBoardName() == module.getFC7():
                    BeBoard = board
            if BeBoard is None:
                raise Exception("There are no FC7s active.")
            
            #Access the currently selected QtOpticalGroup of the QtBeBoard
            OpticalGroup = None
            for og in BeBoard.getAllOpticalGroups().values():
                if og.getFMCID() == module.getFMCID():
                    OpticalGroup = og
            
            #Create it if it doesn't already exist
            if OpticalGroup is None:
                OpticalGroup = QtOpticalGroup(FMCID=module.getFMCID())
                OpticalGroup.setBeBoard(BeBoard) #ignore this line, see explanation in Firmware.py
                BeBoard.addOpticalGroup(
                    FMCID=module.getFMCID(),
                    OpticalGroup=OpticalGroup,
                )
            
            #Create a QtModule object based on the input data
            Module = QtModule(
                moduleName=module.getSerialNumber(),
                moduleType=module.getType(),
                moduleVersion=module.getVersion(),
                FMCPort=module.getFMCPort()
            )
            Module.setOpticalGroup(OpticalGroup) #ignore this line, see explanation in Firmware.py
            
            #Pull VDDA/VDDD trim and chip status from the ChipBox on the StartWindow.
            for chipID in ModuleLaneMap[module.getType()].values():
                Module.getChips()[chipID].setStatus(self.ChipWidgetDict[module].getChipStatus(chipID))
                Module.getChips()[chipID].setVDDA(self.ChipWidgetDict[module].getVDDA(chipID))
                Module.getChips()[chipID].setVDDD(self.ChipWidgetDict[module].getVDDD(chipID))
            
            #Add the QtModule object to the currently selected Optical Group
            OpticalGroup.addModule(FMCPort=module.getFMCPort(), module=Module)

        ret = []
        for board in self.firmware:
            if len(board.getAllOpticalGroups()) != 0: #if modules are connected to the board
                board.setBoardID(len(ret))
                ret.append(board)
        return ret


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
        self.FMCEdit = site_settings.defaultFMC
        # self.FMCEdit.setMinimumWidth(120)
        # self.FMCEdit.setMaximumWidth(200)

        CableIDLabel = QLabel("Cable ID:")
        self.CableIDEdit = QLineEdit()
        # self.IDEdit.setMinimumWidth(120)
        # self.IDEdit.setMaximumWidth(200)
        self.CableIDEdit.textChanged.connect(self.on_TypeChanged)
        self.CableIDEdit.setReadOnly(True)

        TypeLabel = QLabel("Type:")
        #self.Type = site_settings.defaultModuleType

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
        return site_settings.defaultFMC

    def setID(self, laneId):
        self.CableIDEdit.setText(str(laneId))

    def getID(self):
        return self.CableIDEdit.text()

    def setType(self, typeStr):
        self.Type = typeStr

    def getType(self, SerialNumber):
        if "ZH" in SerialNumber:
            self.Type = "TFPX Quad"
        elif "RH" in SerialNumber:
            self.Type = "TFPX CROC 1x2"
        elif "SH" in SerialNumber:
            self.Type = "TFPX CROC Quad"
        return self.Type

    @QtCore.pyqtSlot()
    def on_TypeChanged(self):
        self.typechanged.emit()

    @QtCore.pyqtSlot()
    def on_textChange(self):
        self.SerialString = self.SerialEdit.text()
        ## Add Parser
        self.textchanged.emit()
    
    @QtCore.pyqtSlot()
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
        logger.debug(f'{__name__} : Creating module list')
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
        logger.debug(f'{__name__} : Updating module list')
        [columns, rows] = [self.ListLayout.columnCount(), self.ListLayout.rowCount()]

        for i in range(columns):
            for j in range(rows):
                item = self.ListLayout.itemAtPosition(j, i)
                if item:
                    widget = item.widget()
                    self.ListLayout.removeWidget(widget)
        logger.debug(f'{__name__} : Before connecting module signals')
        for index, module in enumerate(self.ModuleList):
            # module.setMaximumWidth(500)
            module.setMaximumHeight(50)
            module.typechanged.connect(self.on_TypeChanged)
            module.textchanged.connect(self.on_ModuleFilled)
            module.setID(index)
            self.ListLayout.addWidget(module, index, 0, 1, 1)
        logger.debug(f'{__name__} : After connecting module signals')
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
        logger.debug(f'{__name__} : Finished setting up module list')

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
        self.firmware.removeAllModules()

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
        #return self.FilledModuleList

    def getFirmwareDescription(self, **kwargs):
        for index, module in enumerate(self.FilledModuleList):
            if module.getSerialNumber() == "":
                continue

            fwType = module.getType(module.getSerialNumber())
            
            FwModule = QtModule(type=fwType)
            FwModule.setModuleID(module.getID())
            FwModule.setFMCID(module.getFMCID())
            FwModule.setModuleName(module.getSerialNumber())
            
            ##Checked here and the correct fwType is being set
            if fwType in ModuleType.values():
                FwModule.__moduleType = fwType
            else:
                FwModule.__moduleType = "TFPX SCC"

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
