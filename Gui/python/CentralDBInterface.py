import os, sys

def ExtractChipData(chipserial):
    #sqlcommand = f"select c.PART_NAME_LABEL, c.VDDA_TRIM_CODE, c.VDDD_TRIM_CODE, c.EFUSE_CODE from trker_cmsr.c18220 c where c.PART_NAME_LABEL = '{chipserial}'" #example of chipserial is N61F26_16E6_45
    command = f'''python rhapi.py -nx -u https://cmsdca.cern.ch/trk_rhapi "select c.PART_NAME_LABEL, c.VDDA_TRIM_CODE, c.VDDD_TRIM_CODE, c.EFUSE_CODE from trker_cmsr.c18220 c where c.PART_NAME_LABEL = '{chipserial}'"'''
    #chipdata = os.popen(command).read()
    chipdataoutput = os.popen(command).read()
    chipdataoutput = chipdataoutput.split('\n')
    datalabel = chipdataoutput[0].split(',')
    datavalue = chipdataoutput[1].split(',')
    chipdata = dict(zip(datalabel, datavalue))
    return chipdata
    
