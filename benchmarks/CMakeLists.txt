if(DEFINED REFLEXPR_PATH)

include(../cmake/refl_experiments_add_benchmark.cmake)
find_package(Boost REQUIRED COMPONENTS program_options)

foreach(N RANGE 5 50 5)
  refl_experiments_add_benchmark(reflopt ${N} 7 ${CMAKE_BINARY_DIR}/generated/reflopt_benchmark "refl")
  refl_experiments_add_benchmark(boost_po ${N} 7 ${CMAKE_BINARY_DIR}/generated/boost_po_benchmark "boost")
endforeach()

endif()
