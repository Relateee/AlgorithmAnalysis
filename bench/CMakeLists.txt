add_executable(seam_bench src/main.cpp)
target_link_libraries(seam_bench PRIVATE seamcore seam_warnings)
target_compile_definitions(seam_bench PRIVATE
    SEAM_ASSETS_DIR="${SEAM_ASSETS_DIR}"
    SEAM_RESULTS_DIR="${SEAM_RESULTS_DIR}")
