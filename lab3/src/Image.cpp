#include "Image.h"

Image::Image(int w, int h) : width(w), height(h), data(w * h, 0) {}

uint8_t& Image::at(int x, int y) {
    return data[y * width + x];
}

uint8_t Image::at(int x, int y) const {
    return data[y * width + x];
}

void Image::fromQImage(const uint8_t* rgb, int w, int h) {
    width = w;
    height = h;
    data.resize(w * h);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = (y * w + x) * 4;
            uint8_t r = rgb[idx];
            uint8_t g = rgb[idx + 1];
            uint8_t b = rgb[idx + 2];

            data[y * w + x] = static_cast<uint8_t>(0.299 * r + 0.587 * g + 0.114 * b);
        }
    }
}

std::vector<int> Image::histogram() const {
    std::vector<int> hist(256, 0);
    for (uint8_t pixel : data) {
        hist[pixel]++;
    }
    return hist;
}