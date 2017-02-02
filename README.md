#Experiments with "reflexpr"

To compile the example, compile the "reflexpr" fork of Clang by following the instructions on [Matúš's website](http://matus-chochlik.github.io/mirror/doc/html/implement/clang.html).

Then, in this directory, run `make` with the right environment variables to the source directory where you cloned clang and the compiled executable, for example:

```
CLANG_DIR=/home/jackie/code/llvm/tools/clang CXX=/usr/local/bin/clang++ make
```
