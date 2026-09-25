#include "BmpParser.h"
#include <fstream>
#include <cstring>
#include <cmath>

#pragma pack(push, 1)
struct BmpFileHeader {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
};

struct BmpInfoHeader {
    uint32_t biSize;
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
};
#pragma pack(pop)

ImageInfo BmpParser::parse(const std::string& filePath) {
    ImageInfo info;
    info.filePath = filePath;
    info.format = "BMP";

    size_t slashPos = filePath.find_last_of("/\\");
    info.fileName = (slashPos == std::string::npos)
        ? filePath
        : filePath.substr(slashPos + 1);

    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        info.status = ParseStatus::IO_ERROR;
        info.statusMessage = "Cannot open file";
        return info;
    }

    BmpFileHeader fileHeader{};
    file.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
    if (!file || fileHeader.bfType != 0x4D42) {
        info.status = ParseStatus::UNKNOWN_FORMAT;
        info.statusMessage = "Not a BMP file";
        return info;
    }

    BmpInfoHeader infoHeader{};
    file.read(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));
    if (!file) {
        info.status = ParseStatus::CORRUPTED;
        info.statusMessage = "Failed to read BMP info header";
        return info;
    }

    info.width = static_cast<uint32_t>(std::abs(infoHeader.biWidth));
    info.height = static_cast<uint32_t>(std::abs(infoHeader.biHeight));
    info.colorDepth = infoHeader.biBitCount;

    if (infoHeader.biXPelsPerMeter > 0) {
        info.dpi = infoHeader.biXPelsPerMeter * 0.0254;
    }

    switch (infoHeader.biCompression) {
        case 0: info.compression = "None"; break;
        case 1: info.compression = "RLE8"; break;
        case 2: info.compression = "RLE4"; break;
        case 3: info.compression = "Bitfields"; break;
        case 4: info.compression = "JPEG"; break;
        case 5: info.compression = "PNG"; break;
        default: info.compression = "Unknown";
    }

    file.seekg(0, std::ios::end);
    std::streampos actualSize = file.tellg();
    if (fileHeader.bfSize > actualSize) {
        info.status = ParseStatus::CORRUPTED;
        info.statusMessage = "File size mismatch";
    }

    return info;
}