#ifndef DREAM_AUDIO_H
#define DREAM_AUDIO_H

#include <cstdint>
#include <memory>

namespace dream {

// Abstract audio system interface
class IAudioSystem {
public:
    virtual ~IAudioSystem() = default;

    // Initialize audio with given sample rate
    virtual bool initialize(unsigned int sample_rate) = 0;

    // Shutdown audio system
    virtual void shutdown() = 0;

    // Submit audio samples (interleaved stereo, 16-bit signed)
    virtual void submit_samples(const int16_t* samples, size_t sample_count) = 0;

    // Get current sample rate
    virtual unsigned int sample_rate() const = 0;

    // Check if audio is available
    virtual bool is_available() const = 0;
};

// Null audio implementation - discards all samples silently
class NullAudioSystem : public IAudioSystem {
public:
    NullAudioSystem() = default;
    ~NullAudioSystem() override = default;

    bool initialize(unsigned int sample_rate) override {
        m_sample_rate = sample_rate;
        m_initialized = true;
        return true;
    }

    void shutdown() override {
        m_initialized = false;
    }

    void submit_samples(const int16_t* /*samples*/, size_t /*sample_count*/) override {
        // Discard samples silently
    }

    unsigned int sample_rate() const override {
        return m_sample_rate;
    }

    bool is_available() const override {
        return m_initialized;
    }

private:
    unsigned int m_sample_rate = 0;
    bool m_initialized = false;
};

// Factory function to create audio system
// Returns NullAudioSystem by default; Qt implementation will override
std::unique_ptr<IAudioSystem> create_audio_system();

} // namespace dream

#endif // DREAM_AUDIO_H
