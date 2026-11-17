#include "Texture.h"

#include <GLFW/glfw3.h> // pulls in <GL/gl.h> with the right Windows defines

#include <opencv2/imgproc.hpp>

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F // OpenGL 1.2; Windows' gl.h only ships 1.1
#endif

Texture::~Texture() {
    if (id_ != 0)
        glDeleteTextures(1, &id_);
}

void Texture::upload(const cv::Mat& img) {
    cv::Mat rgba;
    if (img.channels() == 1)
        cv::cvtColor(img, rgba, cv::COLOR_GRAY2RGBA);
    else
        cv::cvtColor(img, rgba, cv::COLOR_BGR2RGBA);
    if (!rgba.isContinuous())
        rgba = rgba.clone();

    if (id_ == 0)
        glGenTextures(1, &id_);

    glBindTexture(GL_TEXTURE_2D, id_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // keep seams crisp
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba.cols, rgba.rows, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 rgba.data);

    width_  = rgba.cols;
    height_ = rgba.rows;
}
