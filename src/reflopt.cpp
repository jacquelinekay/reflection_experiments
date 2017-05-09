/* Demo: Specify a struct for command line arguments
 * */

#include "reflopt.hpp"
#include <iostream>
#include <boost/hana/string.hpp>

struct ProgramOptions {
  std::string filename;
  int iterations = 100;
  bool help = false;
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
