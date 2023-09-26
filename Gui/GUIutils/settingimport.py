import settings as set
import csv


class csvImport:
    def __init__(self):
        ##The length of settingnamelist and settingslist should match
        
        self.settingnamelist = ['ExpertUserList','FirmwareList','dblist','FPGAConfigList','ModuleType','firmware_image','ModuleLaneMap',
                'BoxSize','HVPowerSupplyModel','LVPowerSupplyModel','PowerSupplyModel_Termination','PowerSupplyModel_XML_Termination',
                'ConfigFiles','Test','TestName2File','SingleTest','CompositeTest','CompositeList','pretuningList','tuningList',
                'posttuningList','updatedXMLValues','updatedGlobalValue','stepWiseGlobalValue','header']

        self.settingslist = [set.ExpertUserList,set.FirmwareList,set.dblist,set.FPGAConfigList,set.ModuleType,set.firmware_image,set.ModuleLaneMap,
                set.BoxSize,set.HVPowerSupplyModel,set.LVPowerSupplyModel,set.PowerSupplyModel_Termination,set.PowerSupplyModel_XML_Termination,
                set.ConfigFiles,set.Test,set.TestName2File,set.SingleTest,set.CompositeTest,set.CompositeList,set.pretuningList,set.tuningList,
                set.posttuningList,set.updatedXMLValues,set.updatedGlobalValue,set.stepWiseGlobalValue,set.header]

        self.updated_settingslist = []

    def read_csv(self,readpath):
        updated_settingslist = self.settingslist.copy()  # Create a copy to preserve defaults

        try:
            with open(readpath, 'r', newline='') as csv_file:
                reader = csv.DictReader(csv_file)
                headers_in_csv = reader.fieldnames  # Get the headers from the CSV file

                for row in reader:  # Iterate through the rows of the CSV file
                    for header in headers_in_csv:
                        if header in self.settingnamelist:
                            index = self.settingnamelist.index(header)
                            updated_settingslist[index] = row[header]
                        else:
                            print(f'The settingnamelist does not contain the header "{header}".\n' 
                                f'Either update the settingnamelist or check if "{header}" has any typos.')
                            raise KeyError()

            print(f"CSV file '{readpath}' read successfully.")
            return updated_settingslist

        except Exception as e:
            print(f"Error reading CSV file: {str(e)}")

    def create_csv(self,writepath):
        createddictionary = dict(zip(self.settingnamelist, self.settingslist))

        try:
            # Open the CSV file for writing
            with open(writepath, 'w', newline='') as csv_file:
                writer = csv.writer(csv_file)
                
                # Write the headers (keys of the dictionary)
                headers = createddictionary.keys()
                writer.writerow(headers)
                
                # Write the corresponding values (associated with each header)
                values = [createddictionary[header] for header in headers]
                writer.writerow(values)

            print(f"CSV file '{writepath}' created successfully.")

        except Exception as e:
            print(f"Error creating CSV file: {str(e)}")
    

if __name__ == "__main__":
    test = csvImport()
    #test.create_csv('testcsv.csv')
    result = test.read_csv('testcsv.csv')
    print(result)