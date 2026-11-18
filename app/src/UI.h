#pragma once

#include "Texture.h"

#include <seam/Carver.h>
#include <seam/Types.h>

#include <opencv2/core.hpp>

#include <string>

class SeamApp {
public:
    SeamApp();
    void draw(); // call once per ImGui frame

    // Loads an image from disk (Load button, Browse dialog, or drag-and-drop onto the window).
    bool loadFromPath(const std::string& path);
    void useDemoImage();
    // Exercises the real load/preview/carve/texture path in a hidden GL window.
    void smokeTest(const std::string& outputDirectory);

private:
    void drawControls();
    void drawImages();

    void setImage(const cv::Mat& img);
    void previewSeams();
    void carve();
    void refreshEnergy();
    seam::FinderOptions finderOptions() const;

    // Inputs
    char path_[512]{};
    int  algorithm_  = 0; // index into seam::Algorithm
    int  energy_     = 0; // index into seam::EnergyType
    int  targetW_    = 0;
    int  targetH_    = 0;
    bool dpForward_  = false;
    bool dpRolling_  = false;
    int  dpThreads_  = 1;
    bool greedyAll_  = false;
    bool compareDp_  = true;  // preview: also draw the DP seams in green (Question 3)
    bool gcForward_  = true;
    int  gcMaxFlow_  = 1; // 0 = Edmonds-Karp, 1 = Dinic

    // State
    cv::Mat          original_;
    cv::Mat          overlay_; // original with previewed seams drawn on it
    cv::Mat          result_;
    Texture          originalTex_;
    Texture          resultTex_;
    Texture          energyTex_;
    seam::CarveStats lastStats_;
    std::string      status_;
};
