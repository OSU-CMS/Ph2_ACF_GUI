import pandas as pd
import numpy as np
import matplotlib
import matplotlib.pyplot as plt
import os, errno
import glob
import pathlib

################
# Fit function #
################

def ivCurveFit(I, Vin, minVal, lineColor = 'tab:blue', RextReference = -1):
    #indRight      = Vin > minVal
    #IShort        = I[indRight>minVal]
    #VinShort      = Vin[indRight]
    IShort        = I[Vin>minVal]
    VinShort      = Vin[Vin>minVal]
    m,q           = np.polyfit(IShort,VinShort,1)
    abline_values = [m * v + q for v in I]
    ivLinePlt  ,  = plt.plot(I     , abline_values, '--', color = lineColor , label = 'IV fit'          )
    ivShortPlt ,  = plt.plot(IShort, VinShort     , 'x' , color = 'black'   , label = 'Vin fit points'  )
    if RextReference > 0:
        textstr = '\n'.join((
            r'$m=%.3f$'     % (m, ),
            r'$q=%.3f$'     % (q, ),
            r'$R_{ext}=%d$' % (RextReference,)
            ))
    else:
        textstr = '\n'.join((
            r'$m=%.3f$'     % (m, ),
            r'$q=%.3f$'     % (q, )
            ))

    return ivLinePlt, ivShortPlt, textstr

####################
# Analyze function #
####################

def analyze(fileFolder, fileBaseName, minVal):
    """Analisys

    :fileFolder: Folder where file is located
    :fileBaseName: Base name for file to be analyzed
    :minVal: minimum value for fit

    """

    fileSaveDir  = fileFolder + 'Images/'
    fileSaveName = fileSaveDir + fileBaseName + '.png'
    fileName     = fileFolder + fileBaseName + '.csv'
    fileType     = 'None'

    try:
        os.makedirs(fileSaveDir)
    except FileExistsError:
        pass

    df = pd.read_csv(fileName)
    
    if 'Vout' in df:
        fileType = 'SingleSLDOIV'
    else:
        if 'VdddFirst' in df:
            fileType = '2SerieSLDOIV'
        else:
            fileType = 'SCCpoweredBySLDOIV'

    print("Analyzing with type "+fileType)

    if fileType is 'SingleSLDOIV':
        # Defining somewhere else measured values
        VrefReference = 0.6
        VoffReference = 0.5
        RextReference = 797

        # Plot variable
        yMin = -0.1
        yMax = 2.1

        # Final configurations
        figureWidth     = 8
        figureHeight    = 4.8

        # Getting data
        I     = np. array(df['Current']. tolist())
        Vin   = np. array(df['Vin'    ]. tolist())
        Vout  = np. array(df['Vout'   ]. tolist())
        Vref  = np. array(df['Vref'   ]. tolist())
        Voff  = np. array(df['Voff'   ]. tolist())
        VoffL = np. array(df['VoffL'  ]. tolist())

        # Plotting data
        VinPlt  , = plt.plot(I, Vin  , 'o', label = 'Vin'  )
        VoutPlt , = plt.plot(I, Vout , 'o', label = 'Vout' )
        VrefPlt , = plt.plot(I, Vref , 'o', label = 'Vref' )
        VoffPlt , = plt.plot(I, Voff , 'o', label = 'Voff' )
        VoffLPlt, = plt.plot(I, VoffL, 'o', label = 'VoffL')

        # Drawing reference lines
        #plt.gca().set_xlim(-0.1,2.6)
        xMin, xMax = plt.xlim()
        VrefSetPlt = plt.hlines(VrefReference  , xMin, xMax, colors = 'tab:green' , linestyles = 'dashed', label = 'Vref set'   )
        VoutSetPlt = plt.hlines(VrefReference*2, xMin, xMax, colors = 'tab:orange', linestyles = 'dashed', label = 'Vref*2 set' )
        VofHSetPlt = plt.hlines(VoffReference  , xMin, xMax, colors = 'tab:purple', linestyles = 'dashed', label = 'VoffH set'  )
        VoffSetPlt = plt.hlines(VoffReference*2, xMin, xMax, colors = 'tab:red'   , linestyles = 'dashed', label = 'VoffH*2 set')

        # IV curve fit
        ivLinePlt, ivShortPlt, textstr = ivCurveFit(I, Vin, minVal, RextReference = RextReference)

        # Legend
        plt.legend(
                handles =
                [
                VinPlt,
                VoutPlt,
                VrefPlt,
                VoffPlt,
                VoffLPlt,
                VrefSetPlt,
                VoutSetPlt,
                VoffSetPlt,
                VofHSetPlt,
                ivLinePlt,
                ivShortPlt
                ]
                )

    elif fileType is 'SCCpoweredBySLDOIV':
        # Defining somewhere else measured values
        VrefReferenceD = 0.6
        VoffReferenceD = 0.5
        VrefReferenceA = 0.6
        VoffReferenceA = 0.5

        # Plot variable
        yMin = -0.1
        yMax = 2.1

        # Final configurations
        figureWidth     = 8
        figureHeight    = 4.8

        # Getting data
        I      = np. array(df['Current']. tolist())
        Vin    = np. array(df['Vin'    ]. tolist())
        VrefD  = np. array(df['VrefD'   ]. tolist())
        VoffD  = np. array(df['VoffD'   ]. tolist())
        VrefA  = np. array(df['VrefA'   ]. tolist())
        VoffA  = np. array(df['VoffA'   ]. tolist())
        VddD   = np. array(df['Vddd'   ]. tolist())
        VddA   = np. array(df['Vdda'   ]. tolist())

        # Plotting data
        VinPlt   , = plt.plot(I, Vin   , 'o', label = 'Vin'  )
        VrefDPlt , = plt.plot(I, VrefD , 'o', label = 'VrefD' )
        VoffDPlt , = plt.plot(I, VoffD , 'o', label = 'VoffD' )
        VrefAPlt , = plt.plot(I, VrefA , 'o', label = 'VrefA' )
        VoffAPlt , = plt.plot(I, VoffA , 'o', label = 'VoffA' )
        VddDPlt  , = plt.plot(I, VddD  , 'o', label = 'Vddd' )
        VddAPlt  , = plt.plot(I, VddA  , 'o', label = 'Vdda' )

        # Drawing reference lines
        plt.gca().set_xlim(-0.1,2.6)
        xMin, xMax = plt.xlim()
        VrefDSetPlt = plt.hlines(VrefReferenceD  , xMin, xMax, colors = 'tab:orange' , linestyles = 'dashed', label = 'VrefD set'   )
        VoutDSetPlt = plt.hlines(VrefReferenceD*2, xMin, xMax, colors = 'tab:brown'  , linestyles = 'dashed', label = 'VrefD*2 set' )
        VrefASetPlt = plt.hlines(VrefReferenceA  , xMin, xMax, colors = 'tab:red'    , linestyles = 'dashed', label = 'VrefA set'   )
        VoutASetPlt = plt.hlines(VrefReferenceA*2, xMin, xMax, colors = 'tab:pink'   , linestyles = 'dashed', label = 'VrefA*2 set' )

        # IV curve fit
        ivLinePlt, ivShortPlt, textstr = ivCurveFit(I, Vin, minVal)

        # Legend
        plt.legend(
                handles =
                [
                VinPlt,
                VrefDPlt,
                VoffDPlt,
                VrefAPlt,
                VoffAPlt,
                VddDPlt,
                VddAPlt,
                VrefDSetPlt,
                VoutDSetPlt,
                VrefASetPlt,
                VoutASetPlt,
                ivLinePlt,
                ivShortPlt
                ]
                )
    elif fileType is '2SerieSLDOIV':
        # Defining somewhere else measured values
        VrefReferenceD = 0.6
        VoffReferenceD = 0.5
        VrefReferenceA = 0.6
        VoffReferenceA = 0.5

        # Plot variable
        yMin = -0.1
        yMax = 4.1

        # Final configurations
        figureWidth     = 9
        figureHeight    = 5.0

        # Getting data
        I           = np. array(df['Current'    ]. tolist())
        Vps         = np. array(df['Vps'        ]. tolist())
        Vin         = np. array(df['Vin'        ]. tolist())
        VrefD       = np. array(df['VrefD'      ]. tolist())
        VoffD       = np. array(df['VoffD'      ]. tolist())
        VrefA       = np. array(df['VrefA'      ]. tolist())
        VoffA       = np. array(df['VoffA'      ]. tolist())
        VddDFirst   = np. array(df['VdddFirst'  ]. tolist())
        VddAFirst   = np. array(df['VddaFirst'  ]. tolist())
        VddDSecond  = np. array(df['VdddSecond' ]. tolist())
        VddASecond  = np. array(df['VddaSecond' ]. tolist())

        # Plotting data
        VinPlt          , = plt.plot(I, Vin         , 'o', label = 'Vin'                        )
        VrefDPlt        , = plt.plot(I, VrefD       , 'o', label = 'VrefD'                      )
        VoffDPlt        , = plt.plot(I, VoffD       , 'o', label = 'VoffD'                      )
        VrefAPlt        , = plt.plot(I, VrefA       , 'o', label = 'VrefA'                      )
        VoffAPlt        , = plt.plot(I, VoffA       , 'o', label = 'VoffA'                      )
        VddDFirstPlt    , = plt.plot(I, VddDFirst   , 'o', label = 'Vddd first module in chain' )
        VddAFirstPlt    , = plt.plot(I, VddAFirst   , 'o', label = 'Vdda first module in chain' )
        VddDSecondPlt   , = plt.plot(I, VddDSecond  , 'o', label = 'Vddd second module in chain')
        VddASecondPlt   , = plt.plot(I, VddASecond  , 'o', label = 'Vdda second module in chain')
        VpsPlt          , = plt.plot(I, Vps         , 'o', label = 'Vps'                        )

        # Drawing reference lines
        plt.gca().set_xlim(-0.1,2.6)
        xMin, xMax = plt.xlim()
        VrefDSetPlt = plt.hlines(VrefReferenceD  , xMin, xMax, colors = 'tab:orange' , linestyles = 'dashed', label = 'VrefD set'   )
        VoutDSetPlt = plt.hlines(VrefReferenceD*2, xMin, xMax, colors = 'tab:brown'  , linestyles = 'dashed', label = 'VrefD*2 set' )
        VrefASetPlt = plt.hlines(VrefReferenceA  , xMin, xMax, colors = 'tab:red'    , linestyles = 'dashed', label = 'VrefA set'   )
        VoutASetPlt = plt.hlines(VrefReferenceA*2, xMin, xMax, colors = 'tab:pink'   , linestyles = 'dashed', label = 'VrefA*2 set' )

        # IV curve fit
        ivLinePlt   , ivShortPlt    , textstr     = ivCurveFit(I, Vin, minVal                           )
        ivTotLinePlt, ivTotShortPlt , textstrTot  = ivCurveFit(I, Vps, 2*minVal, lineColor = 'tab:cyan' )

        # Legend
        plt.legend(
                handles =
                [
                VinPlt,
                VrefDPlt,
                VoffDPlt,
                VrefAPlt,
                VoffAPlt,
                VddDFirstPlt,
                VddAFirstPlt,
                VddDSecondPlt,
                VddASecondPlt,
                VrefDSetPlt,
                VoutDSetPlt,
                VrefASetPlt,
                VoutASetPlt,
                VpsPlt,
                ivLinePlt,
                ivShortPlt,
                ivTotLinePlt,
                ivTotShortPlt
                ]
                )
        props2 = dict(boxstyle='round', facecolor='turquoise', alpha = 0.8)

    # Box with fit results
    props = dict(boxstyle='round', facecolor='royalblue', alpha = 0.8)
    plt.legend(
            #framealpha = 0,
            labelspacing    = 0.3,
            ncol            = 1,
            loc             = 'lower left',
            bbox_to_anchor  = (1, 0),
            )
    ax    = plt.gca()
    ax.set_ylim(yMin,yMax)
    ax.text(
        1.03,
        1,
        textstr,
        transform           = ax.transAxes,
        fontsize            = 14,
        horizontalalignment = 'left',
        verticalalignment   = 'top',
        bbox = props
        )
    if fileType is '2SerieSLDOIV':
        ax.text(
            1.28,
            1,
            textstrTot,
            transform           = ax.transAxes,
            fontsize            = 14,
            horizontalalignment = 'left',
            verticalalignment   = 'top',
            bbox = props2
        )

    # Title and labels
    plt.xlabel('Input current [A]')
    plt.ylabel('Voltage [V]')
    plt.title('IV curve')

    # Showing and saving
    plt.gcf() .set_size_inches(figureWidth, figureHeight)
    plt.tight_layout()
    plt.grid()
    plt.savefig(fileSaveName, transparent = True, dpi = 1000)
    plt.show()

########
# Main #
########

#def main():
#    """Analyze all the data in folder
#    """
# Variables definition
folderForLoop   =  '2020_05_15/'
fileList        = glob.glob(folderForLoop + '*.csv')
#fileList        = glob.glob(folderForLoop + 'lastScan.csv')

for vFile in fileList:
    fileBaseName = pathlib.Path(vFile).stem
    #fileType     = 'SCCpoweredBySLDOIV'
    #fileType     = 'SingleSLDOIV'
    #fileType     = '2SerieSLDOIV'
    minVal = 1.4
    analyze(folderForLoop, fileBaseName, minVal)

