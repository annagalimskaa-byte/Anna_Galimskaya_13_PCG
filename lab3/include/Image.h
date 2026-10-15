#pragma once
#include <string>
#include <vector>
#include <cstdint>

class Image {
public:
    int width = 0;
    int height = 0;
    std::vector<uint8_t> data;

    Image() = default;
    Image(int w, int h);

    uint8_t& at(int x, int y);
    uint8_t at(int x, int y) const;

    void fromQImage(const uint8_t* rgb, int w, int h);

    std::vector<int> histogram() const;
};