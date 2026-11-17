find_package(imgui CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(seam_app
    src/main.cpp
    src/Texture.cpp
    src/UI.cpp
    src/DemoImage.cpp
    src/FileDialog.cpp
)

target_link_libraries(seam_app PRIVATE seamcore imgui::imgui glfw OpenGL::GL seam_warnings)
if(WIN32)
    target_link_libraries(seam_app PRIVATE comdlg32) # native Open File dialog
endif()
target_compile_definitions(seam_app PRIVATE SEAM_ASSETS_DIR="${SEAM_ASSETS_DIR}")
