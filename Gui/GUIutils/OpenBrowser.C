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
   TGFileBrowser   *fBrowser;     

public:
   TBrowserWindow();
   virtual ~TBrowserWindow();
};


//______________________________________________________________________________
TBrowserWindow::~TBrowserWindow()
{
   // Destructor.

   delete fMain;
}

//______________________________________________________________________________
TBrowserWindow::TBrowserWindow()
{
   // Main  window.

   fMain = new TGMainFrame(gClient->GetRoot(), 10, 10, kVerticalFrame);
   fMain->SetCleanup(kDeepCleanup); // delete all subframes on exit

   fBrowser = new TGFileBrowser(fMain);
   fMain->SetEditable(kFALSE);
   fBrowser->AddFSDirectory("/", "/");
   fBrowser->GotoDir(gSystem->pwd());
   fMain->MapSubwindows();
   fMain->Layout();
   fMain->MapRaised();
}

////////////////////////////////////////////////////////////////////////////////
void OpenBrowser(TString DQMFile)
{
    std::cout<< DQMFile << std::endl;
    TGMainFrame  *fMain = new TGMainFrame(gClient->GetRoot(), 10, 10, kHorizontalFrame);
    fMain->SetCleanup(kDeepCleanup); // delete all subframes on exit
    fMain->SetEditable();

    TGVerticalFrame *fFileBrowser = new TGVerticalFrame(fMain, 10, 10, kFixedWidth);
    TGVerticalFrame *fContentViewer = new TGVerticalFrame(fMain, 10, 10);
 
    fMain->AddFrame(fFileBrowser, new TGLayoutHints(kLHintsLeft | kLHintsExpandY));

    TGFileBrowser *FileBrowser = new TGFileBrowser(fFileBrowser);
    fFileBrowser->AddFrame(FileBrowser, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
    FileBrowser->AddFSDirectory("/", "/");
    FileBrowser->GotoDir(gSystem->pwd());
    FileBrowser->Show();

    //TGListView* lv = new TGListView(fFileBrowser);
    //fFileBrowser->AddFrame(lv, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));

    //TGFileContainer *fc = new TGFileContainer(lv);
    //fc->Connect("DoubleClicked(TGFrame*,Int_t)", "TestFileList", fc,
    //                 "OnDoubleClick(TGLVEntry*,Int_t)");
    //fc->AddFile("../dummy.root");
    //fc->DisplayDirectory();
    


    TGCanvas *Canvas = new TGCanvas(fContentViewer, 10, 10);
    TGListTree *Contents = new TGListTree(Canvas, kHorizontalFrame);
    Contents->Associate(fContentViewer);
    fContentViewer->AddFrame(Canvas, new TGLayoutHints(kLHintsExpandX |
                                            kLHintsExpandY));


    fFileBrowser->Resize(300, fFileBrowser->GetDefaultHeight());
    fContentViewer->Resize(800, fFileBrowser->GetDefaultHeight());

    TGVSplitter *splitter = new TGVSplitter(fMain,2,2);
    splitter->SetFrame(fFileBrowser, kTRUE);
    fMain->AddFrame(splitter, new TGLayoutHints(kLHintsLeft | kLHintsExpandY));
 
    fMain->AddFrame(fContentViewer, new TGLayoutHints(kLHintsRight | kLHintsExpandX |
                                        kLHintsExpandY));
   
    fMain->Connect("CloseWindow()", "TApplication", gApplication, "Terminate()");
    fMain->DontCallClose();

    fMain->MapSubwindows();

    fMain->Resize(1500, 1000);

    TString title = "DQM browser";
    fMain->SetWindowName(title.Data());
    fMain->MapRaised();
}



