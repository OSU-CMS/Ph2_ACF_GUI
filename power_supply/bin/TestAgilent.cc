/*!
 * \authors Mattia Lizzo <mattia.lizzo@cern.ch>, INFN-Firenze
 * \authors Francesco Fiori <francesco.fiori@cern.ch>, INFN-Firenze
 * \authors Antonio Cassese <antonio.cassese@cern.ch>, INFN-Firenze
 * \date Sep 2 2019
 */

// Libraries
#include "DeviceHandler.h"
#include "Scope.h"
#include "ScopeAgilent.h"
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp> //!For command line arg parsing
#include <iomanip>
#include <iostream>
#include <limits>

// Custom Libraries
#include "TTi.h"

// Namespaces
using namespace std;
namespace po = boost::program_options;

/*!
************************************************
* A simple "wait for input".
************************************************
*/
void wait()
{
    cout << "Press ENTER to continue...";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

/*!
************************************************
* Argument parser.
************************************************
*/
po::variables_map process_program_options(const int argc, const char* const argv[])
{
    po::options_description desc("Allowed options");

    desc.add_options()("help,h", "produce help message")

        ("config,c",
         po::value<string>()->default_value("default"),
         "set configuration file path (default files defined for each test) "
         "...")("verbose,v", po::value<string>()->implicit_value("0"), "verbosity level");

    po::variables_map vm;
    try
    {
        po::store(po::parse_command_line(argc, argv, desc), vm);
    }
    catch(po::error const& e)
    {
        std::cerr << e.what() << '\n';
        exit(EXIT_FAILURE);
    }
    po::notify(vm);

    // Help
    if(vm.count("help"))
    {
        cout << desc << "\n";
        return 0;
    }

    // Power supply object option
    if(vm.count("object")) { cout << "Object to initialize set to " << vm["object"].as<string>() << endl; }

    return vm;
}

/*!
 ************************************************
 * Main.
 ************************************************
 */
int main(int argc, char* argv[])
{
    boost::program_options::variables_map v_map = process_program_options(argc, argv);
    std::cout << "Initializing ..." << std::endl;

    std::string        docPath = v_map["config"].as<string>();
    pugi::xml_document docSettings;

    DeviceHandler theHandler;
    theHandler.readSettings(docPath, docSettings);
    Scope* scope;
    try
    {
        theHandler.getScope("AgilentScope");
    }
    catch(const std::out_of_range& oor)
    {
        std::cerr << "Out of Range error: " << oor.what() << '\n';
    }
    scope                                 = theHandler.getScope("AgilentScope");
    ScopeAgilent*            agilentScope = dynamic_cast<ScopeAgilent*>(scope);
    std::vector<std::string> channels     = {"main"};
    for(auto& chan: channels)
    {
        ScopeAgilentChannel* channel = dynamic_cast<ScopeAgilentChannel*>(agilentScope->getChannel(chan));
        std::cout << "Acquiring" << std::endl;
        // std::string waveForm=channel->aquireWaveForm();
        std::string observables = "EHEight|EWIDth|JITTer RMS|JITTer PP|CROSsing";
        channel->setEOMeasurement(observables);
        channel->acquireEOM(100, 20, 80);
        //   std::cout << "done acquiring" << std::endl;

        //   vector<std::string> strs;
        //   vector<float> fdata;
        //   boost::split(strs,waveForm,boost::is_any_of(","));

        //   FILE*  pipe = popen("root -l", "w");
        //   fprintf(pipe, "TCanvas* c =new TCanvas();\n");
        //   fprintf(pipe, "TH1F* h = new TH1F(\"\",\"\",%d,-0.5,-0.5+%d);\n",int(strs.size()-1), int(strs.size()-1));
        //   for (unsigned int i=0; i < strs.size(); ++i){
        // 	if (strs.at(i) == "") continue;
        // 	fprintf(pipe, "h->SetBinContent(%d,%s);\n",int(i), strs.at(i).c_str());
        //   }
        //   fprintf(pipe, "h->Draw();\n");
        //   fprintf(pipe, "c->SaveAs(\"plot_%s.pdf\");\n", chan.c_str());
        //   fprintf(pipe, ".q\n");
        //   fflush(pipe);
        //   fclose(pipe);
    }
    return 0;
}
