HWSettings = {
"nEvents"        :   100, 
"nEvtsBurst"     :   100,

"VDDDTrimTarget" :  1.30,
"VDDATrimTarget" :  1.20,
"VDDDTrimTolerance" : 0.02,
"VDDATrimTolerance" : 0.02,

"nTRIGxEvent"    :    10,
"INJtype"        :     1,
"ResetMask"      :     0,
"ResetTDAC"      :     0,

"ROWstart"       :     0,
"ROWstop"        :   191,
"COLstart"       :   128,
"COLstop"        :   263,

"LatencyStart"   :     0,
"LatencyStop"    :   511,

"VCalHstart"     :   100,
"VCalHstop"      :   600,
"VCalHnsteps"    :    50,
"VCalMED"        :   100,

"TargetCharge"   : 20000,
"KrumCurrStart"  :     0,
"KrumCurrStop"   :   127,

"ThrStart"       :   340,
"ThrStop"        :   440,
"TargetThreshold":  2000,
"TargetOcc"      :  1e-6,
"UnstuckPixels"  :     0,

"TAP0Start"      :     0,
"TAP0Stop"       :  1023,
"TAP1Start"      :     0,
"TAP1Stop"       :   511,
"InvTAP1"        :     1,
"TAP2Start"      :     0,
"TAP2Stop"       :   511,
"InvTAP2"        :     0,

"chain2Test"     :     0,
"byTime"         :     1,
"framesORtime"   :    10,

"TargetBER"      :  1e-5,

"RegNameDAC1"    : "VCAL_HIGH",
"StartValueDAC1" :   250,
"StopValueDAC1"  :   600,
"StepDAC1"       :    50,
"RegNameDAC2"    : "user.ctrl_regs.fast_cmd_reg_5.delay_after_inject_pulse",
"StartValueDAC2" :    28,
"StopValueDAC2"  :    50,


"DoFast"         :     0,
"DisplayHisto"   :     0,
"UpdateChipCfg"  :     1,

"SaveBinaryData" :     0,
"nHITxCol"       :     1,
"InjLatency"     :    32,
"nClkDelays"     :   280,
}
