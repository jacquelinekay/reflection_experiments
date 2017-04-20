/* Demo: Specify a struct for command line arguments
 * */

// TODO: pare down includes

#include "reflopt.hpp"
#include <iostream>

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
