import os
import copy
Ph2_ACF_VERSION = os.environ.get("Ph2_ACF_VERSION")

globalSettingsA = {
'EN_CORE_COL_SYNC'       :     "0",
'EN_CORE_COL_LIN_1'      : "65535",
'EN_CORE_COL_LIN_2'      :     "1",
'EN_CORE_COL_DIFF_1'     :     "0",
'EN_CORE_COL_DIFF_2'     :     "0",
 
'EN_CORE_COL_CAL_LIN_1' : "65535",
'EN_CORE_COL_CAL_LIN_2' : "65535",
'EN_CORE_COL_CAL_LIN_3' : "65535",
'EN_CORE_COL_CAL_LIN_4' : "65535",
'EN_CORE_COL_CAL_LIN_5' :    "15",

'EN_CORE_COL_CAL_SYNC_1': "65535",
'EN_CORE_COL_CAL_SYNC_2': "65535",
'EN_CORE_COL_CAL_SYNC_3': "65535",
'EN_CORE_COL_CAL_SYNC_4': "65535",

'EN_CORE_COL_CAL_DIFF_1': "65535",
'EN_CORE_COL_CAL_DIFF_2': "65535",
'EN_CORE_COL_CAL_DIFF_3': "65535",
'EN_CORE_COL_CAL_DIFF_4': "65535",
'EN_CORE_COL_CAL_DIFF_5':    "15",

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

globalSettingsB = {
    "EN_CORE_COL_0"       :      "65535",
    "EN_CORE_COL_1"       :     "65535",
    "EN_CORE_COL_2"       :     "65535",
    "EN_CORE_COL_3"       :         "63",

    "EN_CORE_COL_CAL_0"    :     "65535",
    "EN_CORE_COL_CAL_1"    :     "65535",
    "EN_CORE_COL_CAL_2"    :     "65535",
    "EN_CORE_COL_CAL_3"    :        "63",

    "HITOR_MASK_0"          :    "65535",
    "HITOR_MASK_1"          :    "65535",
    "HITOR_MASK_2"          :    "65535",
    "HITOR_MASK_3"          :       "63",

    "PrecisionToTEnable_0"   :       "0",
    "PrecisionToTEnable_1"   :       "0",
    "PrecisionToTEnable_2"   :       "0",
    "PrecisionToTEnable_3"   :       "0",

    "EnHitsRemoval_0"         :      "0",
    "EnHitsRemoval_1"         :      "0",
    "EnHitsRemoval_2"         :      "0",
    "EnHitsRemoval_3"         :      "0",

    "EnIsolatedHitRemoval_0"   :     "0",
    "EnIsolatedHitRemoval_1"   :     "0",
    "EnIsolatedHitRemoval_2"   :     "0",
    "EnIsolatedHitRemoval_3"   :     "0",

    "HIT_SAMPLE_MODE"          :    "1",
    "EN_SEU_COUNT"             :    "0",
    "CDR_CONFIG_SEL_PD"        :    "0",

    "LOCKLOSS_CNT"             :     "0",
    "BITFLIP_WNG_CNT"          :     "0",
    "BITFLIP_ERR_CNT"          :     "0",
    "CMDERR_CNT"               :     "0",
    "SKIPPED_TRIGGER_CNT"      :     "0",
    "HITOR_0_CNT"              :     "0",
    "HITOR_1_CNT"              :     "0",
    "HITOR_2_CNT"              :     "0",
    "HITOR_3_CNT"              :     "0",
}

globalSettings_DictA = {
    'Latency'                    :    globalSettingsA,
    'PixelAlive'                 :    globalSettingsA,
    'NoiseScan'                  :    globalSettingsA,
    'GainScan'                   :    globalSettingsA,
    'SCurveScan'                 :    globalSettingsA,
    'ThresholdEqualization'      :    globalSettingsA,
    'GainOptimization'           :    globalSettingsA,
    'ThresholdMinimization'      :    globalSettingsA,
    'ThresholdAdjustment'        :    globalSettingsA,
    'InjectionDelay'             :    globalSettingsA,
    'ClockDelay'                 :    globalSettingsA,
    'BitErrorRate'               :    globalSettingsA,
    'DataRBOptimization'         :    globalSettingsA,
    'ChipIntVoltageTuning'       :    globalSettingsA,
    'GenericDAC-DAC'             :    globalSettingsA,
    'Physics'                    :    globalSettingsA,
    'IVCurve'                    :    globalSettingsA,
}

globalSettings_DictB = {
    'Latency'                    :    globalSettingsB,
    'PixelAlive'                 :    globalSettingsB,
    'NoiseScan'                  :    globalSettingsB,
    'GainScan'                   :    globalSettingsB,
    'SCurveScan'                 :    globalSettingsB,
    'ThresholdEqualization'      :    globalSettingsB,
    'GainOptimization'           :    globalSettingsB,
    'ThresholdMinimization'      :    globalSettingsB,
    'ThresholdAdjustment'        :    globalSettingsB,
    'InjectionDelay'             :    globalSettingsB,
    'ClockDelay'                 :    globalSettingsB,
    'BitErrorRate'               :    globalSettingsB,
    'DataRBOptimization'         :    globalSettingsB,
    'ChipIntVoltageTuning'       :    globalSettingsB,
    'GenericDAC-DAC'             :    globalSettingsB,
    'Physics'                    :    globalSettingsB,
    'IVCurve'                    :    globalSettingsB,

}

