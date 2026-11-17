#pragma once

#include <opencv2/core.hpp>

// Owns one OpenGL texture holding a cv::Mat, for display with ImGui::Image.
class Texture {
public:
    Texture() = default;
    ~Texture();
    Texture(const Texture&)            = delete;
    Texture& operator=(const Texture&) = delete;

    // Uploads a BGR (CV_8UC3) or grayscale (CV_8UC1) image, replacing any previous contents.
    void upload(const cv::Mat& img);

    bool         valid()  const { return id_ != 0; }
    unsigned int id()     const { return id_; }
    int          width()  const { return width_; }
    int          height() const { return height_; }

private:
    unsigned int id_     = 0;
    int          width_  = 0;
    int          height_ = 0;
};
