#ifndef DREAM_TYPES_H
#define DREAM_TYPES_H

#include <cstdint>
#include <atomic>
#include <array>

namespace dream {

// Speed throttling constants
constexpr auto FRAMEINTERVAL = 120u;
constexpr auto TARGETFRAMERATE = 60u;
constexpr auto SAMPLESPERFRAME = 262u;

// CPU timing constants
constexpr auto FRAMESPERSECOND = 59.923;
constexpr auto LINESPERSCREEN = 262.0;
constexpr auto NANOSECOND = 1000000000.0;
constexpr auto COLORBURST = 3579545.0;
constexpr auto AUDIOBUFFERS = 12u;

// Misc constants
constexpr auto QUERY = 255u;

// Default display dimensions
constexpr int DefaultWidth = 640;
constexpr int DefaultHeight = 480;

// Forward declarations
struct SystemState;

struct Point {
    int x{}, y{};
    Point() = default;
    Point(int x, int y) : x(x), y(y) {}
};

struct Size {
    int w{}, h{};
    Size() = default;
    Size(int w, int h) : w(w), h(h) {}
};

struct Rect : Point, Size {
    Rect() = default;
    Rect(int x, int y, int w, int h) : Point(x, y), Size(w, h) {}
    int top() const { return y; }
    int left() const { return x; }
    int right() const { return x + w; }
    int bottom() const { return y + h; }
};

// Bounds-checking array wrapper
template <class T, size_t Size>
struct Array {
    void memcpy_from(const void* buffer, size_t bytes) {
        memcpy(&data[0], buffer, bytes);
    }

    void memcpy_to(void* out_buffer, size_t bytes) const {
        memcpy_to(0, out_buffer, bytes);
    }

    void memcpy_to(size_t start_element, void* out_buffer, size_t bytes) const {
        memcpy(out_buffer, &data[start_element], bytes);
    }

    void memset_fill(uint8_t fill, size_t bytes) {
        memset(data.data(), fill, bytes);
    }

    T& operator[](size_t index) { return data[index]; }
    const T& operator[](size_t index) const { return data[index]; }

private:
    std::array<T, Size> data;
};

// Video array with wrap-around bounds checking
template <class T, size_t Size>
struct VideoArray {
    explicit VideoArray(T* p) : data(p) {}

    T& operator[](size_t index) {
        return data[index % Size];
    }

    const T& operator[](size_t index) const {
        return data[index % Size];
    }

private:
    T* data;
};

// Abstract interface for system state
struct ISystemState {
    enum { OK };
    virtual ~ISystemState() = default;

    virtual int get_window_handle(void** handle) = 0;
    virtual int get_rect(int rect_option, Rect* rect) = 0;
    virtual void set_surface(void* ptr, uint8_t bit_depth, long stride) = 0;
};

// CPU state for debugger
struct CPUState {
    uint16_t PC{};
    uint16_t X{};
    uint16_t Y{};
    uint16_t U{};
    uint16_t S{};
    uint16_t DP{};
    uint16_t D{};
    uint8_t A{};
    uint8_t B{};
    uint8_t CC{};
    bool E{};  // For 6309
    bool F{};  // For 6309
};

// System state - the main emulator state structure
struct SystemState {
    void* window_handle = nullptr;
    void* config_dialog = nullptr;
    void* window_instance = nullptr;

    unsigned char* ram_buffer = nullptr;
    unsigned short* wram_buffer = nullptr;
    std::atomic<unsigned char> ram_size{0};

    double cpu_current_speed = 0.0;
    unsigned char double_speed_multiplier = 0;
    unsigned char double_speed_flag = 0;
    unsigned char turbo_speed_flag = 0;
    unsigned char cpu_type = 0;
    unsigned char frame_skip = 0;
    unsigned char bit_depth = 0;
    unsigned char throttle = 0;

    // Video surface pointers
    unsigned char* surface8 = nullptr;
    unsigned short* surface16 = nullptr;
    unsigned int* surface32 = nullptr;
    long surface_pitch = 0;

    unsigned short line_counter = 0;
    unsigned char scan_lines = 0;
    unsigned char emulation_running = 0;
    unsigned char reset_pending = 0;

    Size window_size;
    unsigned char full_screen = 0;
    bool exiting = false;
    unsigned char mouse_pointer = 0;
    unsigned char overclock_flag = 0;

    char status_line[256] = {0};
    float fps = 0.0f;
};

// Helper to adapt SystemState to ISystemState interface
struct SystemStateAdapter : ISystemState {
    explicit SystemStateAdapter(SystemState* state) : state(state) {}

    int get_window_handle(void** handle) override {
        *handle = state->window_handle;
        return OK;
    }

    int get_rect(int /*rect_option*/, Rect* rect) override {
        rect->x = 0;
        rect->y = 0;
        rect->w = state->window_size.w;
        rect->h = state->window_size.h;
        return OK;
    }

    void set_surface(void* ptr, uint8_t bit_depth, long stride) override {
        state->surface8 = static_cast<unsigned char*>(ptr);
        state->surface16 = static_cast<unsigned short*>(ptr);
        state->surface32 = static_cast<unsigned int*>(ptr);
        state->bit_depth = bit_depth;
        state->surface_pitch = stride;
    }

private:
    SystemState* state;
};

// Audio rate options
constexpr unsigned int AudioRates[] = {0, 11025, 22050, 44100};

} // namespace dream

#endif // DREAM_TYPES_H
