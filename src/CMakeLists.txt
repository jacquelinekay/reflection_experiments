refl_experiments_add_executable(comparisons)

if (Hana_FOUND)
  refl_experiments_add_executable(reflopt)
  refl_experiments_include_dirs(reflopt ${VRM_PP_INC_DIR} ${Hana_INCLUDE_DIRS})
  target_compile_options(reflopt_${refl_keyword} PUBLIC "-DBOOST_HANA_CONFIG_ENABLE_STRING_UDL")
else()
  message(WARNING "Hana was not found, reflopt example will not be built.")
endif()

refl_experiments_add_executable(serialization)

# need to revisit if this is worth working on...
# refl_experiments_add_executable(type_synthesis type_synthesis.cpp)
