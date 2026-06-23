#include <algorithm>
#include <cstring>
#include "../include/image.hpp"
#include "../third_party/stb_image.h"
#include "../third_party/stb_image_write.h"

Image::Image(const std::string& filename) {
    int width, height, channels;
    uint8_t* imgData = stbi_load(filename.c_str(), &width, &height, &channels, 0);

    if (!imgData) {
        throw std::runtime_error("Failed to load image: " + filename);
    }

    width_ = width;
    height_ = height;
    channels_ = channels;

    size_t dataSize = static_cast<size_t>(width * height * channels);
    data_.resize(dataSize);
    std::copy(imgData, imgData + dataSize, data_.begin());

    stbi_image_free(imgData);
}

Image::Image(int width, int height, int channels)
    : width_(width), height_(height), channels_(channels) {
    data_.resize(static_cast<size_t>(width * height * channels), 0);
}

Image::Image(Image&& other) noexcept
    : width_(std::exchange(other.width_, 0)), height_(std::exchange(other.height_, 0)), channels_(std::exchange(other.channels_, 0)), data_(std::move(other.data_)) {}

Image& Image::operator=(Image&& other) noexcept {
    if (this != &other) {
        width_ = std::exchange(other.width_, 0);
        height_ = std::exchange(other.height_, 0);
        channels_ = std::exchange(other.channels_, 0);
        data_ = std::move(other.data_);
    }
    return *this;
}

void Image::save(const std::string& filename) const {
    int result = stbi_write_png(
        filename.c_str(),
        width_,
        height_,
        channels_,
        data_.data(),
        width_ * channels_);

    if (!result) {
        throw std::runtime_error("Failed to save image: " + filename);
    }
}
