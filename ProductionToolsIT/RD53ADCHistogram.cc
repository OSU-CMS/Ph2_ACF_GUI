/*!
  \file                  RD53ADCHistogram.cc
  \brief                 Implementaion of ADCHistogram
  \author                Umberto MOLINATTI
  \version               1.0
  \date                  16/02/21
  Support:               email to umberto.molinatti@cern.ch
*/

#include "RD53ADCHistogram.h"

using namespace Ph2_HwDescription;
using namespace Ph2_HwInterface;

void ADCHistogram::run()
{
    // #########################
    // # Mark enabled channels #
    // #########################
    auto RD53ChipInterface = static_cast<RD53Interface*>(this->fReadoutChipInterface);
    for(const auto cBoard: *fDetectorContainer)
        for(const auto cOpticalGroup: *cBoard)
            for(const auto cHybrid: *cOpticalGroup)
                for(const auto cChip: *cHybrid)
                {
                    for(int input = 0; input < max_counts; input++)
                    {
                        // if(input % 1000 == 0)
                        //	std::cout << "input: " << input << std::endl;
                        ADCcode.push_back(RD53ChipInterface->ReadChipADC(cChip, "IMUXoutput"));
                    }
                }
}

void ADCHistogram::draw()
{
#ifdef __USE_ROOT__
    // histos->fillADC(ADCcode, max_counts);
#endif
}
