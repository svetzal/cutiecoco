#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <cstdint>
#include <vector>

namespace cutie {
namespace win32 {

// GDI-based renderer for CutieCoCo
// TODO: Add DirectX 11 renderer option (DREAM-VCC-6ga)
class Win32Renderer {
public:
    Win32Renderer();
    ~Win32Renderer();

    // Initialize with window handle
    bool init(HWND hwnd);
    void shutdown();

    // Rendering
    void beginFrame();
    void present(const uint8_t* pixels, int srcWidth, int srcHeight);
    void endFrame();

    // Resize handling
    void resize(int width, int height);

private:
    void createBackBuffer(int width, int height);
    void destroyBackBuffer();

private:
    HWND m_hwnd = nullptr;
    HDC m_hdc = nullptr;
    HDC m_memDC = nullptr;
    HBITMAP m_bitmap = nullptr;
    HBITMAP m_oldBitmap = nullptr;
    void* m_bitmapBits = nullptr;

    BITMAPINFO m_bmi = {};

    int m_width = 0;
    int m_height = 0;

    // Source frame dimensions (from emulator)
    int m_srcWidth = 640;
    int m_srcHeight = 480;
};

}  // namespace win32
}  // namespace cutie
