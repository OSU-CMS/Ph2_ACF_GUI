def GenerateXMLConfig(BeBoard, testName, outputDir, txt_files: dict, **arg):
    outputFile = f"{outputDir}/CMSIT_{BeBoard.getBoardName()}_{testName}.xml"
    print(outputFile)

    boardtype = "RD53A"
    RegisterSettingsList = RegisterSettings  # TODO: Investigate whether this actually matters (ie deep vs shallow copy)

    # Get Hardware discription and a list of the modules
    HWDescription0 = HWDescription()

    BeBoardModule0 = BeBoardModule()

    # Set up Optical Groups
    for og in BeBoard.getAllOpticalGroups().values():
        OpticalGroupModule0 = OGModule()
        OpticalGroupModule0.SetOpticalGrp(og.getOpticalGroupID(), og.getFMCID())

        # Set up each module within the optical group
        for module in og.getAllModules().values():
            HyBridModule0 = HyBridModule()
            HyBridModule0.SetHyBridModule(module.getFMCPort(), "1")
            HyBridModule0.SetHyBridName(module.getModuleName())

            moduleType = module.getModuleType()
            hdiVersion = module.getHDIVersion()
            registerKey = "{0}_HDIv{1}".format(moduleType.replace(" ", "_"), hdiVersion)
            print("register key is {0}".format(registerKey))
            RegisterSettingsList = RegisterSettings_dict[registerKey]
            print("I see that the hdi version is {0}".format(hdiVersion))

            RxPolarities = (
                "1"
                if "CROC" in moduleType
                and "Quad" in moduleType
                and "TFPX" in moduleType
                else "0"
                if "CROC" in moduleType
                else None
            )
            # revPolarity = bool(int(RxPolarities))

            FESettings_Dict = (
                {
                    test_key: FESettings_DictB.get(registerKey, FESettingsB_dict)
                    for test_key in HWSettings_DictB
                }
                if "CROC" in moduleType
                else FESettings_DictA
            )
            globalSettings_Dict = (
                globalSettings_DictB if "CROC" in moduleType else globalSettings_DictA
            )
            HWSettings_Dict = (
                HWSettings_DictB if "CROC" in moduleType else HWSettings_DictA
            )
            FELaneConfig_Dict = FELaneConfig_DictB[registerKey]

            if FELaneConfig_Dict is None:
                logger.error(
                    f"No FELaneConfig found for module type {module.getModuleType()}."
                )

            #####
            boardtype = (
                "RD53B" + module.getModuleVersion() if "CROC" in moduleType else "RD53A"
            )

            # Sets up all the chips on the module and adds them to the hybrid module to then be stored in the class
            for chip in module.getChips().values():
                logger.info("chip %s status is %s", chip.getID(), chip.getStatus())
                FEChip = FE()
                if (
                    module.getModuleName(),
                    module.getFMCPort(),
                    chip.getID(),
                ) in txt_files.keys():
                    txt_file = txt_files[
                        module.getModuleName(), module.getFMCPort(), chip.getID()
                    ]
                else:
                    txt_file = "CMSIT_RD53_{0}_{1}_{2}.txt".format(
                        module.getModuleName(), module.getFMCPort(), chip.getID()
                    )
                FEChip.SetFE(
                    chip.getID(),
                    "1" if chip.getStatus() else "0",
                    chip.getLane(),
                    RxPolarities,
                    txt_file,
                )

                chip_settings = FESettings_Dict[testName][registerKey].copy()
                chip_settings["VREF_ADC"] = chip.getVREF()
                chip_settings["INJ_CAP"] = (
                    chip.getCINJ()
                )  # Can uncomment once INJ_CAP is implemented into the dictionary for the XML
                FEChip.ConfigureFE(chip_settings)

                if testName in FELaneConfig_Dict:
                    FEChip.ConfigureLaneConfig(
                        FELaneConfig_Dict[testName][int(chip.getLane())]
                    )
                else:
                    logger.warning(
                        f"Test name {testName} not found in FELaneConfig_Dict."
                    )

                if "trimbit_dict" in arg:
                    logger.info(
                        "Setting VDDA to %s", arg["trimbit_dict"][int(chip.getID())][0]
                    )
                    FEChip.VDDAtrim = arg["trimbit_dict"][int(chip.getID())][0]
                    logger.info(
                        "Setting VDDD to %s", arg["trimbit_dict"][int(chip.getID())][1]
                    )
                    FEChip.VDDDtrim = arg["trimbit_dict"][int(chip.getID())][1]
                else:
                    FEChip.VDDAtrim = chip.getVDDA()
                    FEChip.VDDDtrim = chip.getVDDD()
                FEChip.EfuseID = chip.getEfuseID()
                HyBridModule0.AddFE(FEChip)
            HyBridModule0.ConfigureGlobal(globalSettings_Dict[testName])
            OpticalGroupModule0.AddHyBrid(HyBridModule0)

        BeBoardModule0.AddOGModule(OpticalGroupModule0)

        BeBoardModule0.SetURI(BeBoard.getIPAddress())
        BeBoardModule0.SetBeBoard(BeBoard.getBoardID(), "RD53")

        BeBoardModule0.SetRegisterValue(RegisterSettingsList)
        HWDescription0.AddBeBoard(BeBoardModule0)

    HWSettings_Dict[testName]["DataOutputDir"] = BeBoard.getBoardName()
    HWDescription0.AddSettings(HWSettings_Dict[testName])
    MonitoringModule0 = MonitoringModule(boardtype)
    if "RD53A" in boardtype:
        MonitoringModule0.SetMonitoringList(MonitoringListA)
    else:
        if testName in Monitoring_DictB:
            MonitoringModule0.SetSleepTime(Monitor_SleepTime[testName])
            MonitoringModule0.SetMonitoringList(Monitoring_DictB[testName])
        else:
            MonitoringModule0.SetMonitoringList({})
    HWDescription0.AddMonitoring(MonitoringModule0)
    GenerateHWDescriptionXML(HWDescription0, outputFile, boardtype)

    return outputFile
