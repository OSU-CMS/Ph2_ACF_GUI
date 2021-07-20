FESettings = {
'PA_IN_BIAS_LIN'        :  "350",
'FC_BIAS_LIN'           :   "20",
'KRUM_CURR_LIN'         :   "29",
'LDAC_LIN'              :  "130",
'COMP_LIN'              :  "110",
'REF_KRUM_LIN'          :  "300",
'Vthreshold_LIN'        :  "400",

'IBIASP1_SYNC'          :  "100",
'IBIASP2_SYNC'          :  "150",
'IBIAS_SF_SYNC'         :   "80",
'IBIAS_KRUM_SYNC'       :   "55",
'IBIAS_DISC_SYNC'       :  "280",
'ICTRL_SYNCT_SYNC'      :  "100",
'VBL_SYNC'              :  "360",
'VTH_SYNC'              :  "400",
'VREF_KRUM_SYNC'        :  "450",
'CONF_FE_SYNC'          :    "2",

'PRMP_DIFF'             :  "511",
'FOL_DIFF'              :  "542",
'PRECOMP_DIFF'          :  "512",
'COMP_DIFF'             : "1023",
'VFF_DIFF'              :   "40",
'VTH1_DIFF'             :  "700",
'VTH2_DIFF'             :  "100",
'LCC_DIFF'              :   "20",
'CONF_FE_DIFF'          :    "0",

'VCAL_HIGH'             :  "600",
'VCAL_MED'              :  "100",

'GP_LVDS_ROUTE'         :    "0",
'LATENCY_CONFIG'        :  "136",
'CLK_DATA_DELAY'        :    "0",
'INJECTION_SELECT'      :    "9",

'VOLTAGE_TRIM_DIG'      :   "16",
'VOLTAGE_TRIM_ANA'      :   "16",

'CML_CONFIG_SER_EN_TAP' : "0b00",
'CML_CONFIG_SER_INV_TAP': "0b00",
'CML_TAP0_BIAS'         :  "500",
'CML_TAP1_BIAS'         :    "0",
'CML_TAP2_BIAS'         :    "0",

'MONITOR_CONFIG_ADC'    :    "5",
'MONITOR_CONFIG_BG'     :   "12",
'ADC_OFFSET_VOLT'       :   "63",

'ADC_MAXIMUM_VOLT'      :  "839",
'TEMPSENS_IDEAL_FACTOR' : "1225",
}


FESettings_Dict = {
    'Latency'                   :    FESettings,
    'PixelAlive'                :    FESettings,
    'NoiseScan'                 :    FESettings,
    'GainScan'                  :    FESettings,
    'SCurveScan'                :    FESettings,
    'ThresholdEqualization'     :    FESettings,
    'GainOptimization'          :    FESettings,
    'ThresholdMinimization'     :    FESettings,
    'ThresholdAdjustment'       :    FESettings,
    'InjectionDelay'            :    FESettings,
    'ClockDelay'                :    FESettings,
    'Physics'                   :    FESettings,
}
