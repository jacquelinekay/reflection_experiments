function(refl_experiments_include_dirs target_name)
  set(full_target ${target_name}_${refl_keyword})
  target_include_directories(${full_target} ${ARGN})
endfunction()
