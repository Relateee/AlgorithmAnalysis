#include "DemoImage.h"

#include <opencv2/imgproc.hpp>

cv::Mat makeDemoImage(int width, int height) {
    cv::Mat img(height, width, CV_8UC3);
    const int horizon = height * 2 / 3;

    // Sky gradient (BGR) above the horizon, grass below.
    for (int y = 0; y < height; ++y) {
        cv::Vec3b c;
        if (y < horizon) {
            const double t = static_cast<double>(y) / horizon;
            c = cv::Vec3b(static_cast<uchar>(235 + 15 * t), static_cast<uchar>(190 + 40 * t),
                          static_cast<uchar>(120 + 80 * t));
        } else {
            c = cv::Vec3b(60, 140, 70);
        }
        for (int x = 0; x < width; ++x)
            img.at<cv::Vec3b>(y, x) = c;
    }

    const auto px = [&](double fx) { return static_cast<int>(fx * width); };
    const auto py = [&](double fy) { return static_cast<int>(fy * height); };

    // Sun
    cv::circle(img, {px(0.80), py(0.20)}, py(0.10), {80, 220, 255}, cv::FILLED, cv::LINE_AA);

    // House: body, roof, door, window
    cv::rectangle(img, {px(0.15), py(0.42)}, {px(0.38), horizon + py(0.05)}, {70, 80, 180},
                  cv::FILLED);
    const cv::Point roof[3] = {{px(0.12), py(0.43)}, {px(0.265), py(0.25)}, {px(0.41), py(0.43)}};
    cv::fillConvexPoly(img, roof, 3, {40, 40, 110}, cv::LINE_AA);
    cv::rectangle(img, {px(0.24), py(0.55)}, {px(0.29), horizon + py(0.05)}, {30, 60, 100},
                  cv::FILLED);
    cv::rectangle(img, {px(0.31), py(0.48)}, {px(0.36), py(0.56)}, {240, 230, 200}, cv::FILLED);

    // Tree: trunk + canopy
    cv::rectangle(img, {px(0.62), py(0.50)}, {px(0.65), horizon + py(0.04)}, {30, 70, 110},
                  cv::FILLED);
    cv::circle(img, {px(0.635), py(0.42)}, py(0.12), {40, 110, 40}, cv::FILLED, cv::LINE_AA);

    // Light noise so flat areas aren't perfectly zero-energy (more like a real photo).
    cv::Mat noise(img.size(), CV_8UC3);
    cv::randu(noise, 0, 6);
    img += noise;

    return img;
}
