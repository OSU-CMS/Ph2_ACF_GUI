/*!
  \file                           FrontEndDescription.h
  \brief                          FrontEndDescription base class to describe all parameters common to all FE Components
  in the DAQ chain \author                         Lorenzo BIDEGAIN \version                        1.0 \date 25/06/14
  Support :                       mail to : lorenzo.bidegain@gmail.com
*/

#ifndef FrontEndDescription_H
#define FrontEndDescription_H

#include "Definition.h"
#include <stdint.h>
/*!
 * \namespace Ph2_HwDescription
 * \brief Namespace regrouping all the hardware description
 */
namespace Ph2_HwDescription
{
/*!
 * \class FrontEndDescription
 * \brief Describe all parameters common to all FE Components in the DAQ chain
 */
class FrontEndDescription
{
  public:
    // 3 C'tors with different parameter sets
    FrontEndDescription(uint8_t pBeBoardId, uint8_t pFMCId, uint8_t pOpticalGroupId, uint8_t pHybridId, bool pStatus = true, FrontEndType pType = FrontEndType::UNDEFINED);
    FrontEndDescription();

    // Copy C'tors
    FrontEndDescription(const FrontEndDescription& pFeDesc);

    // Default D'tor
    virtual ~FrontEndDescription();

    /*!
     * \brief Get the Be ID
     * \return The Be ID
     */
    uint8_t getBeBoardId() const { return fBeBoardId; }

    /*!
     * \brief Get the Optical ID
     * \return The Optical ID
     */
    uint8_t getOpticalGroupId() const { return fOpticalGroupId; }

    /*!
     * \brief Get the FMC ID
     * \return The FMC ID
     */
    uint8_t getFMCId() const { return fFMCId; }

    /*!
     * \brief Get the Hybrid Id
     * \return The Hybrid Id
     */
    uint8_t getHybridId() const { return fHybridId; }

    /*!
     * \brief Get the Status
     * \return The Status
     */
    bool getStatus() const { return fStatus; }

    // Setter methods

    /*!
     * \brief Set the status
     * \param pStatus
     */
    void setStatus(bool pStatus) { fStatus = pStatus; }

    void setFrontEndType(FrontEndType pType) { fType = pType; }

    void         setOptical(bool pOptical) { fOptical = pOptical; }
    bool         isOptical() { return fOptical; }
    FrontEndType getFrontEndType() const { return fType; }

    void    setReset(uint8_t pReset) { fReset = pReset; }
    uint8_t getReset() { return fReset; }

    static std::string getFrontEndName(const FrontEndType theFrontEndType)
    {
        if(theFrontEndType == FrontEndType::HYBRID) return "HYBRID";
        if(theFrontEndType == FrontEndType::CBC3) return "CBC3";
        if(theFrontEndType == FrontEndType::MPA2) return "MPA2";
        if(theFrontEndType == FrontEndType::SSA2) return "SSA2";
        if(theFrontEndType == FrontEndType::RD53A) return "RD53A";
        if(theFrontEndType == FrontEndType::RD53Bv1) return "RD53Bv1";
        if(theFrontEndType == FrontEndType::RD53Bv2) return "RD53Bv2";
        if(theFrontEndType == FrontEndType::CIC2) return "CIC2";
        if(theFrontEndType == FrontEndType::OuterTracker2S) return "OuterTracker2S";
        if(theFrontEndType == FrontEndType::OuterTrackerPS) return "OuterTrackerPS";
        if(theFrontEndType == FrontEndType::InnerTrackerDouble) return "InnerTrackerDouble";
        if(theFrontEndType == FrontEndType::InnerTrackerQuad) return "InnerTrackerQuad";
        if(theFrontEndType == FrontEndType::HYBRID2S) return "HYBRID2S";
        if(theFrontEndType == FrontEndType::HYBRIDPS) return "HYBRIDPS";
        if(theFrontEndType == FrontEndType::LpGBT) return "LpGBT";
        if(theFrontEndType == FrontEndType::VTRx)
            return "VTRx";
        else
            return "UNDEFINED";
    }

  protected:
    // BIO Board Id that the FE is connected to
    uint8_t fBeBoardId;
    // Id of the FMC Slot on the BIO Board, all FEs need to know so the right FW registers can be written
    uint8_t fFMCId;
    // Id of the Optical group (link # , etc.. )
    uint8_t fOpticalGroupId;
    // Id of the FE (hybrid/hybrid, etc...)
    uint8_t fHybridId;
    // Enable reset
    uint8_t fReset{1};

    // status (true=active, false=disabled)
    bool fStatus;
    // Front End type enum (HYBRID, CBC2, CBC3, ...)
    FrontEndType fType;
    // optical or electrical communication with back-end
    bool fOptical;
};
} // namespace Ph2_HwDescription

#endif
