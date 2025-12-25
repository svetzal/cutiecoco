#ifndef DREAM_FRAMEBUFFER_H
#define DREAM_FRAMEBUFFER_H
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

#include <cstdint>
#include <cstring>
#include <vector>

namespace dream {

/**
 * @brief Abstract interface for GIME video output
 *
 * This interface abstracts the frame buffer that the GIME writes to,
 * allowing different rendering backends (Qt, SDL, null) to provide
 * their own implementations.
 *
 * The GIME writes pixels in RGBA format (32-bit per pixel).
 * The pitch is in pixels, not bytes.
 */
struct IFrameBuffer {
    virtual ~IFrameBuffer() = default;

    /**
     * @brief Get pointer to 32-bit RGBA pixel buffer
     * @return Pointer to first pixel (top-left corner)
     */
    virtual uint32_t* pixels() = 0;

    /**
     * @brief Get const pointer to pixel buffer
     */
    virtual const uint32_t* pixels() const = 0;

    /**
     * @brief Get frame buffer width in pixels
     */
    virtual int width() const = 0;

    /**
     * @brief Get frame buffer height in pixels
     */
    virtual int height() const = 0;

    /**
     * @brief Get row pitch in pixels (distance between rows)
     *
     * For a packed buffer this equals width(). May be larger if
     * there is row padding.
     */
    virtual int pitch() const = 0;

    /**
     * @brief Get raw byte pointer for callback compatibility
     */
    const uint8_t* data() const {
        return reinterpret_cast<const uint8_t*>(pixels());
    }

    /**
     * @brief Get total size in bytes
     */
    size_t sizeBytes() const {
        return static_cast<size_t>(pitch()) * height() * sizeof(uint32_t);
    }

    /**
     * @brief Clear buffer to a solid color
     * @param rgba 32-bit RGBA color value
     */
    virtual void clear(uint32_t rgba = 0xFF000000) {
        uint32_t* p = pixels();
        size_t count = static_cast<size_t>(pitch()) * height();
        for (size_t i = 0; i < count; ++i) {
            p[i] = rgba;
        }
    }
};

/**
 * @brief Standard frame buffer implementation
 *
 * Owns a contiguous pixel buffer. Used by EmulationThread for
 * the main display output.
 */
class FrameBuffer : public IFrameBuffer {
public:
    /**
     * @brief Create a frame buffer with specified dimensions
     * @param width Width in pixels
     * @param height Height in pixels
     */
    FrameBuffer(int width, int height)
        : m_width(width)
        , m_height(height)
        , m_buffer(static_cast<size_t>(width) * height, 0xFF000000)
    {
    }

    uint32_t* pixels() override { return m_buffer.data(); }
    const uint32_t* pixels() const override { return m_buffer.data(); }
    int width() const override { return m_width; }
    int height() const override { return m_height; }
    int pitch() const override { return m_width; }  // Packed, no padding

private:
    int m_width;
    int m_height;
    std::vector<uint32_t> m_buffer;
};

/**
 * @brief Null frame buffer for headless operation
 *
 * Provides a minimal buffer for testing/headless mode.
 * Discards all pixel writes efficiently.
 */
class NullFrameBuffer : public IFrameBuffer {
public:
    NullFrameBuffer() : m_pixel(0) {}

    uint32_t* pixels() override { return &m_pixel; }
    const uint32_t* pixels() const override { return &m_pixel; }
    int width() const override { return 1; }
    int height() const override { return 1; }
    int pitch() const override { return 1; }
    void clear(uint32_t) override {}

private:
    uint32_t m_pixel;
};

} // namespace dream

#endif // DREAM_FRAMEBUFFER_H
