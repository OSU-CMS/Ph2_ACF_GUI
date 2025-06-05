/*!
  \file                  TEPXQuadNTC.h
  \brief                 Read out TEPX Quad NTC
  \author                PSI
  \version               1.0
  \date                  04/03/24
  Support:               none
*/

#ifndef TEPXQuadNTC_H
#define TEPXQuadNTC_H

#include "../tools/RD53CalibBase.h"

#ifdef __USE_ROOT__
#include "DQMUtils/RD53VoltageTuningHistograms.h"
#endif

// #############################
// # #
// #############################
class TEPXQuadNTC : public CalibBase
{
  public:
    ~TEPXQuadNTC()
    {
        // this->WriteRootFile();
        // this->CloseResultFile();
        //  delete histos;
    }

    void Running() override;
    void Stop() override;
    void ConfigureCalibration() override;
    void sendData() override;

    void localConfigure(const std::string& histoFileName = "", int currentRun = -1) override;
    void run() override;
    void draw(bool saveData = true) override;

    void  analyze();
    float TfromR(float R, const float& R25C, const float& beta);

  private:
    void         fillHisto() override;
    bool         fVerbose;
    double const fR_ref = 4.99; // reference resistor in kOh
  protected:
    int theCurrentRun;
};

#endif
