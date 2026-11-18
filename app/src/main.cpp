#include "UI.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>

#include <cstdio>
#include <exception>
#include <filesystem>
#include <string>
#include <stdexcept>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

static void glfwErrorCallback(int error, const char* description) {
    std::fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

// Drag-and-drop a file onto the window to load it.
static void glfwDropCallback(GLFWwindow* window, int count, const char** paths) {
    if (count > 0)
        if (auto* app = static_cast<SeamApp*>(glfwGetWindowUserPointer(window)))
            app->loadFromPath(paths[0]);
}

int main(int argc, char** argv) {
    const bool smokeTest = argc > 1 && std::string(argv[1]) == "--smoke-test";
    const std::string smokeOutput = argc > 2 ? argv[2] : "results/ui-smoke";
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit())
        return 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    if (smokeTest)
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(1600, 900, "Seam Carving", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // vsync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    if (smokeTest)
        ImGui::GetIO().IniFilename = nullptr;
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    int exitCode = 0;
    try {
        SeamApp app; // scoped so its GL textures are freed before the context goes away
        glfwSetWindowUserPointer(window, &app);
        glfwSetDropCallback(window, glfwDropCallback);
        if (smokeTest)
            app.smokeTest(smokeOutput);

        int frames = 0;
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            app.draw();

            ImGui::Render();
            int w = 0, h = 0;
            glfwGetFramebufferSize(window, &w, &h);
            glViewport(0, 0, w, h);
            glClearColor(0.10f, 0.10f, 0.12f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            if (smokeTest && ++frames == 3) {
                cv::Mat rgba(h, w, CV_8UC4), bgr;
                glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data);
                cv::flip(rgba, rgba, 0);
                cv::cvtColor(rgba, bgr, cv::COLOR_RGBA2BGR);
                if (!cv::imwrite((std::filesystem::path(smokeOutput) / "ui.png").string(), bgr))
                    throw std::runtime_error("Could not save UI screenshot");
                glfwSetWindowShouldClose(window, GLFW_TRUE);
                std::puts("UI smoke test passed: load, preview, both energies, resize, clear result, render.");
            }
            glfwSwapBuffers(window);
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "%s\n", e.what());
        exitCode = 1;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return exitCode;
}
