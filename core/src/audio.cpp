// DREAM Core Audio System
// Default implementation returns null audio (silent)

#include "dream/audio.h"

namespace dream {

std::unique_ptr<IAudioSystem> create_audio_system() {
    return std::make_unique<NullAudioSystem>();
}

} // namespace dream
