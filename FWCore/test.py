import FWCore.DAQ as DAQ
#from   Configuration.Config.SingleSCC import *

myDAQ = DAQ.DAQ("myDAQ")

myDAQ.config = DAQ.SetConfiguration("HWDescription","Configuration.Config.SingleSCC")
myDAQ.configFile = DAQ.ConfigureBoard(myDAQ.config, "XMLforPixelAlive.xml")

myDAQ.step1 = DAQ.Step("PixelAlive",
  HWDescription = "XMLforTest.xml"
)

myDAQ.config2 = DAQ.SetConfiguration("HWDescription","Configuration.Config.SingleSCC")
myDAQ.configFile2 = DAQ.ConfigureBoard(myDAQ.config, "XMLforTest2.xml")


myDAQ.step2 = DAQ.Step("Test2",
  HWDescription = "XMLforTest2.xml"
)

myDAQ.config3 = DAQ.SetConfiguration("HWDescription","Configuration.Config.SingleSCC")
myDAQ.configFile3 = DAQ.ConfigureBoard(myDAQ.config, "XMLforTest3.xml")

myDAQ.step3 = DAQ.Step("Test3",
  HWDescription = "XMLforTest3.xml"
)

myDAQ.Procedure = DAQ.Process(myDAQ.step1,myDAQ.step2,myDAQ.step3)

myDAQ.ShowProcedure()


