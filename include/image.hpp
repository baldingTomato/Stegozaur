#pragma once
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

class Image {
private:
    int width_;
    int height_;
    int channels_;
    std::vector<uint8_t> data_;

public:
    explicit Image(const std::string& filename);

    Image(int width, int height, int channels);

    ~Image() = default;
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;
    Image(Image&& other) noexcept;
    Image& operator=(Image&& other) noexcept;

    [[nodiscard]] int getWidth() const noexcept { return width_; }
    [[nodiscard]] int getHeight() const noexcept { return height_; }
    [[nodiscard]] int getChannels() const noexcept { return channels_; }
    [[nodiscard]] size_t getDataSize() const noexcept { return data_.size(); }
    [[nodiscard]] const std::vector<uint8_t>& getData() const noexcept { return data_; }
    [[nodiscard]] std::vector<uint8_t>& getData() noexcept { return data_; }

    void save(const std::string& filename) const;

    [[nodiscard]] size_t getCapacityBits() const noexcept {
        return data_.size() * 8;  // 8 bitów na bajt
    }
};
