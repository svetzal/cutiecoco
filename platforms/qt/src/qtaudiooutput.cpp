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

#include "qtaudiooutput.h"

#include <QMediaDevices>
#include <QAudioDevice>
#include <QDebug>

QtAudioOutput::QtAudioOutput() = default;

QtAudioOutput::~QtAudioOutput()
{
    shutdown();
}

bool QtAudioOutput::init(uint32_t sampleRate)
{
    if (m_initialized) {
        return true;
    }

    if (sampleRate == 0) {
        return false;
    }

    m_sampleRate = sampleRate;

    // Configure audio format: 16-bit signed mono
    m_format.setSampleRate(static_cast<int>(sampleRate));
    m_format.setChannelCount(1);
    m_format.setSampleFormat(QAudioFormat::Int16);

    // Get default audio output device
    QAudioDevice audioDevice = QMediaDevices::defaultAudioOutput();
    if (audioDevice.isNull()) {
        qWarning() << "QtAudioOutput: No audio output device available";
        return false;
    }

    // Check if format is supported
    if (!audioDevice.isFormatSupported(m_format)) {
        qWarning() << "QtAudioOutput: Audio format not supported, trying nearest format";
        // Qt will convert to the nearest supported format
    }

    // Create audio sink
    m_audioSink = std::make_unique<QAudioSink>(audioDevice, m_format);

    // Configure buffer size - larger buffer reduces underruns at cost of latency
    // Use approximately 200ms worth of audio for smooth playback
    int bufferSize = static_cast<int>(sampleRate / 5 * sizeof(int16_t));  // 200ms
    m_audioSink->setBufferSize(bufferSize);

    // Start the audio sink and get the IO device for writing
    m_audioDevice = m_audioSink->start();
    if (!m_audioDevice) {
        qWarning() << "QtAudioOutput: Failed to start audio sink";
        m_audioSink.reset();
        return false;
    }

    // Pre-fill half the audio buffer with silence to prevent startup underrun
    // but leave room for incoming samples
    size_t silenceSamples = (bufferSize / sizeof(int16_t)) / 2;
    std::vector<int16_t> silence(silenceSamples, 0);
    m_audioDevice->write(reinterpret_cast<const char*>(silence.data()),
                         static_cast<qint64>(silence.size() * sizeof(int16_t)));

    // Set up fade-in: ~100ms to avoid startup crackle
    m_fadeInSamples = sampleRate / 10;  // 100ms worth of samples
    m_samplesProcessed = 0;

    m_initialized = true;
    qDebug() << "QtAudioOutput: Initialized at" << sampleRate << "Hz, buffer size:" << bufferSize;
    return true;
}

void QtAudioOutput::shutdown()
{
    if (!m_initialized) {
        return;
    }

    if (m_audioSink) {
        m_audioSink->stop();
        m_audioSink.reset();
    }

    m_audioDevice = nullptr;
    m_initialized = false;
}

void QtAudioOutput::submitSamples(const int16_t* samples, size_t count)
{
    if (!m_initialized || !m_audioDevice || samples == nullptr || count == 0) {
        return;
    }

    const int16_t* outputSamples = samples;

    // Apply fade-in during startup to avoid crackle
    if (m_samplesProcessed < m_fadeInSamples) {
        m_fadeBuffer.resize(count);

        for (size_t i = 0; i < count; ++i) {
            uint32_t sampleIndex = m_samplesProcessed + static_cast<uint32_t>(i);
            if (sampleIndex < m_fadeInSamples) {
                // Linear fade from 0 to 1
                float fadeLevel = static_cast<float>(sampleIndex) / static_cast<float>(m_fadeInSamples);
                m_fadeBuffer[i] = static_cast<int16_t>(samples[i] * fadeLevel);
            } else {
                m_fadeBuffer[i] = samples[i];
            }
        }

        m_samplesProcessed += static_cast<uint32_t>(count);
        outputSamples = m_fadeBuffer.data();
    }

    // Write samples directly - no resampling
    // Throttling is handled by the caller via getBufferFillLevel()
    const char* data = reinterpret_cast<const char*>(outputSamples);
    qint64 bytesToWrite = static_cast<qint64>(count * sizeof(int16_t));
    m_audioDevice->write(data, bytesToWrite);
}

size_t QtAudioOutput::getQueuedSampleCount() const
{
    if (!m_initialized || !m_audioSink) {
        return 0;
    }

    // Get bytes that can still fit in the buffer
    qsizetype bytesFree = m_audioSink->bytesFree();
    return static_cast<size_t>(bytesFree / sizeof(int16_t));
}

float QtAudioOutput::getBufferFillLevel() const
{
    if (!m_initialized || !m_audioSink) {
        return 0.0f;
    }

    qsizetype bytesFree = m_audioSink->bytesFree();
    qsizetype bufSize = m_audioSink->bufferSize();
    return static_cast<float>(bufSize - bytesFree) / static_cast<float>(bufSize);
}

uint32_t QtAudioOutput::getSampleRate() const
{
    return m_sampleRate;
}
