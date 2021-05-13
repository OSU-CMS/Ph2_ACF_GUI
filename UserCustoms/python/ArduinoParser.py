def ArduinoParser(text):
    StopSingal, ProbeReadsText = ArduinoParserCustomOSU(text)
    return StopSingal, ProbeReadsText


#################Arduino formatting for OSU###############
ProbeMapOSU = {
    'DHT11': "Humidity",
    'MAX31850': "Temperature(single wire)",
    'MAX31865': "Temperature(triple wire)"
}
ThresholdMapOSU = {
    'DHT11': [0,60],
    'MAX31850': [-20,50],
    'MAX31865': [-20,23],
}
def ArduinoParserCustomOSU(text):
    StopSignal = False
    values = text.split(" ")[5:]
    readValue = {}
    ProbeReads = []
    for index,value in enumerate(values):
        value = value.rstrip(":")
        if value in ProbeMapOSU.keys():
            readValue[value] = str(values[index+1])

    for probeName,probeValue in readValue.items():
        if probeName in ThresholdMapOSU.keys():
            if type(ThresholdMapOSU[probeName])!= list or len(ThresholdMapOSU[probeName]) !=2:
                continue
            else:
                if probeValue < ThresholdMapOSU[probeName][0] or  probeValue > ThresholdMapOSU[probeName][1]:
                    colorCode = "#008000"
                    if probeName in ['MAX31850','MAX31865']:
                        StopSignal = True
                else:
                    colorCode = "#FF0000"
                ProbeReads.append('{0}:<p style="color:{1}";>{2}</p>'.format(ProbeMapOSU[probeName],colorCode,probeValue))
        else:
            ProbeReads.append('{0}:{1}'.format(ProbeMapOSU[probeName],probeValue))

    ProbeReadsText = '\n'.join(ProbeReads)
    return StopSignal,ProbeReadsText





