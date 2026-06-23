#pragma once
#include <cstdint>
#include <vector>
#include "algorithm.hpp"
#include "audio.hpp"

class PVDAlgorithm : public Algorithm {
private:
    struct Range {
        int lower;
        int upper;
        int bits;
    };
    static const std::vector<Range> RANGES;
    static constexpr size_t HEADER_SIZE_BITS = 32 + 16 + 16 + 64;

    void embedHeader(std::vector<bool>& bitStream, const AudioInfo& info);
    AudioInfo extractHeader(const std::vector<bool>& bitStream);

public:
    PVDAlgorithm() = default;

    void hide(const Image& coverImage, const Audio& secretAudio, Image& stegoImage) override;
    void extract(const Image& stegoImage, Audio& extractedAudio) override;

    [[nodiscard]] std::string getName() const override;
    [[nodiscard]] std::unique_ptr<Algorithm> clone() const override;
};
