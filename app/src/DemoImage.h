#pragma once

#include <opencv2/core.hpp>

// Procedurally drawn test scene (sky, ground, house, tree, sun) so the app always has an
// image to work with. Flat regions are cheap to carve; the objects should be preserved.
cv::Mat makeDemoImage(int width = 480, int height = 300);
