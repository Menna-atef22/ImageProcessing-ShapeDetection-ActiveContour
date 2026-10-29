#include "Image.h"

#include <QImage>
#include <algorithm>

namespace ImageIO {

bool loadRgb(const std::string& path, RgbImage& out) {
    QImage q;
    if (!q.load(QString::fromStdString(path))) return false;
    q = q.convertToFormat(QImage::Format_RGB888);
    out = RgbImage(q.width(), q.height());
    for (int y = 0; y < q.height(); ++y) {
        const uint8_t* line = q.constScanLine(y);  // scanlines are padded -> copy per row
        std::copy(line, line + size_t(q.width()) * 3, out.px(0, y));
    }
    return true;
}

bool saveRgb(const std::string& path, const RgbImage& img) {
    if (img.empty()) return false;
    QImage q(img.width, img.height, QImage::Format_RGB888);
    for (int y = 0; y < img.height; ++y)
        std::copy(img.px(0, y), img.px(0, y) + size_t(img.width) * 3, q.scanLine(y));
    return q.save(QString::fromStdString(path));
}

bool saveGray(const std::string& path, const GrayImage& img) { return saveRgb(path, grayToRgb(img)); }

}  // namespace ImageIO

GrayImage toGray(const RgbImage& rgb) {
    GrayImage g(rgb.width, rgb.height);
    for (int y = 0; y < rgb.height; ++y)
        for (int x = 0; x < rgb.width; ++x) {
            const uint8_t* p = rgb.px(x, y);
            g.at(x, y) = 0.299f * p[0] + 0.587f * p[1] + 0.114f * p[2];
        }
    return g;
}

RgbImage grayToRgb(const GrayImage& gray) {
    RgbImage out(gray.width, gray.height);
    for (int y = 0; y < gray.height; ++y)
        for (int x = 0; x < gray.width; ++x) {
            float v = std::clamp(gray.at(x, y), 0.f, 255.f);
            uint8_t* p = out.px(x, y);
            p[0] = p[1] = p[2] = uint8_t(v + 0.5f);
        }
    return out;
}

GrayImage normalizeTo255(const GrayImage& g) {
    if (g.empty()) return g;
    auto [mn, mx] = std::minmax_element(g.data.begin(), g.data.end());
    float lo = *mn, range = *mx - *mn;
    GrayImage out(g.width, g.height);
    for (size_t i = 0; i < g.data.size(); ++i) out.data[i] = range > 0 ? (g.data[i] - lo) * 255.f / range : 0.f;
    return out;
}
