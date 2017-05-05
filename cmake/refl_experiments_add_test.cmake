# test options...
function(refl_experiments_add_test target_name)
  set(full_target ${target_name}_${refl_keyword}_test)
  add_executable(${full_target} ${target_name}.cpp)
  target_include_directories(${full_target} PUBLIC ${CMAKE_SOURCE_DIR}/include/reflection_experiments)

  target_compile_options(${full_target} PUBLIC "-std=c++1z;-Xclang;-freflection")

  add_test(${full_target}_ctest ${full_target} CONFIGURATIONS Debug)
endfunction()
