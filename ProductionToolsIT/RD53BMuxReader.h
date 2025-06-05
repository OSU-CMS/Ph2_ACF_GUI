/*!
  \file                  RD53BMuxReader.h
  \brief
  \author
  \version               1.0
  \date
  Support:               none
*/

#ifndef RD53BMuxReader_H
#define RD53BMuxReader_H

#include "../tools/RD53CalibBase.h"

#ifdef __USE_ROOT__
#include "DQMUtils/RD53VoltageTuningHistograms.h"
#endif

// #############################
// # #
// #############################
class RD53BMuxReader : public CalibBase
{
  public:
    ~RD53BMuxReader()
    {
        // this->WriteRootFile();
        //  this->CloseResultFile();
        //   delete histos;
    }

    void Running() override;
    void Stop() override;
    void ConfigureCalibration() override;
    void sendData() override;

    void localConfigure(const std::string& histoFileName = "", int currentRun = -1) override;
    void run() override;
    void draw(bool saveData = true) override;

    void analyze();

  private:
    void fillHisto() override;

  protected:
    int theCurrentRun;
};

#endif
