/*!

        \file                           Hybrid.h
        \brief                          Hybrid Description class
        \author                         Lorenzo BIDEGAIN
        \version                        1.0
        \date                           25/06/14
        Support :                       mail to : lorenzo.bidegain@gmail.com

 */

#ifndef Hybrid_h__
#define Hybrid_h__

#include "FrontEndDescription.h"
#include "ReadoutChip.h"
#include "Utils/Container.h"
#include "Utils/Visitor.h"
#include "Utils/easylogging++.h"
#include <stdint.h>
#include <vector>

// FE Hybrid HW Description Class

/*!
 * \namespace Ph2_HwDescription
 * \brief Namespace regrouping all the hardware description
 */
namespace Ph2_HwDescription
{
/*!
 * \class Hybrid
 * \brief handles a vector of Chip which are connected to the Hybrid
 */
class Hybrid
    : public FrontEndDescription
    , public HybridContainer
{
  public:
    // C'tors take FrontEndDescription or hierachy of connection
    Hybrid(const FrontEndDescription& pFeDesc, uint8_t pHybridId);
    Hybrid(uint8_t pBeBoardId, uint8_t pFMCId, uint8_t pOpticalGroupId, uint8_t pHybridId);

    // Default C'tor
    Hybrid();

    Hybrid(const Hybrid&) = delete;

    // D'tor
    ~Hybrid() {};

    /*!
     * \brief acceptor method for HwDescriptionVisitor
     * \param pVisitor
     */
    void accept(HwDescriptionVisitor& pVisitor)
    {
        pVisitor.visitHybrid(*this);

        for(auto cChip: *this) static_cast<ReadoutChip*>(cChip)->accept(pVisitor);
    }
    /*!
     * \brief get the number of chip connected to the hybrid
     * \return the size of the vector
     */
    uint8_t getNChip() const { return this->size(); }

    void setNPixelChips(uint16_t cNPixelChips) { fNPixelChips = cNPixelChips; }
    void setNStripChips(uint16_t cNStripChips) { fNStripChips = cNStripChips; }

    uint16_t getNPixelChips() const { return fNPixelChips; }
    uint16_t getNStripChips() const { return fNStripChips; }

    /*!
     * \brief Set the I2C Master Id corresponding to the Chip
     * \param The I2C Master Id
     */
    void setMasterId(uint8_t pMasterId) { fMasterId = pMasterId; };

    /*!
     * \brief Get the I2C Master Id corresponding to the Chip
     * \return The I2C Master Id
     */
    uint8_t getMasterId() const { return fMasterId; };

  protected:
    uint16_t fNPixelChips = 0, fNStripChips = 0;
    uint8_t  fMasterId;
};
} // namespace Ph2_HwDescription

#endif
