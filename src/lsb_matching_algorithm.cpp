#include "../include/lsb_matching_algorithm.hpp"
#include <algorithm>
#include <bitset>
#include <cmath>
#include <stdexcept>
#include "../include/audio.hpp"
#include "../include/image.hpp"

size_t LSBMatchingAlgorithm::calculateRequiredBits(const Audio& audio) const {
    auto audioBits = audio.toBits();  // na podstawie info bitsPerSample
    return audioBits.size() + HEADER_SIZE_BITS;
}

size_t LSBMatchingAlgorithm::calculateAvailableBits(const Image& image) const {
    return image.getData().size() * bitsPerChannel_;
}

void LSBMatchingAlgorithm::embedHeader(std::vector<bool>& bitStream, const AudioInfo& info) {
    std::vector<bool> headerBits;
    headerBits.reserve(HEADER_SIZE_BITS);

    // sampleRate (32 bity)
    uint32_t sampleRate = info.sampleRate;
    for (int i = 31; i >= 0; --i) {
        headerBits.push_back((sampleRate >> i) & 1);
    }

    // channels (16 bitów)
    uint16_t channels = info.channels;
    for (int i = 15; i >= 0; --i) {
        headerBits.push_back((channels >> i) & 1);
    }

    // bitsPerSample (16 bitów)
    uint16_t bitsPerSample = info.bitsPerSample;
    for (int i = 15; i >= 0; --i) {
        headerBits.push_back((bitsPerSample >> i) & 1);
    }

    // totalFrames (64 bity)
    uint64_t totalFrames = info.totalFrames;
    for (int i = 63; i >= 0; --i) {
        headerBits.push_back((totalFrames >> i) & 1);
    }

    // Wstaw nagłówek na początek strumienia
    bitStream.insert(bitStream.begin(), headerBits.begin(), headerBits.end());
}

AudioInfo LSBMatchingAlgorithm::extractHeader(const std::vector<bool>& bitStream) {
    if (bitStream.size() < HEADER_SIZE_BITS) {
        throw std::runtime_error("Bit stream too short for header");
    }

    size_t pos = 0;
    AudioInfo info;

    // sampleRate
    uint32_t sampleRate = 0;
    for (int i = 31; i >= 0; --i, ++pos) {
        if (bitStream[pos])
            sampleRate |= (1u << i);
    }
    info.sampleRate = sampleRate;

    // channels
    uint16_t channels = 0;
    for (int i = 15; i >= 0; --i, ++pos) {
        if (bitStream[pos])
            channels |= static_cast<uint16_t>(1u << i);
    }
    info.channels = channels;

    // bitsPerSample
    uint16_t bitsPerSample = 0;
    for (int i = 15; i >= 0; --i, ++pos) {
        if (bitStream[pos])
            bitsPerSample |= static_cast<uint16_t>(1u << i);
    }
    info.bitsPerSample = bitsPerSample;

    // totalFrames
    uint64_t totalFrames = 0;
    for (int i = 63; i >= 0; --i, ++pos) {
        if (bitStream[pos])
            totalFrames |= (1ull << i);
    }
    info.totalFrames = totalFrames;

    return info;
}

LSBMatchingAlgorithm::LSBMatchingAlgorithm(uint8_t bitsPerChannel, unsigned int seed)
    : bitsPerChannel_(bitsPerChannel), rng_(seed) {
    if (bitsPerChannel_ < 1 || bitsPerChannel_ > 8) {
        throw std::invalid_argument("bitsPerChannel must be between 1 and 8");
    }
}

std::string LSBMatchingAlgorithm::getName() const {
    return "LSBm-" + std::to_string(bitsPerChannel_);
}

std::unique_ptr<Algorithm> LSBMatchingAlgorithm::clone() const {
    return std::make_unique<LSBMatchingAlgorithm>(*this);
}

void LSBMatchingAlgorithm::hide(const Image& coverImage, const Audio& secretAudio, Image& stegoImage) {
    // 1. Konwersja audio na bity
    auto audioBits = secretAudio.toBits();

    // 2. Dodanie nagłówka z pełnymi informacjami o audio
    embedHeader(audioBits, secretAudio.getInfo());

    // 3. Sprawdzenie pojemności
    size_t requiredBits = audioBits.size();
    size_t availableBits = calculateAvailableBits(coverImage);

    if (requiredBits > availableBits) {
        throw std::runtime_error(
            "Image too small: need " + std::to_string(requiredBits) +
            " bits, have " + std::to_string(availableBits));
    }

    // 4. Skopiowanie danych obrazu do wynikowego
    auto& coverData = coverImage.getData();
    auto& stegoData = stegoImage.getData();
    stegoData = coverData;

    // 5. Osadzanie bitów metodą LSB matching (+-1)
    size_t bitIndex = 0;
    // Dla każdego bajtu obrazu
    for (size_t i = 0; i < stegoData.size() && bitIndex < audioBits.size(); ++i) {
        uint8_t& pixel = stegoData[i];

        // Dla każdego bitu w tym bajcie, który można wykorzystać
        for (int b = 0; b < bitsPerChannel_ && bitIndex < audioBits.size(); ++b) {
            bool targetBit = audioBits[bitIndex];
            bool currentBit = (pixel >> b) & 1;

            if (currentBit != targetBit) {
                // Potrzebna zmiana – losowo +- 1
                int direction = (rng_() % 2 == 0) ? 1 : -1;
                int newValue = static_cast<int>(pixel) + direction;

                // Zabezpieczenie przed wyjściem poza zakres [0, 255]
                if (newValue < 0)
                    newValue = 1;  // nie używamy 0, bo -1 da 0
                if (newValue > 255)
                    newValue = 254;

                pixel = static_cast<uint8_t>(newValue);
            }
            ++bitIndex;
        }
    }
}

void LSBMatchingAlgorithm::extract(const Image& stegoImage, Audio& extractedAudio) {
    const auto& stegoData = stegoImage.getData();

    // 1. Odczytaj bity z obrazu (LSB)
    std::vector<bool> extractedBits;
    extractedBits.reserve(stegoData.size() * bitsPerChannel_);

    for (uint8_t pixel : stegoData) {
        for (int b = 0; b < bitsPerChannel_; ++b) {
            extractedBits.push_back((pixel >> b) & 1);
        }
    }

    // 2. Wyodrębnij nagłówek
    AudioInfo info = extractHeader(extractedBits);

    // 3. Oblicz rozmiar audio w bitach na podstawie informacji z nagłówka
    size_t audioSizeBits = static_cast<size_t>(info.totalFrames * info.channels * info.bitsPerSample);

    if (extractedBits.size() < HEADER_SIZE_BITS + audioSizeBits) {
        throw std::runtime_error("Extracted bits too short for declared audio size");
    }

    // 4. Pobierz bity audio (pomijając nagłówek)
    auto audioStart = std::next(extractedBits.begin(),
                                static_cast<std::ptrdiff_t>(HEADER_SIZE_BITS));
    auto audioEnd = std::next(audioStart,
                              static_cast<std::ptrdiff_t>(audioSizeBits));
    std::vector<bool> audioBits(audioStart, audioEnd);

    // 5. Odtwórz audio z bitów
    extractedAudio = Audio::fromBits(audioBits, info);
}