##########################################################
# Program to convert mask files (*.txt) from Ph2_ACF DAQ #
# to Alki's DAQ                                          #
#                                       by Mauro Dinardo #
##########################################################
import sys
import csv

nCols = 432
nRows = 336

csvDataEnable = [[0 for x in range(nRows)] for y in range(nCols)]
csvDataHitBUS = [[0 for x in range(nRows)] for y in range(nCols)]
csvDataTDAC   = [[0 for x in range(nRows)] for y in range(nCols)]

def main():
    with open(sys.argv[1]) as fin:
        lines = fin.readlines()
        col   = 0

        for it, line in enumerate(lines):
            if line.find('ENABLE') != -1:

                lines[it] = lines[it].replace('ENABLE', '')
                enableLine = [ele.strip() for ele in lines[it].split(',')]

                lines[it + 1] = lines[it + 1].replace('HITBUS', '')
                hitBUSLine = [ele.strip() for ele in lines[it + 1].split(',')]

                lines[it + 3] = lines[it + 3].replace('TDAC', '')
                TDACLine = [ele.strip() for ele in lines[it + 3].split(',')]

                csvDataEnable[col][:] = enableLine
                csvDataHitBUS[col][:] = hitBUSLine
                csvDataTDAC[col][:]   = TDACLine

                col += 1

    with open('enable.csv', 'w', encoding='UTF8') as fcsv:
        writer = csv.writer(fcsv)
        csvDataEnableTranspose = [list(data) for data in zip(*csvDataEnable)]
        for data in csvDataEnableTranspose:
            writer.writerow(data)

    with open('hitor.csv', 'w', encoding='UTF8') as fcsv:
        writer = csv.writer(fcsv)
        csvDataHitBUSTranspose = [list(data) for data in zip(*csvDataHitBUS)]
        for data in csvDataHitBUSTranspose:
            writer.writerow(data)

    with open('tdac.csv', 'w', encoding='UTF8') as fcsv:
        writer = csv.writer(fcsv)
        csvDataTDACTranspose = [list(data) for data in zip(*csvDataTDAC)]
        for data in csvDataTDACTranspose:
            writer.writerow(data)

if __name__ == '__main__':
    main()
