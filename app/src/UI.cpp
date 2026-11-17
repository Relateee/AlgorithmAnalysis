#include "UI.h"

#include "DemoImage.h"
#include "FileDialog.h"

#include <seam/Energy.h>
#include <seam/SeamFinder.h>

#include <imgui.h>

#include <opencv2/imgcodecs.hpp>

#include <cstdint>
#include <cstdio>
#include <exception>

namespace {

ImTextureID toImTexture(const Texture& t) {
    return (ImTextureID)(std::intptr_t)t.id();
}

void showImage(const Texture& tex) {
    if (!tex.valid()) {
        ImGui::TextDisabled("(nothing yet)");
        return;
    }
    ImGui::Text("%d x %d", tex.width(), tex.height());
    ImGui::Image(toImTexture(tex), ImVec2(static_cast<float>(tex.width()),
                                          static_cast<float>(tex.height())));
}

} // namespace

SeamApp::SeamApp() {
    // Default: assets/images/sample.jpg if you've added one, otherwise the built-in demo scene.
    std::snprintf(path_, sizeof(path_), "%s/images/sample.jpg", SEAM_ASSETS_DIR);
    if (!loadFromPath(path_))
        useDemoImage();
}

void SeamApp::draw() {
    drawControls();
    drawImages();
}

seam::FinderOptions SeamApp::finderOptions() const {
    seam::FinderOptions o;
    o.dp.forwardEnergy       = dpForward_;
    o.dp.rollingRows         = dpRolling_;
    o.dp.threads             = dpThreads_;
    o.greedy.tryAllStarts    = greedyAll_;
    o.graphCut.forwardEnergy = gcForward_;
    o.graphCut.maxFlow       = gcMaxFlow_ == 0 ? seam::MaxFlowAlgorithm::EdmondsKarp
                                               : seam::MaxFlowAlgorithm::Dinic;
    return o;
}

void SeamApp::drawControls() {
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(360, 560), ImGuiCond_FirstUseEver);
    ImGui::Begin("Controls");

    ImGui::SeparatorText("Image");
    ImGui::InputText("Path", path_, sizeof(path_));
    if (ImGui::Button("Load path"))
        loadFromPath(path_);
    ImGui::SameLine();
    if (ImGui::Button("Browse...")) {
        if (const auto chosen = openImageFileDialog())
            loadFromPath(*chosen);
    }
    ImGui::SameLine();
    if (ImGui::Button("Demo image"))
        useDemoImage();
    ImGui::TextDisabled("Tip: drag an image file onto the window.");

    ImGui::SeparatorText("Algorithm");
    ImGui::Combo("Seam finder", &algorithm_, "Dynamic Programming\0Greedy\0Graph Cut\0");
    ImGui::Combo("Energy", &energy_, "Gradient\0Sobel\0");

    if (algorithm_ == 0) {
        ImGui::Checkbox("Forward energy##dp", &dpForward_);
        ImGui::Checkbox("Rolling rows (5d)", &dpRolling_);
        ImGui::SliderInt("Threads (5d)", &dpThreads_, 1, 16);
    } else if (algorithm_ == 1) {
        ImGui::Checkbox("Try all start columns", &greedyAll_);
    } else {
        ImGui::Checkbox("Forward energy##gc", &gcForward_);
        ImGui::Combo("Max-flow", &gcMaxFlow_, "Edmonds-Karp\0Dinic\0");
        ImGui::TextWrapped("Graph cut is slow: try images around 200 px.");
    }

    ImGui::SeparatorText("Target size");
    const bool hasImage = !original_.empty();
    ImGui::BeginDisabled(!hasImage);
    ImGui::SliderInt("Width", &targetW_, 1, hasImage ? original_.cols : 1);
    ImGui::SliderInt("Height", &targetH_, 1, hasImage ? original_.rows : 1);

    if (ImGui::Button("Preview seams"))
        previewSeams();
    ImGui::SameLine();
    if (ImGui::Button("Carve"))
        carve();
    ImGui::EndDisabled();

    ImGui::SeparatorText("Last run");
    ImGui::Text("Seams removed : %d", lastStats_.seamsRemoved);
    ImGui::Text("Energy  (ms)  : %.2f", lastStats_.msEnergy);
    ImGui::Text("Find    (ms)  : %.2f", lastStats_.msFind);
    ImGui::Text("Remove  (ms)  : %.2f", lastStats_.msRemove);
    ImGui::Text("Total   (ms)  : %.2f", lastStats_.msTotal());
    ImGui::Text("Seam cost sum : %.1f", lastStats_.totalSeamCost);

    if (!status_.empty())
        ImGui::TextWrapped("%s", status_.c_str());

    ImGui::End();
}

void SeamApp::drawImages() {
    ImGui::SetNextWindowPos(ImVec2(380, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(1200, 860), ImGuiCond_FirstUseEver);
    ImGui::Begin("Images", nullptr, ImGuiWindowFlags_HorizontalScrollbar);
    if (ImGui::BeginTabBar("views")) {
        if (ImGui::BeginTabItem("Original / seams")) {
            showImage(originalTex_);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Result")) {
            showImage(resultTex_);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

bool SeamApp::loadFromPath(const std::string& path) {
    const cv::Mat img = cv::imread(path, cv::IMREAD_COLOR);
    if (img.empty()) {
        status_ = "Could not load: " + path;
        return false;
    }
    std::snprintf(path_, sizeof(path_), "%s", path.c_str());
    setImage(img);
    status_ = "Loaded " + path;
    return true;
}

void SeamApp::useDemoImage() {
    setImage(makeDemoImage());
    status_ = "Using the built-in demo image.";
}

void SeamApp::setImage(const cv::Mat& img) {
    original_ = img;
    overlay_  = img.clone();
    result_.release();
    targetW_  = img.cols;
    targetH_  = img.rows;
    lastStats_ = {};
    originalTex_.upload(overlay_);
}

void SeamApp::previewSeams() {
    // Draws the first seam of each orientation the current algorithm would remove.
    try {
        seam::Carver carver(seam::makeSeamFinder(static_cast<seam::Algorithm>(algorithm_),
                                                 finderOptions()),
                            static_cast<seam::EnergyType>(energy_));
        const seam::Seam v = carver.findSeam(original_, seam::Orientation::Vertical);
        const seam::Seam h = carver.findSeam(original_, seam::Orientation::Horizontal);
        overlay_ = seam::Carver::drawSeam(original_, v, seam::Orientation::Vertical, {0, 0, 255});
        overlay_ = seam::Carver::drawSeam(overlay_, h, seam::Orientation::Horizontal, {255, 0, 0});
        originalTex_.upload(overlay_);
        lastStats_ = carver.stats();
        status_    = "Red = vertical seam, blue = horizontal seam.";
    } catch (const std::exception& e) {
        status_ = e.what();
    }
}

void SeamApp::carve() {
    // Runs on the UI thread, so the window freezes during long carves - fine for a demo.
    try {
        seam::Carver carver(seam::makeSeamFinder(static_cast<seam::Algorithm>(algorithm_),
                                                 finderOptions()),
                            static_cast<seam::EnergyType>(energy_));
        result_    = carver.resize(original_, targetW_, targetH_);
        lastStats_ = carver.stats();
        resultTex_.upload(result_);
        status_ = "Done.";
    } catch (const std::exception& e) {
        status_ = e.what();
    }
}
