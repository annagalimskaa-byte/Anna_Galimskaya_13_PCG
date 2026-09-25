#pragma once
#include <string>
#include "ImageInfo.h"

class GifParser {
public:
    static ImageInfo parse(const std::string& filePath);
};