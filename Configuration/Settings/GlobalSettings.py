import os
import copy
Ph2_ACF_VERSION = os.environ.get("Ph2_ACF_VERSION")

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

'EN_MACRO_COL_CAL_SYNC_1': "65535",
'EN_MACRO_COL_CAL_SYNC_2': "65535",
'EN_MACRO_COL_CAL_SYNC_3': "65535",
'EN_MACRO_COL_CAL_SYNC_4': "65535",

'EN_MACRO_COL_CAL_DIFF_1': "65535",
'EN_MACRO_COL_CAL_DIFF_2': "65535",
'EN_MACRO_COL_CAL_DIFF_3': "65535",
'EN_MACRO_COL_CAL_DIFF_4': "65535",
'EN_MACRO_COL_CAL_DIFF_5':    "15",

'HITOR_0_MASK_LIN_0'     :     "0",
'HITOR_0_MASK_LIN_1'     :     "0",
'HITOR_1_MASK_LIN_0'     :     "0",
'HITOR_1_MASK_LIN_1'     :     "0",
'HITOR_2_MASK_LIN_0'     :     "0",
'HITOR_2_MASK_LIN_1'     :     "0",
'HITOR_3_MASK_LIN_0'     :     "0",
'HITOR_3_MASK_LIN_1'     :     "0",

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
'HITOR_0_CNT'            :     "0",
'HITOR_1_CNT'            :     "0",
'HITOR_2_CNT'            :     "0",
'HITOR_3_CNT'            :     "0",

}

globalSettings_Dict = {
    'Latency'                    :    globalSettings,
    'PixelAlive'                 :    globalSettings,
    'NoiseScan'                  :    globalSettings,
    'GainScan'                   :    globalSettings,
    'SCurveScan'                 :    globalSettings,
    'ThresholdEqualization'      :    globalSettings,
    'GainOptimization'           :    globalSettings,
    'ThresholdMinimization'      :    globalSettings,
    'ThresholdAdjustment'        :    globalSettings,
    'InjectionDelay'             :    globalSettings,
    'ClockDelay'                 :    globalSettings,
    'BitErrorRate'               :    globalSettings,
    'DataRBOptimization'         :    globalSettings,
    'ChipIntVoltageTuning'       :    globalSettings,
    'GenericDAC-DAC'             :    globalSettings,
    'Physics'                    :    globalSettings,
}

globalSettings_v48 = copy.deepcopy(globalSettings)
globalSettings_v48.pop('EN_MACRO_COL_CAL_LIN_1')
globalSettings_v48.pop('EN_MACRO_COL_CAL_LIN_2')
globalSettings_v48.pop('EN_MACRO_COL_CAL_LIN_3')
globalSettings_v48.pop('EN_MACRO_COL_CAL_LIN_4')
globalSettings_v48.pop('EN_MACRO_COL_CAL_LIN_5')

globalSettings_v48.pop('EN_MACRO_COL_CAL_SYNC_1')
globalSettings_v48.pop('EN_MACRO_COL_CAL_SYNC_2')
globalSettings_v48.pop('EN_MACRO_COL_CAL_SYNC_3')
globalSettings_v48.pop('EN_MACRO_COL_CAL_SYNC_4')

globalSettings_v48.pop('EN_MACRO_COL_CAL_DIFF_1')
globalSettings_v48.pop('EN_MACRO_COL_CAL_DIFF_2')
globalSettings_v48.pop('EN_MACRO_COL_CAL_DIFF_3')
globalSettings_v48.pop('EN_MACRO_COL_CAL_DIFF_4')
globalSettings_v48.pop('EN_MACRO_COL_CAL_DIFF_5')

globalSettings_v48['EN_CORE_COL_CAL_LIN_1'] = "65535"
globalSettings_v48['EN_CORE_COL_CAL_LIN_2'] = "65535"
globalSettings_v48['EN_CORE_COL_CAL_LIN_3'] = "65535"
globalSettings_v48['EN_CORE_COL_CAL_LIN_4'] = "65535"
globalSettings_v48['EN_CORE_COL_CAL_LIN_5'] =    "15"

globalSettings_v48['EN_CORE_COL_CAL_SYNC_1'] = "65535"
globalSettings_v48['EN_CORE_COL_CAL_SYNC_2'] = "65535"
globalSettings_v48['EN_CORE_COL_CAL_SYNC_3'] = "65535"
globalSettings_v48['EN_CORE_COL_CAL_SYNC_4'] = "65535"

globalSettings_v48['EN_CORE_COL_CAL_DIFF_1'] = "65535"
globalSettings_v48['EN_CORE_COL_CAL_DIFF_2'] = "65535"
globalSettings_v48['EN_CORE_COL_CAL_DIFF_3'] = "65535"
globalSettings_v48['EN_CORE_COL_CAL_DIFF_4'] = "65535"
globalSettings_v48['EN_CORE_COL_CAL_DIFF_5'] =    "15"

if "v4-08" in Ph2_ACF_VERSION:
    for key in globalSettings_Dict.keys():
      globalSettings_Dict[key] = globalSettings_v48
