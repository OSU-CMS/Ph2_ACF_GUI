from Configuration.XMLUtil import *
from Configuration.Settings.GlobalSettings import *
from Configuration.Settings.FESettings import *
from Configuration.Settings.HWSettings import *
from Configuration.Settings.RegisterSettings import *

# Config Front-end Chip
FE0_0_0 = FE()
FE0_0_0.ConfigureFE(FESettings)
 
# Config Front-end Module
FEModule0_0 = FEModule()
FEModule0_0.AddFE(FE0_0_0)
FEModule0_0.ConfigureGlobal(globalSettings)

# Config BeBoardModule
BeBoardModule0 = BeBoardModule()
BeBoardModule0.AddFEModule(FEModule0_0)
BeBoardModule0.SetRegisterValue(RegisterSettings)

# Config HWDescription
HWDescription = HWDescription()
HWDescription.AddBeBoard(BeBoardModule0)
HWDescription.AddSettings(HWSettings)
