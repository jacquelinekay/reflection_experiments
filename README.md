# Experiments with reflection

## reflexpr
To compile the example, compile the "reflexpr" fork of Clang by following the instructions on [Matúš's website](http://matus-chochlik.github.io/mirror/doc/html/implement/clang.html). Then pass `REFLEXPR_PATH` to CMake for this project.

```
mkdir build
cd build
cmake .. -DREFLEXPR_PATH=<path to the reflexpr fork>/reflection -DCMAKE_CXX_COMPILER=<reflexpr clang executable>
make
```

## cpp3k
Compile Andrew Sutton's [`clang-reflect`](https://github.com/asutton/clang-reflect) fork and pass `CPP3K_PATH` to CMake for this project.

```
mkdir build
cd build
cmake .. -DCPP3K_PATH=<path to the cpp3k fork>/tools/libcpp3k/include -DCMAKE_CXX_COMPILER=<clang-reflect executable>
make
```
