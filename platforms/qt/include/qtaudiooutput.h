#ifndef QTAUDIOOUTPUT_H
#define QTAUDIOOUTPUT_H
/*
Copyright 2024-2025 CutieCoCo Contributors
This file is part of CutieCoCo.

    CutieCoCo is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    CutieCoCo is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CutieCoCo.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "cutie/interfaces.h"

#include <QAudioSink>
#include <QAudioFormat>
#include <QIODevice>
#include <memory>
#include <vector>

/**
 * @brief Qt implementation of IAudioOutput
 *
 * Uses QAudioSink (Qt6) to play audio samples through the system audio.
 * Buffers samples and manages the audio stream lifecycle.
 */
class QtAudioOutput : public cutie::IAudioOutput {
public:
    QtAudioOutput();
    ~QtAudioOutput() override;

    // IAudioOutput interface
    bool init(uint32_t sampleRate) override;
    void shutdown() override;
    void submitSamples(const int16_t* samples, size_t count) override;
    size_t getQueuedSampleCount() const override;
    uint32_t getSampleRate() const override;

private:
    std::unique_ptr<QAudioSink> m_audioSink;
    QIODevice* m_audioDevice = nullptr;  // Owned by QAudioSink
    QAudioFormat m_format;
    uint32_t m_sampleRate = 0;
    bool m_initialized = false;

    // Fade-in to avoid startup crackle
    uint32_t m_fadeInSamples = 0;      // Total samples for fade-in period
    uint32_t m_samplesProcessed = 0;   // Samples processed so far
    std::vector<int16_t> m_fadeBuffer; // Temp buffer for fade processing
};

#endif // QTAUDIOOUTPUT_H
