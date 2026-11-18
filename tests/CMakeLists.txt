add_executable(seam_tests src/main.cpp)
target_link_libraries(seam_tests PRIVATE seamcore seam_warnings)
add_test(NAME seam_tests COMMAND seam_tests)
set_tests_properties(seam_tests PROPERTIES LABELS "pending;questions3and4")

add_executable(seam_q1_tests src/q1.cpp)
target_link_libraries(seam_q1_tests PRIVATE seamcore seam_warnings)
add_test(NAME seam_q1_tests COMMAND seam_q1_tests)
set_tests_properties(seam_q1_tests PROPERTIES LABELS "q1")
