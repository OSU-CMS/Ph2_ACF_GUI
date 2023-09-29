from Ph2_ACF_GUI.parseVariables import variableParser
import csv

class csvImport:
    def __init__(self):
        self.settingsfilepath = 'Gui/siteSettings.py'
        self.updated_settingslist = {}
        self.parser = variableParser()
        self.defaultSettings = self.parser.parse(self.settingsfilepath)

    def read_csv(self, readpath):
        updated_settings = self.defaultSettings.copy()  # Create a copy to preserve defaults

        try:
            with open(readpath, 'r', newline='') as csv_file:
                reader = csv.DictReader(csv_file)
                
                # Read the single row of headers
                headers_in_csv = reader.fieldnames
                # Read the single row of data
                row = next(reader)
                
                noDefaultList = []
                
                if None in row:
                    unusedData = row[None]
                
                for header in headers_in_csv:
                    if header in self.defaultSettings:
                        updated_settings[header] = row[header]
                    else:
                        noDefaultList.append(header)
                        
                if len(noDefaultList) != 0:
                    print(f'The "siteSettings.py" does not have default values set for variables',noDefaultList)
                
                if len(headers_in_csv) < len(row):
                    print(f'The following data {unusedData} were not used because there was not enough headers in the CSV file.')

            print(f"CSV file '{readpath}' read successfully.")
            return updated_settings

        except Exception as e:
            print(f"Error reading CSV file: {str(e)}")

    def create_csv(self, writepath):
        try:
            # Open the CSV file for writing
            with open(writepath, 'w', newline='') as csv_file:
                writer = csv.writer(csv_file)

                # Write the headers (keys of the dictionary)
                headers = self.defaultSettings.keys()
                writer.writerow(headers)

                # Write the corresponding values (associated with each header)
                values = [self.defaultSettings[header] for header in headers]
                writer.writerow(values)

            print(f"CSV file '{writepath}' created successfully.")

        except Exception as e:
            print(f"Error creating CSV file: {str(e)}")

if __name__ == "__main__":
    test = csvImport()
    #create = test.create_csv('testcsv.csv')
    result = test.read_csv('testcsv.csv')
    result = test.parser.restoreOriginalType(result)
    print(result)
