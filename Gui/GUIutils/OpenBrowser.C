#include "TGComboBox.h"
#include "TGNumberEntry.h"
#include "TGLabel.h"
#include "TGColorSelect.h"
#include "TGHtml.h"
#include "TApplication.h"
#include "TGFileBrowser.h"
#include "TROOT.h"
#include "TBrowser.h"
#include "TString.h"
#include "TSystem.h"
#include "TGFrame.h"
#include "TGListView.h"
#include "TGFSContainer.h"
#include "TRootBrowser.h"
#include "TRootEmbeddedCanvas.h"
#include "TGCanvas.h"



class TBrowserWindow {

private:
   TGMainFrame    *fMain;       // main frame    

public:
   TBrowserWindow(TString DQMFile);
   virtual ~TBrowserWindow();
   void openTBrowser();
};


//______________________________________________________________________________
TBrowserWindow::~TBrowserWindow()
{
   // Destructor.

   delete fMain;
}

void TBrowserWindow::openTBrowser()
{
   new TBrowser();
}
//______________________________________________________________________________
TBrowserWindow::TBrowserWindow(TString DQMFile)
{
   std::cout<< DQMFile << std::endl;
   gSystem->ChangeDirectory(DQMFile);
   fMain = new TGMainFrame(gClient->GetRoot(), 10, 10, kVerticalFrame);
   fMain->SetCleanup(kDeepCleanup); // delete all subframes on exit
   fMain->SetEditable();

   TGCompositeFrame *fCframe = new TGCompositeFrame(fMain, 150, 40, kVerticalFrame|kFixedWidth);

   TGTextButton  *fStart = new TGTextButton(fCframe, "&View");
   fStart->Connect("Clicked()", "TBrowserWindow", this, "openTBrowser()");
   fCframe->AddFrame(fStart, new TGLayoutHints(kLHintsTop | kLHintsExpandX,
                                               0, 0, 0, 0));
   fStart->SetToolTipText("Click to view the DQM files");

   fMain->AddFrame(fCframe, new TGLayoutHints(kLHintsCenterX, 0, 0, 0, 0));

   TGTextButton  *fExit = new TGTextButton(fMain, "&Exit");
   fExit->Connect("Clicked()", "TApplication", gApplication, "Terminate()");
   fMain->AddFrame(fExit, new TGLayoutHints(kLHintsBottom | kLHintsExpandX,0,0,0,0));
   
   fMain->Connect("CloseWindow()", "TApplication", gApplication, "Terminate()");
   fMain->DontCallClose();

   fMain->MapSubwindows();

   fMain->Resize(150, 100);

   TString title = "DQM browser";
   fMain->SetWindowName(title.Data());
   fMain->MapRaised();
}

////////////////////////////////////////////////////////////////////////////////
void OpenBrowser(TString DQMFile)
{
    new TBrowserWindow(DQMFile);
}



