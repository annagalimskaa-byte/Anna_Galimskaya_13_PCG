#pragma once
#include <string>
#include "ImageInfo.h"

class TiffParser {
public:
    static ImageInfo parse(const std::string& filePath);
};