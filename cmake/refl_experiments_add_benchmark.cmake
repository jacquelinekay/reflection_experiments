function(refl_experiments_add_benchmark benchmark_name N M output_prefix)
  set(output "${output_prefix}_${N}.cpp")
  set(full_name "${benchmark_name}_${N}")
  add_custom_command(
    OUTPUT ${full_name}_generated_ ${output}
    COMMAND python3 ${CMAKE_SOURCE_DIR}/benchmarks/generate_benchmark.py ${N} ${M} --in_filename ${CMAKE_SOURCE_DIR}/benchmarks/${benchmark_name}.cpp.empy --out_filename ${output}
    COMMAND ${CMAKE_COMMAND} -E touch ${full_name}_generated_
    DEPENDS
    ${CMAKE_SOURCE_DIR}/benchmarks/generate_benchmark.py
    ${CMAKE_SOURCE_DIR}/benchmarks/${benchmark_name}.cpp.empy
  )
  add_custom_target(${full_name}_generated DEPENDS ${full_name}_generated_)

  #list(GET build_type ${ARGN} 0)

  if("${ARGN}" STREQUAL "refl")
    # TODO Target name!
    add_executable(${full_name} ${output})
    target_include_directories(
      ${full_name} PUBLIC ${CMAKE_SOURCE_DIR}/include/reflection_experiments ${Hana_INCLUDE_DIRS})
    target_compile_options(${full_name} PUBLIC "-std=c++1z;-Xclang;-freflection;-DBOOST_HANA_CONFIG_ENABLE_STRING_UDL")

  elseif("${ARGN}" STREQUAL "boost")
    if(Boost_FOUND)
      add_executable(${full_name} ${output})
      target_include_directories(${full_name} PUBLIC ${Boost_INCLUDE_DIRS})
      target_link_libraries(${full_name} PUBLIC ${Boost_LIBRARIES})
    else()
      message(WARNING "Boost program_options not found, comparison will not be built")
    endif()
  else()
    message(FATAL_ERROR "build type was wrong")
  endif()
  add_dependencies(${full_name} ${full_name}_generated)
endfunction()

