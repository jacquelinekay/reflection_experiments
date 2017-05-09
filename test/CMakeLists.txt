refl_experiments_add_test(reflopt)
refl_experiments_include_dirs(reflopt ${Hana_INCLUDE_DIRS})
target_compile_options(reflopt_${refl_keyword}_test PUBLIC "-DBOOST_HANA_CONFIG_ENABLE_STRING_UDL")

refl_experiments_add_test(serialization)
