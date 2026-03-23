import os


def ExtractChipData(chipserial):
    DB_interface_step1 = f'''python python/rhapi.py --login --no-save-password --clean -u https://cmsdca.cern.ch/trk_rhapi1 "select c.* from trker_cmsr.c18220 c where c.PART_NAME_LABEL = '{chipserial}'"'''
    DB_interface_step2 = f'''python python/rhapi.py --login --no-save-password --clean -u https://cmsdca.cern.ch/trk_rhapi1 "select c.CROC_DATA_ID, c.Y from trker_cmsr.c18240 c where c.PART_NAME_LABEL = '{chipserial}' and c.CROC_DATA_ID like 'DAC_%_LIN' and c.X = 0"'''
    chipdataoutput_step1 = os.popen(DB_interface_step1).read()
    chipdataoutput_step1 = chipdataoutput_step1.split("\n")
    datelabel_step1 = chipdataoutput_step1[0].split(",")
    datavalue_step1 = chipdataoutput_step1[1].split(",")
    chipdata_step1 = dict(zip(datelabel_step1, datavalue_step1))
    chipdataoutput = os.popen(DB_interface_step2).read()
    chipdataoutput = chipdataoutput.split("\n")
    chipdata = {}
    for entry in chipdataoutput:
        if "," in entry:
            datalabel = entry.split(",")[0]
            datavalue = entry.split(",")[1]
            chipdata[datalabel] = datavalue
    chipdata.update(chipdata_step1)

    ##Convert to correct units and map dictionary keys to xml expectations
    chipdata.update({"VDDA": str(chipdata.get("VDDA_TRIM_CODE", "8")),
    "VDDD": str(chipdata.get("VDDD_TRIM_CODE", "8")),
    "IREF": str(chipdata.get("IREF_TRIM_CODE", "0")),
    "EFUSE": str(chipdata.get("EFUSE_CODE", "0")),
    "VREF": str(chipdata.get("VREF_ADC_V", "0")),
    "CINJ": str(chipdata.get("INJ_CAPACIT_F", "8e-12")),
    "ADC_OFFSET_VOLT": str(1e4*float(chipdata.get("ADC_OFF_V", "0"))),
    "ADC_MAXIMUM_VOLT": str(1e3*(4096*float(chipdata.get("ADC_SLO", "0"))+float(chipdata.get("ADC_OFF_V", "0")))),
    })
    return chipdata
