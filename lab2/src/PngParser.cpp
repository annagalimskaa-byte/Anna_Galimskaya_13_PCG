#include <cstring>
#include "PngParser.h"
#include <fstream>
#include <vector>

namespace {

const unsigned char PNG_SIGNATURE[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};

uint32_t readBigEndian32(const unsigned char* data) {
    return (static_cast<uint32_t>(data[0]) << 24) |
           (static_cast<uint32_t>(data[1]) << 16) |
           (static_cast<uint32_t>(data[2]) << 8)  |
           (static_cast<uint32_t>(data[3]));
}

}

ImageInfo PngParser::parse(const std::string& filePath) {
    ImageInfo info;
    info.filePath = filePath;
    info.format = "PNG";

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

    unsigned char signature[8];
    file.read(reinterpret_cast<char*>(signature), 8);
    if (!file || std::memcmp(signature, PNG_SIGNATURE, 8) != 0) {
        info.status = ParseStatus::UNKNOWN_FORMAT;
        info.statusMessage = "Not a PNG file";
        return info;
    }

    bool foundIHDR = false;
    bool foundIEND = false;
    uint32_t dpiFromPhys = 0;

    while (file.good()) {
        unsigned char lengthBytes[4];
        unsigned char typeBytes[4];

        file.read(reinterpret_cast<char*>(lengthBytes), 4);
        if (file.gcount() != 4) break;

        file.read(reinterpret_cast<char*>(typeBytes), 4);
        if (file.gcount() != 4) break;

        uint32_t chunkLength = readBigEndian32(lengthBytes);
        std::string chunkType(reinterpret_cast<char*>(typeBytes), 4);

        if (chunkType == "IHDR") {
            unsigned char ihdr[13];
            file.read(reinterpret_cast<char*>(ihdr), 13);
            if (file.gcount() != 13) {
                info.status = ParseStatus::CORRUPTED;
                info.statusMessage = "Truncated IHDR";
                return info;
            }

            info.width  = readBigEndian32(ihdr);
            info.height = readBigEndian32(ihdr + 4);
            info.colorDepth = ihdr[8];

            uint8_t colorType = ihdr[9];
            switch (colorType) {
                case 0: info.extraInfo = "Grayscale"; break;
                case 2: info.extraInfo = "Truecolor"; break;
                case 3: info.extraInfo = "Indexed"; break;
                case 4: info.extraInfo = "Grayscale+Alpha"; break;
                case 6: info.extraInfo = "Truecolor+Alpha"; break;
                default: info.extraInfo = "Unknown";
            }

            info.compression = (ihdr[10] == 0) ? "Deflate" : "Unknown";
            foundIHDR = true;

            file.seekg(4, std::ios::cur);
        }
        else if (chunkType == "pHYs") {
            unsigned char phys[9];
            file.read(reinterpret_cast<char*>(phys), 9);
            if (file.gcount() == 9) {
                uint32_t ppuX = readBigEndian32(phys);
                uint8_t unit = phys[8];
                if (unit == 1) {
                    dpiFromPhys = ppuX;
                }
            }
            file.seekg(4, std::ios::cur);
        }
        else if (chunkType == "IEND") {
            foundIEND = true;
            break;
        }
        else {
            file.seekg(chunkLength + 4, std::ios::cur);
        }
    }

    if (!foundIHDR) {
        info.status = ParseStatus::CORRUPTED;
        info.statusMessage = "Missing IHDR chunk";
        return info;
    }

    if (!foundIEND) {
        info.status = ParseStatus::CORRUPTED;
        info.statusMessage = "Missing IEND chunk";
        return info;
    }

    if (dpiFromPhys > 0) {
        info.dpi = dpiFromPhys * 0.0254;
    }

    return info;
}