#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct AudioInfo {
    uint32_t sampleRate;
    uint16_t channels;
    uint16_t bitsPerSample;
    uint64_t totalFrames;
};

class Audio {
private:
    AudioInfo info_;
    std::vector<float> samples_;

public:
    explicit Audio(const std::string& filename);

    Audio(const AudioInfo& info, std::vector<float> samples);

    Audio() = default;
    ~Audio() = default;
    Audio(const Audio&) = default;
    Audio& operator=(const Audio&) = default;
    Audio(Audio&&) = default;
    Audio& operator=(Audio&&) = default;

    [[nodiscard]] const AudioInfo& getInfo() const noexcept { return info_; }
    [[nodiscard]] const std::vector<float>& getSamples() const noexcept { return samples_; }
    [[nodiscard]] std::vector<float>& getSamples() noexcept { return samples_; }

    [[nodiscard]] std::vector<bool> toBits() const;

    static Audio fromBits(const std::vector<bool>& bits, const AudioInfo& info);

    void save(const std::string& filename) const;
};
