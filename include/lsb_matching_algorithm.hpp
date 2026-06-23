#pragma once
#include <cstdint>
#include <random>
#include "algorithm.hpp"
#include "audio.hpp"

class LSBMatchingAlgorithm : public Algorithm {
private:
    uint8_t bitsPerChannel_;
    mutable std::mt19937 rng_;
    static constexpr size_t HEADER_SIZE_BITS = 32 + 16 + 16 + 64;

    [[nodiscard]] size_t calculateRequiredBits(const Audio& audio) const;
    [[nodiscard]] size_t calculateAvailableBits(const Image& image) const;
    void embedHeader(std::vector<bool>& bitStream, const AudioInfo& info);
    [[nodiscard]] AudioInfo extractHeader(const std::vector<bool>& bitStream);

public:
    explicit LSBMatchingAlgorithm(uint8_t bitsPerChannel = 1, unsigned int seed = std::random_device{}());

    void hide(const Image& coverImage, const Audio& secretAudio, Image& stegoImage) override;
    void extract(const Image& stegoImage, Audio& extractedAudio) override;

    [[nodiscard]] std::string getName() const override;
    [[nodiscard]] std::unique_ptr<Algorithm> clone() const override;
};
