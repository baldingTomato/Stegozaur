#include "../include/pvd_algorithm.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include "../include/audio.hpp"
#include "../include/image.hpp"

const std::vector<PVDAlgorithm::Range> PVDAlgorithm::RANGES = {
    {0, 7, 3},
    {8, 15, 3},
    {16, 31, 4},
    {32, 63, 5},
    {64, 127, 6},
    {128, 255, 7}};

void PVDAlgorithm::embedHeader(std::vector<bool>& bitStream, const AudioInfo& info) {
    std::vector<bool> headerBits;
    headerBits.reserve(HEADER_SIZE_BITS);

    // 32 bity
    uint32_t sampleRate = info.sampleRate;
    for (int i = 31; i >= 0; --i)
        headerBits.push_back((sampleRate >> i) & 1);

    // 16 bitów
    uint16_t channels = info.channels;
    for (int i = 15; i >= 0; --i)
        headerBits.push_back((channels >> i) & 1);

    // 16 bitów
    uint16_t bitsPerSample = info.bitsPerSample;
    for (int i = 15; i >= 0; --i)
        headerBits.push_back((bitsPerSample >> i) & 1);

    // 64 bity
    uint64_t totalFrames = info.totalFrames;
    for (int i = 63; i >= 0; --i)
        headerBits.push_back((totalFrames >> i) & 1);

    bitStream.insert(bitStream.begin(), headerBits.begin(), headerBits.end());
}

AudioInfo PVDAlgorithm::extractHeader(const std::vector<bool>& bitStream) {
    if (bitStream.size() < HEADER_SIZE_BITS)
        throw std::runtime_error("Bit stream too short for header");

    size_t pos = 0;

    uint32_t sampleRate = 0;
    for (int i = 31; i >= 0; --i, ++pos)
        if (bitStream[pos])
            sampleRate |= (1u << i);

    uint16_t channels = 0;
    for (int i = 15; i >= 0; --i, ++pos)
        if (bitStream[pos])
            channels |= static_cast<uint16_t>(1u << i);

    uint16_t bitsPerSample = 0;
    for (int i = 15; i >= 0; --i, ++pos)
        if (bitStream[pos])
            bitsPerSample |= static_cast<uint16_t>(1u << i);

    uint64_t totalFrames = 0;
    for (int i = 63; i >= 0; --i, ++pos)
        if (bitStream[pos])
            totalFrames |= (1ull << i);

    AudioInfo info;
    info.sampleRate = sampleRate;
    info.channels = channels;
    info.bitsPerSample = bitsPerSample;
    info.totalFrames = totalFrames;
    return info;
}

void PVDAlgorithm::hide(const Image& coverImage, const Audio& secretAudio, Image& stegoImage) {
    auto audioBits = secretAudio.toBits();
    embedHeader(audioBits, secretAudio.getInfo());

    const auto& coverData = coverImage.getData();
    auto& stegoData = stegoImage.getData();
    stegoData = coverData;

    int width = coverImage.getWidth();
    int height = coverImage.getHeight();
    int channels = coverImage.getChannels();

    size_t bitIndex = 0;
    size_t totalBits = audioBits.size();

    for (int y = 0; y < height && bitIndex < totalBits; ++y) {
        for (int c = 0; c < channels && bitIndex < totalBits; ++c) {
            for (int x = 0; x + 1 < width && bitIndex < totalBits; x += 2) {
                size_t idx1 = static_cast<size_t>((y * width + x) * channels + c);
                size_t idx2 = static_cast<size_t>((y * width + x + 1) * channels + c);

                uint8_t p1_orig = stegoData[idx1];
                uint8_t p2_orig = stegoData[idx2];

                int d = static_cast<int>(p1_orig) - static_cast<int>(p2_orig);
                bool p1_greater = (d >= 0);
                int abs_d = std::abs(d);

                // Znajdź przedział
                const Range* range = nullptr;
                for (const auto& r : RANGES) {
                    if (abs_d >= r.lower && abs_d <= r.upper) {
                        range = &r;
                        break;
                    }
                }
                if (!range) continue; // nie powinno się zdarzyć

                int k = range->bits;
                if (bitIndex + k > totalBits) {
                    // nie ma już bitów – przerywamy
                    bitIndex = totalBits;
                    break;
                }

                // Odczytaj k bitów (MSB first)
                uint32_t val = 0;
                for (int b = 0; b < k; ++b) {
                    if (audioBits[bitIndex + b]) {
                        val |= (1u << (k - 1 - b));
                    }
                }

                int new_d = range->lower + static_cast<int>(val);

                // Ustal nowe wartości pikseli tak, aby różnica = new_d
                uint8_t p1_new, p2_new;
                if (p1_greater) {
                    // Chcemy: p1_new - p2_new = new_d
                    // Możemy zachować jeden piksel bez zmian, a drugi dostosować
                    int p1_candidate = static_cast<int>(p2_orig) + new_d;
                    if (p1_candidate >= 0 && p1_candidate <= 255) {
                        p1_new = static_cast<uint8_t>(p1_candidate);
                        p2_new = p2_orig;
                    } else {
                        // Jeśli nie, zachowujemy p1_orig i modyfikujemy p2
                        int p2_candidate = static_cast<int>(p1_orig) - new_d;
                        if (p2_candidate >= 0 && p2_candidate <= 255) {
                            p1_new = p1_orig;
                            p2_new = static_cast<uint8_t>(p2_candidate);
                        } else {
                            // Para nie nadaje się – pomijamy (nie zużywamy bitów)
                            continue;
                        }
                    }
                } else {
                    // p2_new - p1_new = new_d
                    int p2_candidate = static_cast<int>(p1_orig) + new_d;
                    if (p2_candidate >= 0 && p2_candidate <= 255) {
                        p1_new = p1_orig;
                        p2_new = static_cast<uint8_t>(p2_candidate);
                    } else {
                        int p1_candidate = static_cast<int>(p2_orig) - new_d;
                        if (p1_candidate >= 0 && p1_candidate <= 255) {
                            p1_new = static_cast<uint8_t>(p1_candidate);
                            p2_new = p2_orig;
                        } else {
                            continue;
                        }
                    }
                }

                // Zastosuj zmiany
                stegoData[idx1] = p1_new;
                stegoData[idx2] = p2_new;
                bitIndex += k;
            }
        }
    }

    if (bitIndex < totalBits) {
        throw std::runtime_error("Not enough pixel pairs to hide the audio");
    }
}

void PVDAlgorithm::extract(const Image& stegoImage, Audio& extractedAudio) {
    const auto& stegoData = stegoImage.getData();
    int width = stegoImage.getWidth();
    int height = stegoImage.getHeight();
    int channels = stegoImage.getChannels();

    std::vector<bool> extractedBits;

    for (int y = 0; y < height; ++y) {
        for (int c = 0; c < channels; ++c) {
            for (int x = 0; x + 1 < width; x += 2) {
                size_t idx1 = static_cast<size_t>((y * width + x) * channels + c);
                size_t idx2 = static_cast<size_t>((y * width + x + 1) * channels + c);

                uint8_t p1 = stegoData[idx1];
                uint8_t p2 = stegoData[idx2];

                int d = static_cast<int>(p1) - static_cast<int>(p2);
                int abs_d = std::abs(d);

                const Range* range = nullptr;
                for (const auto& r : RANGES) {
                    if (abs_d >= r.lower && abs_d <= r.upper) {
                        range = &r;
                        break;
                    }
                }
                if (!range)
                    continue;

                int val = abs_d - range->lower;
                int k = range->bits;

                // zapisz bity (MSB first)
                for (int b = k - 1; b >= 0; --b) {
                    extractedBits.push_back((val >> b) & 1);
                }
            }
        }
    }

    if (extractedBits.size() < HEADER_SIZE_BITS) {
        throw std::runtime_error("Not enough bits extracted for header");
    }

    AudioInfo info = extractHeader(extractedBits);

    size_t audioSizeBits = static_cast<size_t>(info.totalFrames * info.channels * info.bitsPerSample);
    if (extractedBits.size() < HEADER_SIZE_BITS + audioSizeBits) {
        throw std::runtime_error("Extracted bits too short for declared audio size");
    }

    std::vector<bool> audioBits(
        extractedBits.begin() + static_cast<std::ptrdiff_t>(HEADER_SIZE_BITS),
        extractedBits.begin() + static_cast<std::ptrdiff_t>(HEADER_SIZE_BITS + audioSizeBits));

    extractedAudio = Audio::fromBits(audioBits, info);
}

std::string PVDAlgorithm::getName() const {
    return "PVD";
}

std::unique_ptr<Algorithm> PVDAlgorithm::clone() const {
    return std::make_unique<PVDAlgorithm>(*this);
}
