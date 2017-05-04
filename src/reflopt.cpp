/* Demo: Specify a struct for command line arguments
 * */

#include "reflopt.hpp"
#include <iostream>
#include <boost/hana/string.hpp>

#ifndef BOOST_HANA_CONFIG_ENABLE_STRING_UDL
static_assert(false, "reflopt example won't work without Hana string literals enabled");
#endif

using namespace boost::hana::literals;

struct ProgramOptions {
  REFLOPT_OPTION(std::string, filename, "--filename");
  REFLOPT_OPTION(int, iterations, "--iterations", "-i", "Number of times to run the algorithm.") = 100;
  REFLOPT_OPTION(bool, help, "--help", "-h", "Print help and exit");
};

int main(int argc, char** argv) {
  auto args = reflopt::parse<ProgramOptions>(argc, argv);
  if (!args) {
    std::cerr << "Argument parsing failed." << std::endl;
    return -1;
  }

  std::cout << args->filename << "\n";
  std::cout << args->iterations << "\n";

  return 0;
}
