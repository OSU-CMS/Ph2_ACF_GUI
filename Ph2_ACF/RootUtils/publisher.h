#include "RootWeb/include/rootweb.hh"
#include <string>
class TH1D;
namespace RootWeb
{
void  makeMainPage(RootWSite& site, const std::string& inFilename);
TH1D* createPlot();
void  prepareSiteStuff(RootWSite& site, std::string& directory, const std::string& run);
void  makeDQMmonitor(const std::string& inFilename, std::string& directory, const std::string& run);
} // namespace RootWeb
