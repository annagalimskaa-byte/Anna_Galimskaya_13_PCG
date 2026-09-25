#pragma once
#include <string>
#include "ImageInfo.h"

class JpegParser {
public:
    static ImageInfo parse(const std::string& filePath);
};