#include "publisher.h"
#include "TCanvas.h"
#include "TClass.h"
#include "TDirectory.h"
#include "TFile.h"
#include "TH1D.h"
#include "TKey.h"
#include <cstdlib>

namespace RootWeb
{
void makeMainPage(RootWSite& site, const std::string& inFilename)
{
    auto& myPage = site.addPage("DQM Plots");
    myPage.setAddress("index.html");

    // With a content containing some plots
    auto& content1 = myPage.addContent("Common Plots");

    // open DQM file
    TFile*             fin = TFile::Open(inFilename.c_str());
    std::vector<TKey*> directoryKey;
    TIter              next(fin->GetListOfKeys());
    TObject*           obj;
    // Header etc.
    while((obj = next()))
    {
        TKey*   key = dynamic_cast<TKey*>(obj);
        TClass* cl  = gROOT->GetClass(key->GetClassName());
        if(cl->InheritsFrom("TDirectory"))
            directoryKey.push_back(key);
        else if(cl->InheritsFrom("TH1"))
        {
            TH1* h = dynamic_cast<TH1*>(key->ReadObj());
            h->SetDirectory(0);
            std::string canvasName("c");
            canvasName += key->GetName();
            TCanvas* myCanvas = new TCanvas(canvasName.c_str(), "c1", 700, 500);
            myCanvas->cd();
            h->Draw();
            auto& myImage = content1.addImage(myCanvas, 700, 500);
            myImage.setComment("A little explanation here always helps");
            myImage.setName(h->GetTitle());
        }
    }
    std::cout << "DirKeyVec size=" << directoryKey.size() << std::endl;

    // CBC Plots
    for(auto& key: directoryKey)
    {
        auto&       content2 = myPage.addContent("Plots for " + std::string(key->GetName()));
        TDirectory* dir      = dynamic_cast<TDirectory*>(key->ReadObj());
        TIter       next(dir->GetListOfKeys());
        TKey*       hkey;
        while((hkey = dynamic_cast<TKey*>(next())))
        {
            TClass* cl = gROOT->GetClass(hkey->GetClassName());
            if(!cl->InheritsFrom("TH1")) continue;
            TH1* h = dynamic_cast<TH1*>(hkey->ReadObj());
            h->SetDirectory(0);
            std::string canvasName("c");
            canvasName += hkey->GetName();
            TCanvas* myCanvas = new TCanvas(canvasName.c_str(), "c1", 700, 500);
            myCanvas->cd();
            h->Draw();
            auto& myImage = content2.addImage(myCanvas, 700, 500);
            myImage.setComment("A little explanation here always helps");
            myImage.setName(h->GetTitle());
        }
    }
    fin->Close();

    auto& content3 = myPage.addContent("All plots here");
    content3.addBinaryFile(inFilename, "content of all your plots is here", inFilename);
}

void prepareSiteStuff(RootWSite& site, std::string& directory, const std::string& run)
{
    directory += run;
    site.setTargetDirectory(directory);
    site.setTitle(run);
    site.setComment("Complete run list");
    site.setCommentLink("../");
    site.addAuthor("Ph2_DAQ Team");
    site.setRevision("0.1");
    site.setProgram("Ph2_DAQ", "https://gitlab.cern.ch/cmstkph2/Ph2_ACF");
}

void makeDQMmonitor(const std::string& inFilename, std::string& directory, const std::string& run)
{
    RootWSite site;
    prepareSiteStuff(site, directory, run);
    makeMainPage(site, inFilename);
    site.makeSite(false);
}
} // namespace RootWeb
