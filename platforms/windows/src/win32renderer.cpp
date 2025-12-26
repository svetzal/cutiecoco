// CutieCoCo - Windows GDI Renderer Implementation

#include "win32renderer.h"

#include <cstring>

namespace cutie {
namespace win32 {

Win32Renderer::Win32Renderer() = default;

Win32Renderer::~Win32Renderer()
{
    shutdown();
}

bool Win32Renderer::init(HWND hwnd)
{
    m_hwnd = hwnd;
    m_hdc = GetDC(hwnd);
    if (!m_hdc) {
        return false;
    }

    // Get initial window size
    RECT rect;
    GetClientRect(hwnd, &rect);
    m_width = rect.right - rect.left;
    m_height = rect.bottom - rect.top;

    // Initialize BITMAPINFO for DIB
    m_bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    m_bmi.bmiHeader.biPlanes = 1;
    m_bmi.bmiHeader.biBitCount = 32;
    m_bmi.bmiHeader.biCompression = BI_RGB;

    createBackBuffer(m_width, m_height);

    return true;
}

void Win32Renderer::shutdown()
{
    destroyBackBuffer();

    if (m_hdc && m_hwnd) {
        ReleaseDC(m_hwnd, m_hdc);
        m_hdc = nullptr;
    }
    m_hwnd = nullptr;
}

void Win32Renderer::createBackBuffer(int width, int height)
{
    if (width <= 0 || height <= 0) return;

    destroyBackBuffer();

    m_memDC = CreateCompatibleDC(m_hdc);
    if (!m_memDC) return;

    // Create DIB section for direct pixel access
    m_bmi.bmiHeader.biWidth = width;
    m_bmi.bmiHeader.biHeight = -height;  // Negative for top-down DIB
    m_bmi.bmiHeader.biSizeImage = width * height * 4;

    m_bitmap = CreateDIBSection(
        m_memDC,
        &m_bmi,
        DIB_RGB_COLORS,
        &m_bitmapBits,
        nullptr,
        0
    );

    if (m_bitmap) {
        m_oldBitmap = static_cast<HBITMAP>(SelectObject(m_memDC, m_bitmap));
    }
}

void Win32Renderer::destroyBackBuffer()
{
    if (m_memDC) {
        if (m_oldBitmap) {
            SelectObject(m_memDC, m_oldBitmap);
            m_oldBitmap = nullptr;
        }
        DeleteDC(m_memDC);
        m_memDC = nullptr;
    }

    if (m_bitmap) {
        DeleteObject(m_bitmap);
        m_bitmap = nullptr;
    }

    m_bitmapBits = nullptr;
}

void Win32Renderer::resize(int width, int height)
{
    if (width != m_width || height != m_height) {
        m_width = width;
        m_height = height;
        createBackBuffer(width, height);
    }
}

void Win32Renderer::beginFrame()
{
    // Nothing to do for GDI
}

void Win32Renderer::present(const uint8_t* pixels, int srcWidth, int srcHeight)
{
    if (!m_memDC || !m_bitmapBits || !pixels) return;

    m_srcWidth = srcWidth;
    m_srcHeight = srcHeight;

    // Scale source pixels to back buffer
    // For now, use simple nearest-neighbor scaling via StretchDIBits

    // Set up source bitmap info
    BITMAPINFO srcBmi = {};
    srcBmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    srcBmi.bmiHeader.biWidth = srcWidth;
    srcBmi.bmiHeader.biHeight = -srcHeight;  // Top-down
    srcBmi.bmiHeader.biPlanes = 1;
    srcBmi.bmiHeader.biBitCount = 32;
    srcBmi.bmiHeader.biCompression = BI_RGB;

    // Set stretch mode for better quality when scaling up
    SetStretchBltMode(m_hdc, HALFTONE);
    SetBrushOrgEx(m_hdc, 0, 0, nullptr);

    // Calculate aspect-ratio preserving destination rect
    float srcAspect = static_cast<float>(srcWidth) / srcHeight;
    float dstAspect = static_cast<float>(m_width) / m_height;

    int destX = 0, destY = 0;
    int destWidth = m_width, destHeight = m_height;

    if (srcAspect > dstAspect) {
        // Source is wider - letterbox top/bottom
        destHeight = static_cast<int>(m_width / srcAspect);
        destY = (m_height - destHeight) / 2;
    } else if (srcAspect < dstAspect) {
        // Source is taller - pillarbox left/right
        destWidth = static_cast<int>(m_height * srcAspect);
        destX = (m_width - destWidth) / 2;
    }

    // Clear the window background (for letterbox/pillarbox areas)
    RECT rect = {0, 0, m_width, m_height};
    FillRect(m_hdc, &rect, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));

    // Stretch blit from source pixels directly to window DC
    StretchDIBits(
        m_hdc,
        destX, destY, destWidth, destHeight,  // Destination
        0, 0, srcWidth, srcHeight,             // Source
        pixels,
        &srcBmi,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

void Win32Renderer::endFrame()
{
    // Rendering was done directly in present()
}

}  // namespace win32
}  // namespace cutie
