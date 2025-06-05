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
parser.add_argument('name', metavar='histoname', help='histogram name')
parser.add_argument('--output','-o', metavar='outputdir', help='output directory', dest='output')
args = parser.parse_args()
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

from ROOT import gROOT, gStyle, TFile, TH1F, TLegend, TCanvas


def getHistogram(tfile, histoname):
    h = tfile.Get(histoname)
    if not h:
        raise Exception("Failed to load histogram {0}.".format(name))
    N = h.GetEntries()
    nbins = h.GetNbinsX()
    xmin = h.GetXaxis().GetXmin()
    xmax = h.GetXaxis().GetXmax()
    hname = h.GetName()
    h_norm = TH1F(hname, hname, nbins, xmin, xmax)
    for i in range(1,nbins+1):
        y = h.GetBinContent(i)
        h.SetBinContent(i, float(y))
    return h


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
    ## histograms
    h = getHistogram(tfile, name)
    ## canvas
    cname = name.replace('/','__').replace('(','').replace(')','')
    c1 = TCanvas(cname)
    h.Draw()
    cname = outdir+'/'+cname+'.png'
    c1.SaveAs(cname)