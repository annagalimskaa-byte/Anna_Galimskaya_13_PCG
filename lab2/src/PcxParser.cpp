#include "PcxParser.h"
#include <fstream>
#include <cstring>
#include <iostream>

namespace {

uint16_t readLittleEndian16(const unsigned char* data) {
    return static_cast<uint16_t>(data[0]) |
           (static_cast<uint16_t>(data[1]) << 8);
}

}

ImageInfo PcxParser::parse(const std::string& filePath) {
    ImageInfo info;
    info.filePath = filePath;
    info.format = "PCX";

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

    unsigned char header[128];
    file.read(reinterpret_cast<char*>(header), 128);
    if (file.gcount() != 128) {
        info.status = ParseStatus::IO_ERROR;
        info.statusMessage = "File too small for PCX header";
        return info;
    }

    if (header[0] != 0x0A) {
        info.status = ParseStatus::UNKNOWN_FORMAT;
        info.statusMessage = "Not a PCX file";
        return info;
    }

    uint8_t version = header[1];
    uint8_t encoding = header[2];
    uint8_t bitsPerPixel = header[3];

    uint16_t xmin = readLittleEndian16(header + 4);
    uint16_t ymin = readLittleEndian16(header + 6);
    uint16_t xmax = readLittleEndian16(header + 8);
    uint16_t ymax = readLittleEndian16(header + 10);
    uint16_t hdpi = readLittleEndian16(header + 12);
    uint16_t vdpi = readLittleEndian16(header + 14);
    uint8_t nPlanes = header[65];
    uint16_t bytesPerLine = readLittleEndian16(header + 66);

    if (xmax < xmin || ymax < ymin) {
        info.status = ParseStatus::CORRUPTED;
        info.statusMessage = "Invalid PCX dimensions";
        return info;
    }

    info.width = static_cast<uint32_t>(xmax - xmin + 1);
    info.height = static_cast<uint32_t>(ymax - ymin + 1);
    info.dpi = hdpi;
    info.colorDepth = static_cast<uint16_t>(bitsPerPixel) * nPlanes;
    info.compression = (encoding == 1) ? "RLE" : "None";

    info.extraInfo = "v" + std::to_string(version)
                   + ", " + std::to_string(nPlanes) + " planes"
                   + ", " + std::to_string(bytesPerLine) + " bytes/line";

    bool foundEOI = false;

    while (file.good()) {
        unsigned char b;
        file.read(reinterpret_cast<char*>(&b), 1);
        if (!file) break;

        if (b == 0x0C) {
            unsigned char nextByte;
            file.read(reinterpret_cast<char*>(&nextByte), 1);
            if (!file) break;

            if (nextByte == 0x00) {
                foundEOI = true;
                break;
            }
        }
    }

    if (!foundEOI) {
        info.status = ParseStatus::CORRUPTED;
        info.statusMessage = "Missing EOI marker (0x0C)";
        return info;
    }

    return info;
}