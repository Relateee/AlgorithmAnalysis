add_executable(seam_tests src/main.cpp)
target_link_libraries(seam_tests PRIVATE seamcore seam_warnings)
add_test(NAME seam_tests COMMAND seam_tests)
