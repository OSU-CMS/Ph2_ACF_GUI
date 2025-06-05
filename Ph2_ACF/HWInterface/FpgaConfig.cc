/* Copyright 2014 Institut Pluridisciplinaire Hubert Curien
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   FileName : 		FpgaConfig.cc
   Content : 		FPGA configuration
   Programmer : 	Christian Bonnin
   Version :
   Date of creation : 2014-07-10
   Support : 		mail to : christian.bonnin@iphc.cnrs.fr
*/

#include "HWInterface/FpgaConfig.h"
// #include <boost/format.hpp>
// #include <boost/thread.hpp>
// #include <fstream>
// #include <sys/stat.h>
// #include <time.h>

// using namespace std;
// using namespace Ph2_HwDescription;

namespace Ph2_HwInterface
{
FpgaConfig::FpgaConfig(RegManager&& pbbfi) : fwManager(std::move(pbbfi))
{
    numUploadingFpga = 0;
    progressValue    = 0;
    progressString   = "";
}
} // namespace Ph2_HwInterface
