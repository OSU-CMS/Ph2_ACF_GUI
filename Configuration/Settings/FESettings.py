import os
import copy
Ph2_ACF_VERSION = os.environ.get("Ph2_ACF_VERSION")

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
'VTH_SYNC'              :  "150",
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
#'CML_TAP0_BIAS'         :  "500",
#'CML_TAP1_BIAS'         :    "0",
#'CML_TAP2_BIAS'         :    "0",

'MONITOR_CONFIG_ADC'    :    "5",
'MONITOR_CONFIG_BG'     :   "12",
'ADC_OFFSET_VOLT'       :   "63",

'ADC_MAXIMUM_VOLT'      :  "839",
'TEMPSENS_IDEAL_FACTOR' : "1225",
}

FESettingsB = {
    'DAC_PREAMP_L_LIN'       :   "300",
    'DAC_PREAMP_R_LIN'       :   "300",
    'DAC_PREAMP_TL_LIN'      :   "300",
    'DAC_PREAMP_TR_LIN'      :   "300",
    'DAC_PREAMP_T_LIN'       :   "300",
    'DAC_PREAMP_M_LIN'       :   "300",
    'DAC_FC_LIN'             :    "20",
    'DAC_KRUM_CURR_LIN'      :    "70",
    'DAC_REF_KRUM_LIN'       :   "360",
    'DAC_COMP_LIN'           :   "110",
    'DAC_COMP_TA_LIN'        :   "110",
    'DAC_GDAC_L_LIN'       :   "450",
    'DAC_GDAC_R_LIN'        :   "450",
    'DAC_GDAC_M_LIN'         :   "450",
    'DAC_LDAC_LIN'           :   "140",

    'VCAL_HIGH'              :  "2000",
    'VCAL_MED'               :   "100",

    'GP_LVDS_ROUTE_0'        :  "1495",
    'GP_LVDS_ROUTE_1'        :  "1495",
    'TriggerConfig'          :   "136",
    'CLK_DATA_DELAY'         :     "0",
    'CAL_EDGE_FINE_DELAY'    :     "0",
    'ANALOG_INJ_MODE'        :     "0",

    'VOLTAGE_TRIM_DIG'       :     "10", #FIXME: temp changed from 8 to 10 for RH0010
    'VOLTAGE_TRIM_ANA'       :     "7", #FIXME: temp changed from 8 to 7 for RH0010

    'CML_CONFIG_SER_EN_TAP'  :  "0b00",
    'CML_CONFIG_SER_INV_TAP' :  "0b00",
    'DAC_CML_BIAS_0'         :   "500",
    'DAC_CML_BIAS_1'         :     "0",
    'DAC_CML_BIAS_2'         :     "0",

    'MON_ADC_TRIM'           :     "5",

    'ToT6to4Mapping'         :     "0",
    'ToTDualEdgeCount'       :     "0",

    'ADC_OFFSET_VOLT'        :    "63",
    'ADC_MAXIMUM_VOLT'       :   "839",
    'TEMPSENS_IDEAL_FACTOR'  :  "1225",
    'VREF_ADC'               :   "800",
}

FESettings_DictA = {
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
    'IVCurve'                   :    FESettings,
}

FESettings_DictA = copy.deepcopy(FESettings_DictA)

FESettings_DictB = {
    'Latency'                   :    FESettingsB,
    'PixelAlive'                :    FESettingsB,
    'NoiseScan'                 :    FESettingsB,
    'GainScan'                  :    FESettingsB,
    'SCurveScan'                :    FESettingsB,
    'ThresholdEqualization'     :    FESettingsB,
    'GainOptimization'          :    FESettingsB,
    'ThresholdMinimization'     :    FESettingsB,
    'ThresholdAdjustment'       :    FESettingsB,
    'InjectionDelay'            :    FESettingsB,
    'ClockDelay'                :    FESettingsB,
    'Physics'                   :    FESettingsB,
    'IVCurve'                   :    FESettingsB,
}

FESettings_A_v48 = copy.deepcopy(FESettings)

#FESettings_A_v48.pop("CML_TAP0_BIAS")
#FESettings_A_v48.pop("CML_TAP1_BIAS")
#FESettings_A_v48.pop("CML_TAP2_BIAS")

FESettings_A_v48["DAC_CML_BIAS_0"] = "500"
FESettings_A_v48["DAC_CML_BIAS_1"] = "0"
FESettings_A_v48["DAC_CML_BIAS_2"] = "0"

FESettings_A_49 = copy.deepcopy(FESettings_A_v48)

FESettings_A_49["VREF_ADC"] = "900"


if "v4-08" in Ph2_ACF_VERSION:
    for key in FESettings_DictA.keys():
        FESettings_DictA[key] = FESettings_A_v48       
        

if "v4-09" in Ph2_ACF_VERSION:
    for key in FESettings_DictA.keys():
        FESettings_DictA[key] = FESettings_A_49

if "v4-10" in Ph2_ACF_VERSION:
    for key in FESettings_DictA.keys():
        FESettings_DictA[key] = FESettings_A_49

if "v4-11" in Ph2_ACF_VERSION:
    for key in FESettings_DictA.keys():
        FESettings_DictA[key] = FESettings_A_49

if "v4-13" in Ph2_ACF_VERSION:
    for key in FESettings_DictA.keys():
        FESettings_DictA[key] = FESettings_A_49

if "v4-14" in Ph2_ACF_VERSION:
    for key in FESettings_DictA.keys():
        FESettings_DictA[key] = FESettings_A_49