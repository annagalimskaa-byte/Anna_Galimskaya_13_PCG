#pragma once
#include <string>
#include "ImageInfo.h"

class PcxParser {
public:
    static ImageInfo parse(const std::string& filePath);
};