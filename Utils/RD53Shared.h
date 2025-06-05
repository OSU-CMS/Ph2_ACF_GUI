/*!
  \file                  RD53Shared.h
  \brief                 Shared constants/functions across RD53 classes
  \author                Mauro DINARDO
  \version               1.0
  \date                  28/06/18
  Support:               email to mauro.dinardo@cern.ch
*/

#ifndef RD53Shared_H
#define RD53Shared_H

#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>

#define BOOST_UBLAS_NDEBUG 1
#include <boost/numeric/ublas/lu.hpp>
#include <boost/numeric/ublas/matrix.hpp>

namespace Ph2_HwDescription
{
class RD53;
}

namespace Ph2_HwInterface
{
class RD53Interface;
}

namespace RD53Shared
{
extern Ph2_HwDescription::RD53*        firstChip;
extern Ph2_HwInterface::RD53Interface* chipInterface;

const char     RESULTDIR[]            = "Results";                                       // Directory containing the results
const float    PRECISION              = 1e-2;                                            // Resolution on computing observables
const float    SUPERPRECISION         = 1e-9;                                            // Super resolution on computing observables
const uint8_t  ISGOOD                 = 0;                                               // Encoding good channels
const uint8_t  ISMASKED               = 1;                                               // Encoding masked channels
const uint8_t  ISDISABLED             = 2;                                               // Encoding disabled channels
const float    ISFITERROR             = -3;                                              // Encoding fit errors
const uint8_t  MAXBITCHIPREG          = 16;                                              // Maximum number of bits of a chip register
const size_t   NTHREADS               = round(std::thread::hardware_concurrency() / 2.); // Number of potential threads for the current CPU (removing hyper-threading)
const uint16_t DEEPSLEEP              = 10000;                                           // [microseconds]
const uint16_t SUPERDEEPSLEEP         = 1000;                                            // [milliseconds]
const uint8_t  AURORASLEEP            = 100;                                             // [milliseconds]
const uint8_t  READOUTSLEEP           = 50;                                              // [microseconds]
const uint8_t  MAXATTEMPTS            = 10;                                              // Maximum number of attempts
const uint16_t MAXATTEMPTSCMDDISPATCH = 100;                                             // Maximum number of attempts to dispatch a command
const uint8_t  MAXSTEPS               = 10;                                              // Maximum number of steps for a scan
const uint8_t  NENDOFCALIB            = 2;                                               // Maximum number of end-of-calib data

enum class INJtype : uint8_t
{
    None,
    Analog,
    Digital,
    SelfTrigger,
    Custom,
    XtalkCoupled,
    XtalkDeCoupled
};

std::string fromInt2Str(int val);
std::string composeFileName(const std::string& configFileName, const std::string& fName2Add);
size_t      countBitsOne(size_t num);
void        resetDefaultFloat();
std::string gitGitCommit();

constexpr size_t setBits(size_t nBit2Set) { return (1L << nBit2Set) - 1; }

template <typename T>
static void setFirstChip(T& theDetectorContainer)
{
    try
    {
        firstChip = static_cast<Ph2_HwDescription::RD53*>(theDetectorContainer.getFirstObject()->getFirstObject()->getFirstObject()->getFirstObject());
    }
    catch(std::runtime_error& err)
    {
        firstChip = nullptr;
    }
}

template <typename T>
static void setChipInterface(T& theChipInterface)
{
    chipInterface = static_cast<Ph2_HwInterface::RD53Interface*>(&theChipInterface);
}

template <typename T>
inline void myMove(std::vector<T> source, std::vector<T>& destination)
{
    destination.insert(destination.end(), std::make_move_iterator(source.begin()), std::make_move_iterator(source.end()));
}

template <typename T>
T mtxInversion(const boost::numeric::ublas::matrix<T>& input, boost::numeric::ublas::matrix<T>& inverse)
{
    if(input.size1() != input.size2()) return 0;

    boost::numeric::ublas::matrix<T>                  mLU(input);
    boost::numeric::ublas::permutation_matrix<size_t> pivots(mLU.size1());

    bool isSingular;
    if((isSingular = boost::numeric::ublas::lu_factorize(mLU, pivots)) != false) return 0;

    T det = 1;
    for(unsigned int i = 0u; i < pivots.size(); i++)
    {
        if(static_cast<unsigned int>(pivots(i)) != i) det *= -1;
        det *= mLU(i, i);
    }

    inverse.assign(boost::numeric::ublas::identity_matrix<T>(mLU.size1()));
    boost::numeric::ublas::lu_substitute(mLU, pivots, inverse);

    return det;
}

} // namespace RD53Shared

#endif
