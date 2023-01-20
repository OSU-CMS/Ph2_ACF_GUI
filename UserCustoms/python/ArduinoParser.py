import re

def ArduinoParser(text):
    print(text)
    try:
        StopSingal, ProbeReadsText = ArduinoParserCustomOSU(text)
        return StopSingal, ProbeReadsText
    except Exception as err:
        return False,""


#################Arduino formatting for OSU###############
ProbeMapOSU = {
    'DHT11': "Humidity",
    'MAX31850': "Temperature(single wire)",
    'MAX31865': "Temperature(triple wire)"
}
ThresholdMapOSU = {
    'DHT11': [0,60],
    'MAX31850': [-20,50],
    'MAX31865': [-20,23]
}
def ArduinoParserCustomOSU(text):
    StopSignal = False
    values = re.split(" |\t",text)[1:]
    readValue = {}
    ProbeReads = []

    for index,value in enumerate(values):
        value = value.rstrip(":")
        if value in ProbeMapOSU.keys() and 'Temperature' not in values[index-1]:
            readValue[value] = float(values[index+1])

    for probeName,probeValue in readValue.items():
        if probeName in ThresholdMapOSU.keys():
            if type(ThresholdMapOSU[probeName])!= list or len(ThresholdMapOSU[probeName]) !=2:
                continue
            else:
                if probeValue < ThresholdMapOSU[probeName][0] or  probeValue > ThresholdMapOSU[probeName][1]:
                    colorCode = "#FF0000"
                    if probeName in ['MAX31850','MAX31865']:
                        StopSignal = True
                        
                else:
                    colorCode = "#008000"
                ProbeReads.append('{0}:<span style="color:{1}";>{2}</span>'.format(ProbeMapOSU[probeName],colorCode,probeValue))
                
        else:
            ProbeReads.append('{0}:{1}'.format(ProbeMapOSU[probeName],probeValue))
    ProbeReadsText = '\t'.join(ProbeReads)
    return StopSignal,ProbeReadsText

    
