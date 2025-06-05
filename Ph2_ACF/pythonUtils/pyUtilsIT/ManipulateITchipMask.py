##############################################################
# Program to manupalte mask files (*.txt) of Ph2_ACF DAQ     #
#                                       by Mauro Dinardo     #
##############################################################
# Users can specify an enable/injection file of the form:    #
# row 0 col 130 en                                           #
# row 1 col 130 inj                                          #
# row 2 col 130 en                                           #

# row 0 col 129 en                                           #
# row 1 col 129 en                                           #
# row 2 col 129 en                                           #

# row 0 col 131 en                                           #
# row 1 col 131 en                                           #
# row 2 col 131 en                                           #
##############################################################
# Or they can specify that even(odd) columns are enabled and #
# odd(even) columns are injected: 'coupled'                  #
# Or they can specify that even rows are enabled and odd     #
# rows are injected: 'decoupled'                             #
##############################################################
# Users can also specify the group number, i.e. pattern      #
# number among the several possible (0, NROWS - 1)           #
##############################################################
# A png image is also saved to show the chosen pattern       #
##############################################################

from argparse import ArgumentParser
from PIL      import Image
import numpy as np

NCOLS     = 432
NROWS     = 336
NROW_CORE =   8
HITPERCOL =   1


class Mask(object):
    # Definition: col[row]
    def __init__(self):
        self.registers = []

        self.enable = []
        self.hitBUS = []
        self.injEN  = []
        self.TDAC   = []

    def reset(self):
        self.enable = [['0' for row in range(NROWS)] for col in range(NCOLS)]
        self.injEN  = [['0' for row in range(NROWS)] for col in range(NCOLS)]

    def copy(self, target):
        target.registers = self.registers[:]
        target.enable    = self.enable[:]
        target.hitBUS    = self.hitBUS[:]
        target.injEN     = self.injEN[:]
        target.TDAC      = self.TDAC[:]


def ArgParser():
    parser = ArgumentParser()

    parser.add_argument('-f', '--inFile',      dest = 'inFile',      type = str, help = 'Chip cfg file',          required = True,  default = '')
    parser.add_argument('-o', '--outFile',     dest = 'outFile',     type = str, help = 'Output file name',       required = True,  default = '')
    parser.add_argument('-m', '--maskFile',    dest = 'maskFile',    type = str, help = 'Used defined mask file', required = False, default = '')
    parser.add_argument('-p', '--patternType', dest = 'patternType', type = str, help = 'Pattern type: coupled or decoupled', required = False, default = '')
    parser.add_argument('-g', '--groupNumber', dest = 'groupNumber', type = int, help = 'Group number',           required = False, default = 0)

    options = parser.parse_args()

    if options.inFile:
        print('--> I\'m reading input file:', options.inFile)
    if options.outFile:
        print('--> I\'m reading output file:', options.outFile)
    if options.maskFile:
        print('--> I\'m reading mask file:', options.maskFile)
    if options.patternType:
        print('--> I\'m reading option make pattern:', options.patternType)
    if options.groupNumber:
        print('--> I\'m reading group number:', options.groupNumber)

    return options


def makeGroup(groupNumber, patternType):
    # Definition: black = injected, grey = enabled
    newMaskEn  = []
    newMaskInj = []

    for col in range(0, NCOLS):
        for i in range(HITPERCOL):
            row  = (NROW_CORE * col + i * NROWS//HITPERCOL)%NROWS
            row += groupNumber
            row %= NROWS

            ##########
            # Enable #
            ##########
            newMaskEn.append([row, col])

            ##########
            # Inject #
            ##########
            if patternType == 'coupled':
                if col % 2 == 0 and col + 1 < NCOLS:
                    newMaskInj.append([row, col + 1])
                elif col % 2 == 1 and col - 1 >= 0:
                    newMaskInj.append([row, col - 1])
            elif patternType == 'decoupled':
                if col % 2 == 0 and col + 1 < NCOLS and row - 1 >= 0:
                    newMaskInj.append([row - 1, col + 1])
                elif col % 2 == 1 and col - 1 >= 0 and row + 1 < NROWS:
                    newMaskInj.append([row + 1, col - 1])
            else:
                print('Option not recognized:', patternType)
                return [], []

    return newMaskEn, newMaskInj


def readMaskFile(fileName):
    # Definition: [[row, col], ...]
    newMaskEn  = []
    newMaskInj = []

    with open(fileName) as fin:
        lines = fin.readlines()

        for it, line in enumerate(lines):
            if line.find('en') != -1:
                newMaskEn.append([int(s) for s in line.split() if s.isdigit()])
            if line.find('inj') != -1:
                newMaskInj.append([int(s) for s in line.split() if s.isdigit()])

    return newMaskEn, newMaskInj


def applyMask(newMaskEn, newMaskInj, mask, orgMask):
    mask.reset()

    for en in newMaskEn:
        if orgMask.enable[en[1]][en[0]] == '1':
            mask.enable[en[1]][en[0]] = '1'

    for inj in newMaskInj:
        mask.injEN[inj[1]][inj[0]] = '1'


def readCFGfile(fileName):
    mask = Mask()

    with open(fileName) as fin:
        lines     = fin.readlines()
        col       = 0
        foundMask = False

        for it, line in enumerate(lines):
            if line.find('COL                  000') != -1:
                col += 1
                foundMask = True
            elif foundMask == True and line.find('COL') != -1:
                col += 1
            elif line.find('ENABLE') != -1:
                lines[it] = lines[it].replace('ENABLE', '')
                enableLine = [ele.strip() for ele in lines[it].split(',')]
                mask.enable.append(enableLine)
            elif line.find('HITBUS') != -1:
                lines[it] = lines[it].replace('HITBUS', '')
                hitBUSLine = [ele.strip() for ele in lines[it].split(',')]
                mask.hitBUS.append(hitBUSLine)
            elif line.find('INJEN') != -1:
                lines[it] = lines[it].replace('INJEN', '')
                injENLine = [ele.strip() for ele in lines[it].split(',')]
                mask.injEN.append(injENLine)
            elif line.find('TDAC') != -1:
                lines[it] = lines[it].replace('TDAC', '')
                TDACLine = [ele.strip() for ele in lines[it].split(',')]
                mask.TDAC.append(TDACLine)
            elif foundMask == False:
                mask.registers.append(line)

    return mask


def saveCFGfile(fileName, mask):
    with open(fileName, 'w', encoding='UTF8') as fout:
        for line in mask.registers:
            fout.write(line)

        for it in range(len(mask.enable)):
            fout.write('COL                  ' + f'{it:03}'  + '\n')
            fout.write('ENABLE ' + ','.join(mask.enable[it]) + '\n')
            fout.write('HITBUS ' + ','.join(mask.hitBUS[it]) + '\n')
            fout.write('INJEN  ' + ','.join(mask.injEN[it])  + '\n')
            fout.write('TDAC   ' + ','.join(mask.TDAC[it])   + '\n')
            fout.write('\n')


def saveImageOfPattern(newMaskEn, newMaskInj):
    thePattern = np.full((NROWS, NCOLS), 255) # While color

    for pixel in newMaskEn:
        thePattern[pixel[0], pixel[1]] = 150 # Grey color

    for pixel in newMaskInj:
        thePattern[pixel[0], pixel[1]] = 0 # Black color

    frame = Image.fromarray(np.uint8(thePattern))
    frame.save('InjectionSchema.png')


"""
#################
# Start program #
#################
"""

cmd     = ArgParser()
mask    = readCFGfile(cmd.inFile)
orgMask = Mask()
mask.copy(orgMask)

newMaskEn, newMaskInj = [], []
if cmd.maskFile:
    newMaskEn, newMaskInj = readMaskFile(cmd.maskFile)
    applyMask(newMaskEn, newMaskInj, mask, orgMask)
elif cmd.patternType:
    newMaskEn, newMaskInj = makeGroup(cmd.groupNumber, cmd.patternType)
    applyMask(newMaskEn, newMaskInj, mask, orgMask)

saveCFGfile(cmd.outFile, mask)
saveImageOfPattern(newMaskEn, newMaskInj)
