'''
    Description:
        - Plot histogram
          given two ROOT files and hist name
    Requirements:
        - ROOT
    ________________________
    <emery.nibigira@cern.ch>
'''
import os, sys, argparse
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
parser = argparse.ArgumentParser(prog='python '+sys.argv[0])
parser.add_argument('rootfile', metavar='rootfile', help='ROOT file with full path')
parser.add_argument('name', metavar='canvasname', help='canvas name')
parser.add_argument('--output','-o', metavar='outputdir', help='output directory', dest='output')
args = parser.parse_args()
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

from ROOT import gROOT, gStyle, TFile, TH1F, TLegend, TCanvas


def getCanvas(tfile, canvasname):
    c = tfile.Get(canvasname)
    if not c:
        raise Exception("Failed to load canvas {0}.".format(name))
    return c


if __name__ == "__main__":

    gROOT.SetBatch()
    gStyle.SetOptStat(0)

    rfile = args.rootfile
    name = args.name       # 'Detector/Board_0/OpticalGroup_0/Hybrid_0/D_B(0)_O(0)_HybridNoiseDistribution_Hybrid(0)'
    outdir = '.'
    if args.output:
        outdir = args.output
        if outdir.endswith('/'): outdir = outdir.replace(outdir[-1],'')
        try:
            os.makedirs(outdir)
        except OSError:
            if not os.path.isdir(outdir):
                raise

    ## rootfile
    tfile = TFile(rfile, "READ")
    ## canvas
    c1 = getCanvas(tfile, name)
    cname = name.replace('/','__').replace('(','').replace(')','')
    c1.Draw()
    cname = outdir+'/'+cname+'.png'
    c1.SaveAs(cname)