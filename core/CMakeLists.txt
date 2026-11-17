find_package(OpenCV CONFIG REQUIRED)
find_package(Threads REQUIRED)

add_library(seamcore STATIC
    src/Energy.cpp
    src/SeamFinder.cpp
    src/DPSeamFinder.cpp
    src/GreedySeamFinder.cpp
    src/GraphCutSeamFinder.cpp
    src/MaxFlow.cpp
    src/Carver.cpp
)

target_include_directories(seamcore PUBLIC include)
target_link_libraries(seamcore
    PUBLIC  ${OpenCV_LIBS} Threads::Threads
    PRIVATE seam_warnings)
