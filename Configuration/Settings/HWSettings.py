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

HWSettings_Latency = copy.deepcopy(HWSettings)
HWSettings_Latency["LatencyStart"] = 110
HWSettings_Latency["LatencyStop"] = 150
HWSettings_Latency["DoFast"] = 1

HWSettings_PixelAlive = copy.deepcopy(HWSettings)

HWSettings_NoiseScan = copy.deepcopy(HWSettings)
HWSettings_NoiseScan["nEvents"] = 1e7
HWSettings_NoiseScan["nEvtsBurst"] = 1e4
HWSettings_NoiseScan["INJtype"] = 0
HWSettings_NoiseScan["nClkDelays"] = 10
HWSettings_NoiseScan["nTRIGxEvent"] = 1

HWSettings_SCurve =  copy.deepcopy(HWSettings)

HWSettings_ThresEqu = copy.deepcopy(HWSettings)
HWSettings_ThresEqu["nEvents"] = 100
HWSettings_ThresEqu["nEvtsBurst"] = 100

HWSettings_GainScan = copy.deepcopy(HWSettings)
HWSettings_GainScan["VCalHstop"] = 4000
HWSettings_GainScan["VCalHnsteps"] = 20

HWSettings_GainOpt = copy.deepcopy(HWSettings)
HWSettings_GainOpt["VCalHstop"] = 4000
HWSettings_GainOpt["VCalHnsteps"] = 20

HWSettings_ThresMin = copy.deepcopy(HWSettings)
HWSettings_ThresMin["nEvents"] = 1e7
HWSettings_ThresMin["nEvtsBurst"] = 1e4
HWSettings_ThresMin["INJtype"] = 1
HWSettings_ThresMin["nClkDelays"] = 10

HWSettings_ThresAdj = copy.deepcopy(HWSettings)

HWSettings_InjDelay = copy.deepcopy(HWSettings)
HWSettings_InjDelay["nTRIGxEvent"] = 1
HWSettings_InjDelay["DoFast"] = 1

HWSettings_ClockDelay = copy.deepcopy(HWSettings)
HWSettings_ClockDelay["nTRIGxEvent"] = 1
HWSettings_ClockDelay["DoFast"] = 1

HWSettings_Physics = copy.deepcopy(HWSettings)




HWSettings_Dict = {
    'Latency'                    :    HWSettings_Latency,
    'PixelAlive'                 :    HWSettings_PixelAlive,
    'NoiseScan'                  :    HWSettings_NoiseScan,
    'GainScan'                   :    HWSettings_GainScan,
    'SCurveScan'                 :    HWSettings_SCurve,
    'ThresholdEqualization'      :    HWSettings_ThresEqu,
    'GainOptimization'           :    HWSettings_GainOpt,
    'ThresholdMinimization'      :    HWSettings_ThresMin,
    'ThresholdAdjustment'        :    HWSettings_ThresAdj,
    'InjectionDelay'             :    HWSettings_InjDelay,
    'ClockDelay'                 :    HWSettings_ClockDelay,
    'Physics'                    :    HWSettings_Physics,
}
