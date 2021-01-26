globalSettings = {
'EN_CORE_COL_SYNC'       :     "0",
'EN_CORE_COL_LIN_1'      : "65535",
'EN_CORE_COL_LIN_2'      :     "1",
'EN_CORE_COL_DIFF_1'     :     "0",
'EN_CORE_COL_DIFF_2'     :     "0",
 
'EN_MACRO_COL_CAL_LIN_1' : "65535",
'EN_MACRO_COL_CAL_LIN_2' : "65535",
'EN_MACRO_COL_CAL_LIN_3' : "65535",
'EN_MACRO_COL_CAL_LIN_4' : "65535",
'EN_MACRO_COL_CAL_LIN_5' :    "15",

'HITOR_0_MASK_SYNC'      : "65535",
'HITOR_1_MASK_SYNC'      : "65535",
'HITOR_2_MASK_SYNC'      : "65535",
'HITOR_3_MASK_SYNC'      : "65535",

'HITOR_0_MASK_DIFF_0'    : "65535",
'HITOR_0_MASK_DIFF_1'    :     "1",
'HITOR_1_MASK_DIFF_0'    : "65535",
'HITOR_1_MASK_DIFF_1'    :     "1",
'HITOR_2_MASK_DIFF_0'    : "65535",
'HITOR_2_MASK_DIFF_1'    :     "1",
'HITOR_3_MASK_DIFF_0'    : "65535",
'HITOR_3_MASK_DIFF_1'    :     "1",

'LOCKLOSS_CNT'           :     "0",
'BITFLIP_WNG_CNT'        :     "0",
'BITFLIP_ERR_CNT'        :     "0",
'CMDERR_CNT'             :     "0",
'SKIPPED_TRIGGER_CNT'    :     "1",
}

globalSettings_Dict = {
    'Latency'                    :    globalSettings,
    'PixelAlive'                 :    globalSettings,
    'NoiseScan'                  :   globalSettings,
    'GainScan'                   :    globalSettings,
    'SCurveScan'                 :    globalSettings,
    'ThresholdEqualization'      :    globalSettings,
    'GainOptimization'           :    globalSettings,
    'ThresholdMinimization'      :    globalSettings,
    'ThresholdAdjustment'        :    globalSettings,
    'InjectionDelay'             :    globalSettings,
    'ClockDelay'                 :    globalSettings,
    'Physics'                    :    globalSettings,
}
