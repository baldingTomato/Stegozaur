#pragma once
#include <memory>
#include <string>
#include <vector>

class Image;
class Audio;

class Algorithm {
public:
    virtual ~Algorithm() = default;

    virtual void hide(const Image& coverImage, const Audio& secretAudio, Image& stegoImage) = 0;
    virtual void extract(const Image& stegoImage, Audio& extractedAudio) = 0;

    [[nodiscard]] virtual std::string getName() const = 0;
    [[nodiscard]] virtual std::unique_ptr<Algorithm> clone() const = 0;
};
