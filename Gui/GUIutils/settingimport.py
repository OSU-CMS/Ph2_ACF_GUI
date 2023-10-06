from Ph2_ACF_GUI.parseVariables import variableParser
import csv
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)


class csvImport:
    def __init__(self):
        self.settingsTemplatePath = 'Gui/siteSettings_template.py'
        self.updatedsiteSettingsPath = 'Gui/siteSettings.py'
        self.updated_settingslist = {}
        self.parser = variableParser()
        self.defaultSettings = self.parser.parse(self.settingsTemplatePath)

    def import_settings(self, csvpath):
        csvstringdata = self._read_csv(csvpath)
        csvdata = self.parser.restoreOriginalType(csvstringdata)
        self._update_settings(csvdata)

    def _read_csv(self, readpath):
        updated_settings = self.defaultSettings.copy()  # Create a copy to preserve defaults
        
        noDefaultList = []
        unUsedData = []
        MissingDatalist = []
                
        try:
            with open(readpath, 'r', newline='') as csv_file:
                reader = csv.reader(csv_file)

                for row in reader:
                    if len(row) == 2:
                        header = row[0]
                        data = row[1]

                        if header == '':
                            unUsedData.append(data)
                        elif header not in self.defaultSettings:
                            noDefaultList.append(header)
                            if data == '' or data == 'None':
                                updated_settings[header] = None
                            else:
                                updated_settings[header] = data
                        else:
                            if data == '' or data == 'None':
                                updated_settings[header] = None
                            else:
                                updated_settings[header] = data

                    else:
                        MissingDatalist.append(row[0])
                        
            logger.info(f"CSV file '{readpath}' read successfully.")
            
            if len(noDefaultList) != 0:
                logger.info(f'The "siteSettings.py" does not have default values set for variables',noDefaultList)
            if len(unUsedData) != 0:
                logger.info(f'The following data {unUsedData} were not used because there was no header in the CSV file.')
            if len(MissingDatalist) != 0:
                logger.info(f'The following list {MissingDatalist} were skipped because there was no value associated or csv file had more than two columns.')
            
            return updated_settings

        except Exception as e:
            logger.error(f"Error reading CSV file: {str(e)}")

    def _update_settings(self,updated_settings_dict):
        try:
            # Write the updated settings (or create a new file) as siteSettings.py
            with open(self.updatedsiteSettingsPath, 'w') as settings_file:
                for key, value in updated_settings_dict.items():
                    settings_file.write(f"{key} = {repr(value)}\n")

            logger.info("siteSettings.py file updated/created successfully.")

        except Exception as e:
            logger.error(f"Error updating/creating siteSettings.py file: {str(e)}")

    def create_csv(self, writepath):
        try:
            # Open the CSV file for writing
            with open(writepath, 'w', newline='') as csv_file:
                writer = csv.writer(csv_file)

                # Write the headers in the first column and corresponding values in the second column
                for header, value in self.defaultSettings.items():
                    # Convert None to an empty string when writing to the CSV
                    value = '' if value is None else value
                    writer.writerow([header, value])

            logger.info(f"CSV file '{writepath}' created successfully.")

        except Exception as e:
            logger.error(f"Error creating CSV file: {str(e)}")

if __name__ == "__main__":
    test = csvImport()
    #create = test.create_csv('testcsv.csv')    #this will create the csv using the existing template
    test.import_settings('testcsv.csv')
