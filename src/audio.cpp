#include "../include/audio.hpp"
#include "../third_party/dr_wav.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

Audio::Audio(const std::string& filename) {
    drwav wav;
    if (!drwav_init_file(&wav, filename.c_str(), nullptr)) {
        throw std::runtime_error("Failed to load WAV file: " + filename);
    }

    info_.sampleRate = wav.sampleRate;
    info_.channels = static_cast<uint16_t>(wav.channels);
    info_.bitsPerSample = static_cast<uint16_t>(wav.bitsPerSample);
    info_.totalFrames = wav.totalPCMFrameCount;

    samples_.resize(static_cast<size_t>(info_.totalFrames * info_.channels));

    drwav_uint64 framesRead = drwav_read_pcm_frames_f32(
        &wav,
        info_.totalFrames,
        samples_.data());

    drwav_uninit(&wav);

    if (framesRead != info_.totalFrames) {
        throw std::runtime_error("Failed to read all audio samples");
    }
}

Audio::Audio(const AudioInfo& info, std::vector<float> samples)
    : info_(info), samples_(std::move(samples)) {
    if (samples_.size() != info_.totalFrames * info_.channels) {
        throw std::runtime_error("Sample count mismatch with audio info");
    }
}

std::vector<bool> Audio::toBits() const {
    int bitsPerSample = info_.bitsPerSample;
    std::vector<bool> bits;
    bits.reserve(samples_.size() * bitsPerSample);

    for (float sample : samples_) {
        // float na int o odpowiedniej liczbie bitów
        int maxVal = (1 << (bitsPerSample - 1)) - 1; // dla signed
        int16_t pcmSample = static_cast<int16_t>(sample * maxVal);

        for (int i = bitsPerSample - 1; i >= 0; --i) {
            bits.push_back((pcmSample >> i) & 1);
        }
    }
    return bits;
}

Audio Audio::fromBits(const std::vector<bool>& bits, const AudioInfo& info) {
    int bitsPerSample = info.bitsPerSample;
    size_t numSamples = bits.size() / bitsPerSample;

    if (numSamples != info.totalFrames * info.channels) {
        throw std::runtime_error("Bit count mismatch with audio info");
    }

    std::vector<float> samples(numSamples);

    for (size_t i = 0; i < numSamples; ++i) {
        int16_t pcmSample = 0;
        for (int j = 0; j < bitsPerSample; ++j) {
            size_t bitIndex = i * bitsPerSample + j;
            if (bits[bitIndex]) {
                pcmSample |= (1 << (bitsPerSample - 1 - j));
            }
        }
        int maxVal = (1 << (bitsPerSample - 1)) - 1;
        samples[i] = static_cast<float>(pcmSample) / maxVal;
    }

    return Audio(info, std::move(samples));
}

void Audio::save(const std::string& filename) const {
    drwav_data_format format;
    format.container = drwav_container_riff;
    format.format = DR_WAVE_FORMAT_PCM;
    format.channels = static_cast<drwav_uint16>(info_.channels);
    format.sampleRate = info_.sampleRate;
    format.bitsPerSample = info_.bitsPerSample;

    drwav wav;
    if (!drwav_init_file_write(&wav, filename.c_str(), &format, nullptr)) {
        throw std::runtime_error("Failed to create WAV file: " + filename);
    }

    //  float na int16
    std::vector<int16_t> pcmSamples(samples_.size());
    for (size_t i = 0; i < samples_.size(); ++i) {
        pcmSamples[i] = static_cast<int16_t>(samples_[i] * 32767.0f);
    }

    drwav_uint64 framesWritten = drwav_write_pcm_frames(
        &wav,
        info_.totalFrames,
        pcmSamples.data());

    drwav_uninit(&wav);

    if (framesWritten != info_.totalFrames) {
        throw std::runtime_error("Failed to write all audio samples");
    }
}
