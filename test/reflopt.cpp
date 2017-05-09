#include "reflopt.hpp"

#include <cassert>

#ifndef BOOST_HANA_CONFIG_ENABLE_STRING_UDL
static_assert(false, "reflopt example won't work without Hana string literals enabled");
#endif

struct ProgramOptions {
  std::string filename;
  int iterations;
  bool help;
};

int main() {
  {
    static const char* argv[] = {"exename", "--filename", "test", "--iterations", "50", "--help", "1"};
    int argc = 7;
    const auto args = reflopt::parse<ProgramOptions>(argc, argv);
    if (!args) {
      return 255;
    }

    assert(args->filename == "test");
    assert(args->iterations == 50);
    assert(args->help == true);
  }

  // Short flags
  {
    static const char* argv[] = {"exename", "--filename", "test", "-i", "60", "-h", "1"};
    int argc = 7;
    const auto args = reflopt::parse<ProgramOptions>(argc, argv);
    if (!args) {
      return 255;
    }

    assert(args->filename == "test");
    assert(args->iterations == 60);
    assert(args->help == true);
  }

  // Failures
  {
    static const char* argv[] = {"exename", "--asdf", "test", "-i", "60", "-h", "1"};
    int argc = 7;
    const auto args = reflopt::parse<ProgramOptions>(argc, argv);
    assert(!args);
  }
  {
    static const char* argv[] = {"exename", "--filename", "-i", "60", "-h", "1"};
    int argc = 7;
    const auto args = reflopt::parse<ProgramOptions>(argc, argv);
    assert(!args);
  }

  return 0;
}
