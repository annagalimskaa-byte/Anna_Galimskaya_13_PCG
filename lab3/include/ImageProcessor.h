#pragma once
#include "Image.h"

class ImageProcessor {
public:
    static Image otsuThreshold(const Image& src);
    static Image iterativeThreshold(const Image& src);
    static Image adaptiveThreshold(const Image& src, int windowSize, int C);

    static int findOtsuThreshold(const Image& src);
    static int findIterativeThreshold(const Image& src);
};