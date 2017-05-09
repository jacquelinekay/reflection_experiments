cmake_minimum_required(VERSION 3.5)

project(reflection_experiments VERSION 0.0.1 LANGUAGES CXX)
include(ExternalProject)

include(cmake/refl_experiments_add_executable.cmake)
include(cmake/refl_experiments_include_dirs.cmake)

unset(refl_keyword)
if(DEFINED REFLEXPR_PATH)
  set(refl_keyword "reflexpr" CACHE STRING "key for reflection facility headers")
  add_definitions(-DUSING_REFLEXPR)
  include_directories(${REFLEXPR_PATH})
elseif(DEFINED CPP3K_PATH)
  set(refl_keyword "cpp3k" CACHE STRING "key for reflection facility headers")
  add_definitions(-DUSING_CPP3K)
  include_directories(${CPP3K_PATH})
  set(CMAKE_CXX_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lc++abi")
else()
  message(FATAL_ERROR "No reflection fork was specified. Unable to build targets using reflection.")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/extlibs/hana/cmake/")
set(Hana_ROOT "${CMAKE_SOURCE_DIR}/extlibs/hana")
find_package(Hana)

# Build example executables
add_subdirectory(src)

if(ENABLE_TESTING)
  include(cmake/refl_experiments_add_test.cmake)
  include(CTest)
  add_subdirectory(test)
endif()
