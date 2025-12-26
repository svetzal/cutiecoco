// CutieCoCo - Windows Audio Output Implementation

#include "win32audio.h"

#include <cstring>

namespace cutie {
namespace win32 {

Win32Audio::Win32Audio() = default;

Win32Audio::~Win32Audio()
{
    shutdown();
}

bool Win32Audio::init(uint32_t sampleRate)
{
    m_sampleRate = sampleRate;

    // Set up wave format for 16-bit stereo
    WAVEFORMATEX wfx = {};
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 2;
    wfx.nSamplesPerSec = sampleRate;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    MMRESULT result = waveOutOpen(
        &m_waveOut,
        WAVE_MAPPER,
        &wfx,
        reinterpret_cast<DWORD_PTR>(waveOutCallback),
        reinterpret_cast<DWORD_PTR>(this),
        CALLBACK_FUNCTION
    );

    if (result != MMSYSERR_NOERROR) {
        return false;
    }

    // Initialize audio buffers
    for (int i = 0; i < NUM_BUFFERS; ++i) {
        m_buffers[i].data.resize(BUFFER_SIZE_SAMPLES * 2);  // Stereo
        m_buffers[i].header.lpData = reinterpret_cast<LPSTR>(m_buffers[i].data.data());
        m_buffers[i].header.dwBufferLength = static_cast<DWORD>(m_buffers[i].data.size() * sizeof(int16_t));
        m_buffers[i].header.dwUser = static_cast<DWORD_PTR>(i);
        m_buffers[i].inUse = false;

        result = waveOutPrepareHeader(m_waveOut, &m_buffers[i].header, sizeof(WAVEHDR));
        if (result != MMSYSERR_NOERROR) {
            shutdown();
            return false;
        }
    }

    m_initialized = true;
    return true;
}

void Win32Audio::shutdown()
{
    if (m_waveOut) {
        waveOutReset(m_waveOut);

        for (int i = 0; i < NUM_BUFFERS; ++i) {
            if (m_buffers[i].header.dwFlags & WHDR_PREPARED) {
                waveOutUnprepareHeader(m_waveOut, &m_buffers[i].header, sizeof(WAVEHDR));
            }
            m_buffers[i].data.clear();
            m_buffers[i].inUse = false;
        }

        waveOutClose(m_waveOut);
        m_waveOut = nullptr;
    }

    m_initialized = false;
    m_queuedSamples = 0;
}

void Win32Audio::submitSamples(const int16_t* samples, size_t count)
{
    if (!m_initialized || !samples || count == 0) return;

    // Find a free buffer
    AudioBuffer* buffer = nullptr;
    for (int i = 0; i < NUM_BUFFERS; ++i) {
        int idx = (m_currentBuffer + i) % NUM_BUFFERS;
        if (!m_buffers[idx].inUse) {
            buffer = &m_buffers[idx];
            m_currentBuffer = (idx + 1) % NUM_BUFFERS;
            break;
        }
    }

    if (!buffer) {
        // All buffers in use - audio may glitch
        return;
    }

    // Copy samples to buffer (convert mono to stereo if needed)
    size_t samplesToWrite = count > BUFFER_SIZE_SAMPLES ? BUFFER_SIZE_SAMPLES : count;
    size_t bytesToWrite = samplesToWrite * sizeof(int16_t) * 2;  // Stereo

    // Assuming input is mono, duplicate to stereo
    for (size_t i = 0; i < samplesToWrite; ++i) {
        buffer->data[i * 2] = samples[i];      // Left
        buffer->data[i * 2 + 1] = samples[i];  // Right
    }

    buffer->header.dwBufferLength = static_cast<DWORD>(bytesToWrite);
    buffer->inUse = true;

    MMRESULT result = waveOutWrite(m_waveOut, &buffer->header, sizeof(WAVEHDR));
    if (result == MMSYSERR_NOERROR) {
        m_queuedSamples += samplesToWrite;
    } else {
        buffer->inUse = false;
    }
}

size_t Win32Audio::getQueuedSampleCount() const
{
    return m_queuedSamples;
}

void CALLBACK Win32Audio::waveOutCallback(
    HWAVEOUT /*hwo*/,
    UINT msg,
    DWORD_PTR instance,
    DWORD_PTR param1,
    DWORD_PTR /*param2*/)
{
    if (msg == WOM_DONE) {
        auto* self = reinterpret_cast<Win32Audio*>(instance);
        auto* header = reinterpret_cast<WAVEHDR*>(param1);
        self->onBufferDone(header);
    }
}

void Win32Audio::onBufferDone(WAVEHDR* header)
{
    int bufferIndex = static_cast<int>(header->dwUser);
    if (bufferIndex >= 0 && bufferIndex < NUM_BUFFERS) {
        m_buffers[bufferIndex].inUse = false;

        size_t samplesInBuffer = header->dwBufferLength / (sizeof(int16_t) * 2);
        if (m_queuedSamples >= samplesInBuffer) {
            m_queuedSamples -= samplesInBuffer;
        } else {
            m_queuedSamples = 0;
        }
    }
}

}  // namespace win32
}  // namespace cutie
