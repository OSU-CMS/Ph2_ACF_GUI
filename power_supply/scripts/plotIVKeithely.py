#!/usr/bin/env python3
import argparse
import os
import pandas as pd
import numpy as np
import matplotlib
import matplotlib.pyplot as plt

def main(args):
    # Retriveing data from command line
    fileFolder          	= args.directory
    fileBaseName,fileExt	= os.path.splitext(args.filename)
    fileSaveDir     	        = os.path.join(fileFolder + '/Images/')
    nPoints         	        = args.nPoints

    if os.path.isdir(fileSaveDir) is False:
        os.mkdir(fileSaveDir)
    
    # Variables definition
    fileSaveNamePng 	= os.path.join(fileSaveDir, fileBaseName + '.png')
    fileSaveNamePdf 	= os.path.join(fileSaveDir, fileBaseName + '.pdf')
    fileName        	= os.path.join(fileFolder, fileBaseName + fileExt)
    
    if args.verbosity is True:
        print("Data folder: " + fileFolder)
        print("File Name: " + fileName)
        print("Save file png: " + fileSaveNamePng)
        print("Save file pdf: " + fileSaveNamePdf)
        print("Skip Header: ", args.skipHeader)
        print("Number of points: ", nPoints)

    if args.skipHeader is True:
        df = pd.read_csv(fileName,skiprows=1,sep='\t',header=None)
    else:
        df = pd.read_csv(fileName,sep='\t',header=None)
    df = pd.DataFrame(df)

    I = []
    Vin = np. array(df.iloc[:,0] .tolist())
    for vPoint in range(1,nPoints*2,2): 
        I.append(np. array(df.iloc[:,vPoint]. tolist()))
    I = np.transpose(I)

    for vPoint in range(0,nPoints):
        labelStr = 'DataSet ' + str(vPoint)
        VinPlt  , = plt.plot(Vin, I[:,vPoint] , 'o-', label = labelStr  )

    # Legend
    plt.legend(ncol=2)

    # Title and labels
    plt.gca().invert_xaxis()
    plt.gca().invert_yaxis()
    plt.ylabel('Current [A]')
    plt.xlabel('Voltage [V]')
    plt.title('IV curve')

    # Showing and saving
    #plt.gcf() .set_size_inches(7,5.2)
    plt.tight_layout()
    plt.grid()
    plt.savefig(fileSaveNamePdf,                     dpi = 1000)
    plt.savefig(fileSaveNamePng, transparent = True, dpi = 1000)
    if args.showPlot is True:
        plt.show()


parser = argparse.ArgumentParser(description='Plot of IV curve data using matplotlib and pandas')
parser.add_argument('-d','--directory', help = 'data and save folder', default = '../results/',type = str)
parser.add_argument('-f','--filename', help = 'save filename', default = 'LastIV.csv',type = str)
parser.add_argument('-n','--nPoints', help = 'number of current points per voltage value', default = 1,type = int)
parser.add_argument('-k','--skipHeader', help = 'skips first line of input file', action = 'store_true')
parser.add_argument('-s','--showPlot', help = 'shows plot', action = 'store_true')
parser.add_argument('-v','--verbosity', help = 'verbosity', action = 'store_true')
args = parser.parse_args()

if __name__ == "__main__":
    main(args)
