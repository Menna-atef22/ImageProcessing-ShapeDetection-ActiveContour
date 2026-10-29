#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Plain image containers (no OpenCV, no Qt types) so every algorithm is
// written "from scratch" on raw arrays.

struct GrayImage {
    int width = 0, height = 0;
    std::vector<float> data;  // row-major, values usually in [0,255]

    GrayImage() = default;
    GrayImage(int w, int h, float v = 0.f) : width(w), height(h), data(size_t(w) * h, v) {}

    bool empty() const { return data.empty(); }
    float& at(int x, int y) { return data[size_t(y) * width + x]; }
    float at(int x, int y) const { return data[size_t(y) * width + x]; }
    // Border-clamped read (replicate edge pixels)
    float atClamped(int x, int y) const {
        x = x < 0 ? 0 : (x >= width ? width - 1 : x);
        y = y < 0 ? 0 : (y >= height ? height - 1 : y);
        return data[size_t(y) * width + x];
    }
};

struct RgbImage {
    int width = 0, height = 0;
    std::vector<uint8_t> data;  // 3 bytes per pixel: R,G,B

    RgbImage() = default;
    RgbImage(int w, int h) : width(w), height(h), data(size_t(w) * h * 3, 0) {}

    bool empty() const { return data.empty(); }
    uint8_t* px(int x, int y) { return &data[(size_t(y) * width + x) * 3]; }
    const uint8_t* px(int x, int y) const { return &data[(size_t(y) * width + x) * 3]; }
};

namespace ImageIO {
// Only file reading/writing uses Qt (QImage). Everything else is plain C++.
bool loadRgb(const std::string& path, RgbImage& out);
bool saveRgb(const std::string& path, const RgbImage& img);
bool saveGray(const std::string& path, const GrayImage& img);
}  // namespace ImageIO

// Conversions
GrayImage toGray(const RgbImage& rgb);         // luminance 0.299R+0.587G+0.114B
RgbImage grayToRgb(const GrayImage& gray);     // clamps to [0,255]
GrayImage normalizeTo255(const GrayImage& g);  // stretch min..max -> 0..255 (for display)
