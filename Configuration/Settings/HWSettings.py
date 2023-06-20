import copy

HWSettings = {
"nEvents"           :   100,
"nEvtsBurst"        :   100,

"VDDDTrimTarget"    :  1.30,
"VDDATrimTarget"    :  1.20,
"VDDDTrimTolerance" :  0.01,
"VDDATrimTolerance" :  0.01,

"nTRIGxEvent"       :    10,
"INJtype"           :     1,
"ResetMask"         :     0,
"ResetTDAC"         :     0,

"ROWstart"          :     0,
"ROWstop"           :   191,
"COLstart"          :   128,
"COLstop"           :   263,

"LatencyStart"      :     0,
"LatencyStop"       :   511,

"VCalHstart"        :   100,
"VCalHstop"         :   600,
"VCalHnsteps"       :    50,
"VCalMED"           :   100,

"TargetCharge"      : 20000,
"KrumCurrStart"     :     0,
"KrumCurrStop"      :   127,

"ThrStart"          :   340,
"ThrStop"           :   440,
"TargetThr"         :  2000,
"TargetOcc"         :  1e-6,
"UnstuckPixels"     :     0,

"TAP0Start"         :     0,
"TAP0Stop"          :  1023,
"TAP1Start"         :     0,
"TAP1Stop"          :   511,
"TAP2Start"         :     0,
"TAP2Stop"          :   511,
"InvTAP2"           :     0,

"chain2Test"        :     0,
"byTime"            :     1,
"framesORtime"      :    10,

"RegNameDAC1"       :   'VCAL_HIGH',
"StartValueDAC1"    :   250,
"StopValueDAC1"     :   600,
"StepDAC1"          :    50,
"RegNameDAC2"       :    'user.ctrl_regs.fast_cmd_reg_5.delay_after_inject_pulse',
"StartValueDAC2"    :    28,
"StopValueDAC2"     :    50,
"StepDAC2"          :     1,

"DoFast"            :     0,
"DisplayHisto"      :     0,
"UpdateChipCfg"     :     1,

"SaveBinaryData"    :     0,
"nHITxCol"          :     1,
"InjLatency"        :    32,
"nClkDelays"        :   280,
}

HWSettingsA_v49 = copy.deepcopy(HWSettings)
HWSettingsA_v49["ResetTDAC"] = -1
HWSettingsA_v49["TargetCharge"] = 10000
HWSettingsA_v49["TDACGainStart"] = 130
HWSettingsA_v49["TDACGainStop"] = 130
HWSettingsA_v49["TDACGainNSteps"] = 0
HWSettingsA_v49["DoNSteps"] = 0
HWSettingsA_v49["OccPerPixel"] = 2e-5
HWSettingsA_v49["MaxMaskedPixels"] = 1
HWSettingsA_v49["DoOnlyNGroups"] = 0
HWSettingsA_v49["DisableChannelsAtExit"] = 1
HWSettingsA_v49.pop("DoFast")
HWSettingsA_v49["DataOutputDir"] = ""

#Adding CROC HW_settings
HWSettingsB_v49 = copy.deepcopy(HWSettingsA_v49)
HWSettingsB_v49["DoDataIntegrity"] = 0
HWSettingsB_v49["ROWstop"] = 335
HWSettingsB_v49["COLstart"] = 0
HWSettingsB_v49["COLstop"] = 431
HWSettingsB_v49["VCalHstop"] = 1100
HWSettingsB_v49["KrumCurrStop"] = 210
HWSettingsB_v49["TDACGainStart"] = 140
HWSettingsB_v49["TDACGainStop"] = 140
HWSettingsB_v49["VDDDTrimTarget"] = 1.20
HWSettingsB_v49["RegNameDAC1"] = "user.ctrl_regs.fast_cmd_reg_5.delay_after_inject_pulse"
HWSettingsB_v49["StartValueDAC1"] = 28
HWSettingsB_v49["StopValueDAC1"] = 50
HWSettingsB_v49["StepDAC1"] = 1
HWSettingsB_v49["RegNameDAC2"] = "VCAL_HIGH"
HWSettingsB_v49["StartValueDAC2"] = 300
HWSettingsB_v49["StopValueDAC2"] = 1000
HWSettingsB_v49["StepDAC2"] = 20
HWSettingsB_v49["nClkDelays"] = 1300

#End section
HWSettingsA_Latency = copy.deepcopy(HWSettingsA_v49)
HWSettingsA_Latency["LatencyStart"] = 110
HWSettingsA_Latency["LatencyStop"] = 150

HWSettingsA_PixelAlive = copy.deepcopy(HWSettingsA_v49)

HWSettingsA_NoiseScan = copy.deepcopy(HWSettingsA_v49)
HWSettingsA_NoiseScan["nEvents"] = 1e7
HWSettingsA_NoiseScan["nEvtsBurst"] = 1e4
HWSettingsA_NoiseScan["INJtype"] = 0
HWSettingsA_NoiseScan["nClkDelays"] = 10
HWSettingsA_NoiseScan["nTRIGxEvent"] = 1

HWSettingsA_SCurve =  copy.deepcopy(HWSettingsA_v49)

HWSettingsA_ThresEqu = copy.deepcopy(HWSettingsA_v49)
HWSettingsA_ThresEqu["nEvents"] = 100
HWSettingsA_ThresEqu["nEvtsBurst"] = 100

HWSettingsA_GainScan = copy.deepcopy(HWSettingsA_v49)
HWSettingsA_GainScan["VCalHstop"] = 4000
HWSettingsA_GainScan["VCalHnsteps"] = 20

HWSettingsA_GainOpt = copy.deepcopy(HWSettingsA_v49)
HWSettingsA_GainOpt["VCalHstop"] = 4000
HWSettingsA_GainOpt["VCalHnsteps"] = 20

HWSettingsA_ThresMin = copy.deepcopy(HWSettingsA_v49)
HWSettingsA_ThresMin["nEvents"] = 1e7
HWSettingsA_ThresMin["nEvtsBurst"] = 1e4
HWSettingsA_ThresMin["INJtype"] = 0
HWSettingsA_ThresMin["nClkDelays"] = 10
HWSettingsA_ThresMin["ThrStart"] = 370
HWSettingsA_ThresMin["nTRIGxEvent"] = 1

HWSettingsA_ThresAdj = copy.deepcopy(HWSettingsA_v49)

HWSettingsA_InjDelay = copy.deepcopy(HWSettingsA_v49)
HWSettingsA_InjDelay["LatencyStart"] = 110
HWSettingsA_InjDelay["nTRIGxEvent"] = 1
HWSettingsA_InjDelay["DoOnlyNGroups"] = 1

HWSettingsA_ClockDelay = copy.deepcopy(HWSettingsA_v49)
HWSettingsA_ClockDelay["nTRIGxEvent"] = 1
HWSettingsA_ClockDelay["DoOnlyNGroups"] = 1

HWSettingsA_BitErrRate = copy.deepcopy(HWSettingsA_v49)

HWSettingsA_DataRBOpt = copy.deepcopy(HWSettingsA_v49)

HWSettingsA_VoltageTuning = copy.deepcopy(HWSettingsA_v49)

HWSettingsA_GenDACDAC = copy.deepcopy(HWSettingsA_v49)

HWSettingsA_Physics = copy.deepcopy(HWSettingsA_v49)

#For CROC
HWSettingsB_Latency = copy.deepcopy(HWSettingsB_v49)
HWSettingsB_Latency["LatencyStart"] = 110
HWSettingsB_Latency["LatencyStop"] = 150

HWSettingsB_PixelAlive = copy.deepcopy(HWSettingsB_v49)
HWSettingsB_PixelAlive["DoDataIntegrity"] = 1

HWSettingsB_NoiseScan = copy.deepcopy(HWSettingsB_v49)
HWSettingsB_NoiseScan["nEvents"] = 1e7
HWSettingsB_NoiseScan["nEvtsBurst"] = 1e4
HWSettingsB_NoiseScan["INJtype"] = 0
HWSettingsB_NoiseScan["nClkDelays"] = 70
HWSettingsB_NoiseScan["nTRIGxEvent"] = 1

HWSettingsB_SCurve =  copy.deepcopy(HWSettingsB_v49)

HWSettingsB_ThresEqu = copy.deepcopy(HWSettingsB_v49)
HWSettingsB_ThresEqu["nEvents"] = 100
HWSettingsB_ThresEqu["nEvtsBurst"] = 100

HWSettingsB_GainScan = copy.deepcopy(HWSettingsB_v49)
HWSettingsB_GainScan["VCalHstop"] = 4000
HWSettingsB_GainScan["VCalHnsteps"] = 20

HWSettingsB_GainOpt = copy.deepcopy(HWSettingsB_v49)
HWSettingsB_GainOpt["VCalHstop"] = 4000
HWSettingsB_GainOpt["VCalHnsteps"] = 20

HWSettingsB_ThresMin = copy.deepcopy(HWSettingsB_v49)
HWSettingsB_ThresMin["nEvents"] = 1e7
HWSettingsB_ThresMin["nEvtsBurst"] = 1e4
HWSettingsB_ThresMin["INJtype"] = 0
HWSettingsB_ThresMin["nClkDelays"] = 70
HWSettingsB_ThresMin["ThrStart"] = 370
HWSettingsB_ThresMin["nTRIGxEvent"] = 1

HWSettingsB_ThresAdj = copy.deepcopy(HWSettingsB_v49)
HWSettingsB_ThresAdj["ThrStart"] = 380
HWSettingsB_ThresAdj["ThrStop"] = 500

HWSettingsB_InjDelay = copy.deepcopy(HWSettingsB_v49)
HWSettingsB_InjDelay["LatencyStart"] = 110
HWSettingsB_InjDelay["nTRIGxEvent"] = 1
HWSettingsB_InjDelay["DoOnlyNGroups"] = 1

HWSettingsB_ClockDelay = copy.deepcopy(HWSettingsB_v49)
HWSettingsB_ClockDelay["nTRIGxEvent"] = 1
HWSettingsB_ClockDelay["DoOnlyNGroups"] = 1

HWSettingsB_BitErrRate = copy.deepcopy(HWSettingsB_v49)

HWSettingsB_DataRBOpt = copy.deepcopy(HWSettingsB_v49)

HWSettingsB_VoltageTuning = copy.deepcopy(HWSettingsB_v49)

HWSettingsB_GenDACDAC = copy.deepcopy(HWSettingsB_v49)

HWSettingsB_Physics = copy.deepcopy(HWSettingsB_v49)

#End


HWSettings_DictA = {
    'Latency'                    :    HWSettingsA_Latency,
    'PixelAlive'                 :    HWSettingsA_PixelAlive,
    'NoiseScan'                  :    HWSettingsA_NoiseScan,
    'GainScan'                   :    HWSettingsA_GainScan,
    'SCurveScan'                 :    HWSettingsA_SCurve,
    'ThresholdEqualization'      :    HWSettingsA_ThresEqu,
    'GainOptimization'           :    HWSettingsA_GainOpt,
    'ThresholdMinimization'      :    HWSettingsA_ThresMin,
    'ThresholdAdjustment'        :    HWSettingsA_ThresAdj,
    'InjectionDelay'             :    HWSettingsA_InjDelay,
    'ClockDelay'                 :    HWSettingsA_ClockDelay,
    'BitErrorRate'               :    HWSettingsA_BitErrRate,
    'DataRBOptimization'         :    HWSettingsA_DataRBOpt,
    'ChipIntVoltageTuning'       :    HWSettingsA_VoltageTuning,
    'GenericDAC-DAC'             :    HWSettingsA_GenDACDAC,
    'Physics'                    :    HWSettingsA_Physics,
    'IVCurve'                    :    HWSettingsA_v49,
}

HWSettings_DictB = {
    'Latency'                    :    HWSettingsB_Latency,
    'PixelAlive'                 :    HWSettingsB_PixelAlive,
    'NoiseScan'                  :    HWSettingsB_NoiseScan,
    'GainScan'                   :    HWSettingsB_GainScan,
    'SCurveScan'                 :    HWSettingsB_SCurve,
    'ThresholdEqualization'      :    HWSettingsB_ThresEqu,
    'GainOptimization'           :    HWSettingsB_GainOpt,
    'ThresholdMinimization'      :    HWSettingsB_ThresMin,
    'ThresholdAdjustment'        :    HWSettingsB_ThresAdj,
    'InjectionDelay'             :    HWSettingsB_InjDelay,
    'ClockDelay'                 :    HWSettingsB_ClockDelay,
    'BitErrorRate'               :    HWSettingsB_BitErrRate,
    'DataRBOptimization'         :    HWSettingsB_DataRBOpt,
    'ChipIntVoltageTuning'       :    HWSettingsB_VoltageTuning,
    'GenericDAC-DAC'             :    HWSettingsB_GenDACDAC,
    'Physics'                    :    HWSettingsB_Physics,
    'IVCurve'                    :    HWSettingsB_v49,
}