add_executable(seam_tests src/main.cpp)
target_link_libraries(seam_tests PRIVATE seamcore seam_warnings)
add_test(NAME seam_tests COMMAND seam_tests)
set_tests_properties(seam_tests PROPERTIES LABELS "smoke")

add_executable(seam_q1_tests src/q1.cpp)
target_link_libraries(seam_q1_tests PRIVATE seamcore seam_warnings)
add_test(NAME seam_q1_tests COMMAND seam_q1_tests)
set_tests_properties(seam_q1_tests PROPERTIES LABELS "q1")

add_executable(seam_q3_tests src/q3.cpp)
target_link_libraries(seam_q3_tests PRIVATE seamcore seam_warnings)
add_test(NAME seam_q3_tests COMMAND seam_q3_tests)
set_tests_properties(seam_q3_tests PROPERTIES LABELS "q3")

add_executable(seam_q4_tests src/q4.cpp)
target_link_libraries(seam_q4_tests PRIVATE seamcore seam_warnings)
add_test(NAME seam_q4_tests COMMAND seam_q4_tests)
set_tests_properties(seam_q4_tests PROPERTIES LABELS "q4")

# Question 5 improvements: multi-threaded DP and incremental (local) energy updates.
add_executable(seam_q5_threads_tests src/q5_threads.cpp)
target_link_libraries(seam_q5_threads_tests PRIVATE seamcore seam_warnings)
add_test(NAME seam_q5_threads_tests COMMAND seam_q5_threads_tests)
set_tests_properties(seam_q5_threads_tests PROPERTIES LABELS "q5")

add_executable(seam_q5_energy_tests src/q5_energy.cpp)
target_link_libraries(seam_q5_energy_tests PRIVATE seamcore seam_warnings)
add_test(NAME seam_q5_energy_tests COMMAND seam_q5_energy_tests)
set_tests_properties(seam_q5_energy_tests PROPERTIES LABELS "q5")
