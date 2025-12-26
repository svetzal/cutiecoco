// CutieCoCo Core Audio System
// Default implementation returns null audio (silent)

#include "cutie/audio.h"

namespace cutie {

std::unique_ptr<IAudioSystem> create_audio_system() {
    return std::make_unique<NullAudioSystem>();
}

} // namespace cutie
