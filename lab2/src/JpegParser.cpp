#include "JpegParser.h"
#include <fstream>
#include <cstring>
#include <iostream>

namespace {

uint16_t readBigEndian16(const unsigned char* data) {
    return (static_cast<uint16_t>(data[0]) << 8) |
           static_cast<uint16_t>(data[1]);
}

}

ImageInfo JpegParser::parse(const std::string& filePath) {
    ImageInfo info;
    info.filePath = filePath;
    info.format = "JPEG";

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

    unsigned char soi[2];
    file.read(reinterpret_cast<char*>(soi), 2);
    if (!file || soi[0] != 0xFF || soi[1] != 0xD8) {
        info.status = ParseStatus::UNKNOWN_FORMAT;
        info.statusMessage = "Not a JPEG file";
        return info;
    }

    bool foundSOF = false;
    bool foundEOI = false;

    while (file.good()) {
        unsigned char prefix;
        file.read(reinterpret_cast<char*>(&prefix), 1);
        if (!file) break;

        if (prefix != 0xFF) continue;

        unsigned char marker;
        file.read(reinterpret_cast<char*>(&marker), 1);
        if (!file) break;

        while (marker == 0xFF) {
            file.read(reinterpret_cast<char*>(&marker), 1);
            if (!file) break;
        }

        if (marker == 0xD9) {
            foundEOI = true;
            break;
        }

        if (marker == 0x01 || (marker >= 0xD0 && marker <= 0xD7)) {
            continue;
        }

        unsigned char lengthBytes[2];
        file.read(reinterpret_cast<char*>(lengthBytes), 2);
        if (file.gcount() != 2) break;

        uint16_t segmentLength = readBigEndian16(lengthBytes);
        if (segmentLength < 2) break;

        uint16_t dataLength = segmentLength - 2;

        if (marker == 0xC0 || marker == 0xC1 || marker == 0xC2 || marker == 0xC3) {
            unsigned char sof[8];
            file.read(reinterpret_cast<char*>(sof), 8);
            if (file.gcount() != 8) {
                info.status = ParseStatus::CORRUPTED;
                info.statusMessage = "Truncated SOF";
                return info;
            }

            info.colorDepth = sof[0];
            info.height = readBigEndian16(sof + 1);
            info.width  = readBigEndian16(sof + 3);
            uint8_t components = sof[5];

            if (marker == 0xC0) info.compression = "Baseline";
            else if (marker == 0xC1) info.compression = "Extended";
            else if (marker == 0xC2) info.compression = "Progressive";
            else info.compression = "Lossless";

            info.extraInfo = std::to_string(components) + " components";

            file.seekg(dataLength - 8, std::ios::cur);
            foundSOF = true;
        }
        else if (marker == 0xE0) {
            unsigned char app0[14];
            file.read(reinterpret_cast<char*>(app0), 14);
            if (file.gcount() == 14) {
                if (std::memcmp(app0, "JFIF\0", 5) == 0) {
                    uint8_t units = app0[7];
                    uint16_t xDensity = readBigEndian16(app0 + 8);

                    if (xDensity > 0) {
                        if (units == 0)      info.dpi = xDensity;
                        else if (units == 1) info.dpi = xDensity * 0.0254;
                        else if (units == 2) info.dpi = xDensity * 2.54;
                    }
                }
                file.seekg(dataLength - 14, std::ios::cur);
            } else {
                file.seekg(dataLength, std::ios::cur);
            }
        }
        else if (marker == 0xDA) {
            file.seekg(dataLength, std::ios::cur);

            while (file.good()) {
                unsigned char b;
                file.read(reinterpret_cast<char*>(&b), 1);
                if (!file) break;

                if (b != 0xFF) continue;

                unsigned char next;
                file.read(reinterpret_cast<char*>(&next), 1);
                if (!file) break;

                if (next == 0x00) continue;

                if (next >= 0xD0 && next <= 0xD7) continue;

                if (next == 0xD9) {
                    foundEOI = true;
                    break;
                }

                if (next == 0xFF) {
                    file.seekg(-1, std::ios::cur);
                    continue;
                }
            }
            break;
        }
        else {
            file.seekg(dataLength, std::ios::cur);
        }
    }

    if (!foundSOF) {
        info.status = ParseStatus::CORRUPTED;
        info.statusMessage = "Missing SOF marker";
        return info;
    }

    if (!foundEOI) {
        info.status = ParseStatus::CORRUPTED;
        info.statusMessage = "Missing EOI marker";
        return info;
    }

    return info;
}