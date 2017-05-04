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

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/extlibs/vrm_cmake/cmake/")
include(vrm_cmake)

# Find `vrm_pp`:
vrm_cmake_find_extlib(vrm_pp)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/extlibs/hana/cmake/")
set(Hana_ROOT "${CMAKE_SOURCE_DIR}/extlibs/hana")
find_package(Hana)

refl_experiments_add_executable(comparisons comparisons.cpp)

if (Hana_FOUND)
  refl_experiments_add_executable(reflopt reflopt.cpp)
  refl_experiments_include_dirs(reflopt ${VRM_PP_INC_DIR} ${Hana_INCLUDE_DIRS})
  target_compile_options(reflopt_${refl_keyword} PUBLIC "-DBOOST_HANA_CONFIG_ENABLE_STRING_UDL")
else()
  message(WARNING "Hana was not found, reflopt example will not be built.")
endif()

refl_experiments_add_executable(serialization serialization.cpp)

# need to revisit if this is worth working on...
# refl_experiments_add_executable(type_synthesis type_synthesis.cpp)

# TODO: Run tests
