#include "UI.h"

#include "DemoImage.h"
#include "FileDialog.h"

#include <seam/Energy.h>
#include <seam/SeamFinder.h>

#include <imgui.h>

#include <opencv2/imgcodecs.hpp>

#include <cstdint>
#include <algorithm>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <stdexcept>

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
    const ImVec2 available = ImGui::GetContentRegionAvail();
    const float width = static_cast<float>(tex.width());
    const float height = static_cast<float>(tex.height());
    const float scale = std::min({1.f, std::max(1.f, available.x) / width,
                                      std::max(1.f, available.y) / height});
    ImGui::Image(toImTexture(tex), ImVec2(width * scale, height * scale));
}

} // namespace

SeamApp::SeamApp() {
    // Default: assets/images/sample.jpg if you've added one, otherwise the built-in demo scene.
    std::snprintf(path_, sizeof(path_), "%s/images/sample.jpg", SEAM_ASSETS_DIR);
    if (!std::filesystem::is_regular_file(path_) || !loadFromPath(path_))
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
    // Indices match seam::Algorithm. Graph Cut stays disabled until Question 4 is implemented.
    static const char* kFinders[] = {"Dynamic Programming", "Greedy",
                                     "Graph Cut (Question 4: pending)"};
    constexpr int kImplementedFinders = 2;
    if (ImGui::BeginCombo("Seam finder", kFinders[algorithm_])) {
        for (int i = 0; i < IM_ARRAYSIZE(kFinders); ++i) {
            ImGui::BeginDisabled(i >= kImplementedFinders);
            if (ImGui::Selectable(kFinders[i], algorithm_ == i))
                algorithm_ = i;
            ImGui::EndDisabled();
        }
        ImGui::EndCombo();
    }
    if (ImGui::Combo("Energy", &energy_, "Gradient\0Sobel\0") && !original_.empty())
        refreshEnergy();

    if (algorithm_ == 0) {
        ImGui::Checkbox("Forward energy##dp", &dpForward_);
        ImGui::Checkbox("Rolling rows (5d)", &dpRolling_);
        ImGui::BeginDisabled();
        ImGui::SliderInt("Threads (5d)", &dpThreads_, 1, 16);
        ImGui::EndDisabled();
        ImGui::TextDisabled("Multi-threading: pending (Question 5d).");
    } else if (algorithm_ == 1) {
        ImGui::Checkbox("Try all start columns", &greedyAll_);
        ImGui::Checkbox("Preview: compare with DP", &compareDp_);
        ImGui::TextDisabled("DP seams are drawn in green underneath.");
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
    ImGui::Text("Pixel energy sum : %.1f", lastStats_.totalSeamCost);

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
        if (ImGui::BeginTabItem("Energy map")) {
            ImGui::TextDisabled("Brighter pixels have higher energy (normalized for display).");
            showImage(energyTex_);
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
    resultTex_.clear();
    targetW_  = img.cols;
    targetH_  = img.rows;
    lastStats_ = {};
    originalTex_.upload(overlay_);
    refreshEnergy();
}

void SeamApp::refreshEnergy() {
    const cv::Mat energy = seam::computeEnergy(seam::toGray(original_),
                                               static_cast<seam::EnergyType>(energy_));
    cv::Mat display;
    cv::normalize(energy, display, 0, 255, cv::NORM_MINMAX, CV_8U);
    energyTex_.upload(display);
}

void SeamApp::previewSeams() {
    // Draws the first seam of each orientation the current algorithm would remove.
    try {
        const auto energy = static_cast<seam::EnergyType>(energy_);
        overlay_ = original_.clone();

        // Question 3 comparison: draw DP's seams first (green) so the greedy seams sit on top.
        const bool compare = algorithm_ == static_cast<int>(seam::Algorithm::Greedy) && compareDp_;
        double dpVertical = 0, dpHorizontal = 0;
        if (compare) {
            seam::Carver dp(seam::makeSeamFinder(seam::Algorithm::DP, finderOptions()), energy);
            const seam::Seam v = dp.findSeam(original_, seam::Orientation::Vertical);
            dpVertical = dp.stats().totalSeamCost;
            const seam::Seam h = dp.findSeam(original_, seam::Orientation::Horizontal);
            dpHorizontal = dp.stats().totalSeamCost - dpVertical;
            overlay_ = seam::Carver::drawSeam(overlay_, v, seam::Orientation::Vertical, {0, 255, 0});
            overlay_ = seam::Carver::drawSeam(overlay_, h, seam::Orientation::Horizontal, {0, 255, 0});
        }

        seam::Carver carver(seam::makeSeamFinder(static_cast<seam::Algorithm>(algorithm_),
                                                 finderOptions()),
                            energy);
        const seam::Seam v = carver.findSeam(original_, seam::Orientation::Vertical);
        const double vertical = carver.stats().totalSeamCost;
        const seam::Seam h = carver.findSeam(original_, seam::Orientation::Horizontal);
        const double horizontal = carver.stats().totalSeamCost - vertical;
        overlay_ = seam::Carver::drawSeam(overlay_, v, seam::Orientation::Vertical, {0, 0, 255});
        overlay_ = seam::Carver::drawSeam(overlay_, h, seam::Orientation::Horizontal, {255, 0, 0});
        originalTex_.upload(overlay_);
        lastStats_ = carver.stats();

        status_ = "Red = vertical seam, blue = horizontal seam.";
        if (compare) {
            char buf[256];
            std::snprintf(buf, sizeof(buf),
                          "\nGreen = DP seams.\nVertical cost:   greedy %.1f vs DP %.1f\n"
                          "Horizontal cost: greedy %.1f vs DP %.1f",
                          vertical, dpVertical, horizontal, dpHorizontal);
            status_ += buf;
        }
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
        status_ = "Done. Open the Result tab to view the resized image.";
    } catch (const std::exception& e) {
        status_ = e.what();
    }
}

void SeamApp::smokeTest(const std::string& outputDirectory) {
    namespace fs = std::filesystem;
    fs::create_directories(outputDirectory);
    const auto output = [&](const char* name) { return (fs::path(outputDirectory) / name).string(); };
    const auto require = [](bool condition) {
        if (!condition) throw std::runtime_error("UI smoke test failed");
    };
    require(cv::imwrite(output("input.png"), makeDemoImage(96, 64)));
    require(loadFromPath(output("input.png")));
    require(originalTex_.valid() && energyTex_.valid() && !resultTex_.valid());
    for (int energy : {0, 1}) {
        energy_ = energy;
        refreshEnergy();
        previewSeams();
        require(lastStats_.seamsRemoved == 0 && overlay_.size() == original_.size());
        require(status_ == "Red = vertical seam, blue = horizontal seam.");
        targetW_ = 80;
        targetH_ = 52;
        carve();
        require(result_.cols == 80 && result_.rows == 52);
        require(lastStats_.seamsRemoved == 28);
        require(resultTex_.width() == 80 && resultTex_.height() == 52);
        require(cv::imwrite(output(energy == 0 ? "gradient.png" : "sobel.png"), result_));
    }
    require(cv::imwrite(output("seams.png"), overlay_));
    require(loadFromPath(output("input.png")));
    require(result_.empty() && !resultTex_.valid() && lastStats_.seamsRemoved == 0);
    previewSeams();
    targetW_ = 80;
    targetH_ = 52;
    carve();
}
