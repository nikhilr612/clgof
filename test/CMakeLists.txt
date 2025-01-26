# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(clgofTests LANGUAGES CXX)

# ---- Tests ----

add_executable(clgof_test source/clgof_test.cpp)
target_link_libraries(clgof_test PRIVATE clgof_lib)
target_compile_features(clgof_test PRIVATE cxx_std_17)

add_test(NAME clgof_test COMMAND clgof_test)

# ---- End-of-file commands ----

add_folders(Test)
