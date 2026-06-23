#include "../include/lsb_algorithm.hpp"
#include <algorithm>
#include <bitset>
#include <stdexcept>
#include "../include/audio.hpp"
#include "../include/image.hpp"

LSBAlgorithm::LSBAlgorithm(uint8_t bitsPerChannel)
    : bitsPerChannel_(bitsPerChannel) {
    if (bitsPerChannel_ < 1 || bitsPerChannel_ > 8) {
        throw std::invalid_argument("bitsPerChannel must be between 1 and 8");
    }
}

std::string LSBAlgorithm::getName() const {
    return "LSB-" + std::to_string(bitsPerChannel_);
}

std::unique_ptr<Algorithm> LSBAlgorithm::clone() const {
    return std::make_unique<LSBAlgorithm>(*this);
}

size_t LSBAlgorithm::calculateRequiredBits(const Audio& audio) const {
    auto audioBits = audio.toBits();
    return audioBits.size() + HEADER_SIZE_BITS;
}

size_t LSBAlgorithm::calculateAvailableBits(const Image& image) const {
    return image.getData().size() * bitsPerChannel_;
}

void LSBAlgorithm::embedHeader(std::vector<bool>& bitStream, const AudioInfo& info) {
    // Nagłówek: sampleRate (32 bity), channels (16), bitsPerSample (16), totalFrames (64)
    std::vector<bool> headerBits;
    headerBits.reserve(HEADER_SIZE_BITS);

    uint32_t sampleRate = info.sampleRate;
    for (int i = 31; i >= 0; --i) {
        headerBits.push_back((sampleRate >> i) & 1);
    }

    uint16_t channels = info.channels;
    for (int i = 15; i >= 0; --i) {
        headerBits.push_back((channels >> i) & 1);
    }

    uint16_t bitsPerSample = info.bitsPerSample;
    for (int i = 15; i >= 0; --i) {
        headerBits.push_back((bitsPerSample >> i) & 1);
    }

    uint64_t totalFrames = info.totalFrames;
    for (int i = 63; i >= 0; --i) {
        headerBits.push_back((totalFrames >> i) & 1);
    }

    // Nagłówek na początek strumienia
    bitStream.insert(bitStream.begin(), headerBits.begin(), headerBits.end());
}

AudioInfo LSBAlgorithm::extractHeader(const std::vector<bool>& bitStream) {
    if (bitStream.size() < HEADER_SIZE_BITS) {
        throw std::runtime_error("Bit stream too short for header");
    }

    size_t pos = 0;

    // sampleRate (32 bity)
    uint32_t sampleRate = 0;
    for (int i = 31; i >= 0; --i, ++pos) {
        if (bitStream[pos]) {
            sampleRate |= (1u << i);
        }
    }

    // channels (16 bitów)
    uint16_t channels = 0;
    for (int i = 15; i >= 0; --i, ++pos) {
        if (bitStream[pos]) {
            channels |= (1u << i);
        }
    }

    // bitsPerSample (16 bitów)
    uint16_t bitsPerSample = 0;
    for (int i = 15; i >= 0; --i, ++pos) {
        if (bitStream[pos]) {
            bitsPerSample |= (1u << i);
        }
    }

    // totalFrames (64 bity)
    uint64_t totalFrames = 0;
    for (int i = 63; i >= 0; --i, ++pos) {
        if (bitStream[pos]) {
            totalFrames |= (1ull << i);
        }
    }

    AudioInfo info;
    info.sampleRate = sampleRate;
    info.channels = channels;
    info.bitsPerSample = bitsPerSample;
    info.totalFrames = totalFrames;
    return info;
}

void LSBAlgorithm::hide(const Image& coverImage, const Audio& secretAudio, Image& stegoImage) {
    // 1. Konwersja audio na bity
    auto audioBits = secretAudio.toBits();

    // 2. Dodanie nagłówka z informacjami o audio
    embedHeader(audioBits, secretAudio.getInfo());

    // 3. Sprawdzenie pojemności
    size_t requiredBits = audioBits.size();
    size_t availableBits = calculateAvailableBits(coverImage);

    if (requiredBits > availableBits) {
        throw std::runtime_error("Image too small...");
    }

    // 4. Tworzenie obrazu wynikowego
    auto& coverData = coverImage.getData();
    auto& stegoData = stegoImage.getData();
    stegoData = coverData;  // kopiowanie danych

    // 5. Osadzanie bitów
    size_t bitIndex = 0;
    for (size_t i = 0; i < stegoData.size() && bitIndex < audioBits.size(); ++i) {
        for (int b = 0; b < bitsPerChannel_ && bitIndex < audioBits.size(); ++b) {
            uint8_t& pixel = stegoData[i];

            // Maska bitowa jako uint8_t – 1u, aby uniknąć konwersji z int
            uint8_t mask = static_cast<uint8_t>(1u << b);

            // Wyczyść bit na pozycji b
            pixel &= static_cast<uint8_t>(~mask);

            // Wstaw nowy bit
            if (audioBits[bitIndex]) {
                pixel |= mask;
            }

            ++bitIndex;
        }
    }
}

void LSBAlgorithm::extract(const Image& stegoImage, Audio& extractedAudio) {
    const auto& stegoData = stegoImage.getData();

    // 1. Odczytaj bity z obrazu
    std::vector<bool> extractedBits;
    extractedBits.reserve(stegoData.size() * bitsPerChannel_);

    for (uint8_t pixel : stegoData) {
        for (int b = 0; b < bitsPerChannel_; ++b) {
            extractedBits.push_back((pixel >> b) & 1);
        }
    }

    // 2. Odczytaj nagłówek
    AudioInfo info = extractHeader(extractedBits);

    // Rozmiar audio w bitach na podstawie informacji z nagłówka
    size_t audioSizeBits = static_cast<size_t>(info.totalFrames * info.channels * info.bitsPerSample);

    // 3. Sprawdź, czy mamy wystarczająco dużo bitów
    if (extractedBits.size() < HEADER_SIZE_BITS + audioSizeBits) {
        throw std::runtime_error("Extracted bits too short for declared audio size");
    }

    // 4. Pobierz tylko bity audio pomijając nagłówek
    auto audioStart = std::next(extractedBits.begin(), 
                                static_cast<std::ptrdiff_t>(HEADER_SIZE_BITS));
    auto audioEnd = std::next(audioStart, 
                              static_cast<std::ptrdiff_t>(audioSizeBits));
    std::vector<bool> audioBits(audioStart, audioEnd);

    // 5. Odtwórz audio z bitów, używając odczytanego info
    extractedAudio = Audio::fromBits(audioBits, info);
}
