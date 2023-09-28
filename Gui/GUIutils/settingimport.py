from Ph2_ACF_GUI.Gui import siteSettings as set
import csv

class csvImport:
    def __init__(self):
        self.settingslist = {
            'GPIB_DebugMode': set.GPIB_DebugMode,
            'defaultACFVersion': set.defaultACFVersion,
            'defaultFC7': set.defaultFC7,
            'defaultFC7IP': set.defaultFC7IP,
            'defaultFMC': set.defaultFMC,
            'defaultUSBPortHV': set.defaultUSBPortHV,
            'defaultHVModel': set.defaultHVModel,
            'defaultUSBPortLV': set.defaultUSBPortLV,
            'defaultLVModel': set.defaultLVModel,
            'defaultModuleType': set.defaultModuleType,
            'defaultPowerMode': set.defaultPowerMode,
            'defaultHVCurrentCompliance': set.defaultHVCurrentCompliance,
            'defaultHVsetting': set.defaultHVsetting,
            'defaultSensorBaudRate': set.defaultSensorBaudRate,
            'defaultDBServerIP': set.defaultDBServerIP,
            'defaultDBName': set.defaultDBName,
            'defaultTargetThr': set.defaultTargetThr,
            'defaultSLDOscanVoltage': set.defaultSLDOscanVoltage,
            'defaultSLDOscanMaxCurrent': set.defaultSLDOscanMaxCurrent,
            'ModuleCurrentMap': set.ModuleCurrentMap,
            'ModuleVoltageMapSLDO': set.ModuleVoltageMapSLDO,
            'ModuleVoltageMap': set.ModuleVoltageMap,
            'usePeltier': set.usePeltier,
            'defaultPeltierPort': set.defaultPeltierPort,
            'defaultPeltierBaud': set.defaultPeltierBaud,
            'defaultPeltierSetTemp': set.defaultPeltierSetTemp,
            'defaultPeltierMaxTemp': set.defaultPeltierMaxTemp,
            'defaultPeltierMaxTempDiff': set.defaultPeltierMaxTempDiff
        }

        self.updated_settingslist = {}

    def read_csv(self, readpath):
        updated_settingslist = self.settingslist.copy()  # Create a copy to preserve defaults

        try:
            with open(readpath, 'r', newline='') as csv_file:
                reader = csv.DictReader(csv_file)
                headers_in_csv = reader.fieldnames  # Get the headers from the CSV file

                for row in reader:  # Iterate through the rows of the CSV file
                    for header in headers_in_csv:
                        if header in self.settingslist:
                            updated_settingslist[header] = row[header]
                        else:
                            print(f'The settingslist does not contain the header "{header}".\n'
                                  f'Either update the settingslist or check if "{header}" has any typos.')

            print(f"CSV file '{readpath}' read successfully.")
            return updated_settingslist

        except Exception as e:
            print(f"Error reading CSV file: {str(e)}")

    def create_csv(self, writepath):
        try:
            # Open the CSV file for writing
            with open(writepath, 'w', newline='') as csv_file:
                writer = csv.writer(csv_file)

                # Write the headers (keys of the dictionary)
                headers = self.settingslist.keys()
                writer.writerow(headers)

                # Write the corresponding values (associated with each header)
                values = [self.settingslist[header] for header in headers]
                writer.writerow(values)

            print(f"CSV file '{writepath}' created successfully.")

        except Exception as e:
            print(f"Error creating CSV file: {str(e)}")

if __name__ == "__main__":
    test = csvImport()
    #test.create_csv('testcsv.csv')
    result = test.read_csv('testcsv.csv')
    print(result)
