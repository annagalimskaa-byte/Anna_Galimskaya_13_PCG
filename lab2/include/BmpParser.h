
#pragma once
#include <string>
#include "ImageInfo.h"

class BmpParser {
public:
    static ImageInfo parse(const std::string& filePath);
};

