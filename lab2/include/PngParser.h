#pragma once
#include <string>
#include "ImageInfo.h"

class PngParser {
public:
    static ImageInfo parse(const std::string& filePath);
};