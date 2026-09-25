#include "TiffParser.h"
#include <fstream>
#include <cstring>
#include <iostream>
#include <vector>

namespace {

uint16_t read16(const unsigned char* data, bool littleEndian) {
    if (littleEndian)
        return static_cast<uint16_t>(data[0]) |
               (static_cast<uint16_t>(data[1]) << 8);
    return (static_cast<uint16_t>(data[0]) << 8) |
            static_cast<uint16_t>(data[1]);
}

uint32_t read32(const unsigned char* data, bool littleEndian) {
    if (littleEndian)
        return static_cast<uint32_t>(data[0]) |
               (static_cast<uint32_t>(data[1]) << 8) |
               (static_cast<uint32_t>(data[2]) << 16) |
               (static_cast<uint32_t>(data[3]) << 24);
    return (static_cast<uint32_t>(data[0]) << 24) |
           (static_cast<uint32_t>(data[1]) << 16) |
           (static_cast<uint32_t>(data[2]) << 8) |
            static_cast<uint32_t>(data[3]);
}

uint32_t readValue(const unsigned char* entry, uint16_t type, bool littleEndian) {
    if (type == 3) {
        return read16(entry + 8, littleEndian);
    }
    return read32(entry + 8, littleEndian);
}

}

ImageInfo TiffParser::parse(const std::string& filePath) {
    ImageInfo info;
    info.filePath = filePath;
    info.format = "TIFF";

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

    unsigned char header[8];
    file.read(reinterpret_cast<char*>(header), 8);
    if (file.gcount() != 8) {
        info.status = ParseStatus::IO_ERROR;
        info.statusMessage = "File too small";
        return info;
    }

    bool littleEndian;
    if (header[0] == 'I' && header[1] == 'I') {
        littleEndian = true;
        info.extraInfo = "Little-endian";
    } else if (header[0] == 'M' && header[1] == 'M') {
        littleEndian = false;
        info.extraInfo = "Big-endian";
    } else {
        info.status = ParseStatus::UNKNOWN_FORMAT;
        info.statusMessage = "Not a TIFF file";
        return info;
    }

    uint16_t magic = read16(header + 2, littleEndian);
    if (magic != 42) {
        info.status = ParseStatus::UNKNOWN_FORMAT;
        info.statusMessage = "Invalid TIFF magic";
        return info;
    }

    uint32_t ifdOffset = read32(header + 4, littleEndian);

    file.seekg(ifdOffset, std::ios::beg);
    unsigned char countBytes[2];
    file.read(reinterpret_cast<char*>(countBytes), 2);
    if (file.gcount() != 2) {
        info.status = ParseStatus::CORRUPTED;
        info.statusMessage = "Cannot read IFD count";
        return info;
    }

    uint16_t entryCount = read16(countBytes, littleEndian);

    uint32_t xResolution = 0;
    uint32_t yResolution = 0;
    uint16_t resolutionUnit = 2;
    std::vector<uint16_t> bitsPerSample;

    for (uint16_t i = 0; i < entryCount; i++) {
        unsigned char entry[12];
        file.read(reinterpret_cast<char*>(entry), 12);
        if (file.gcount() != 12) break;

        uint16_t tag = read16(entry, littleEndian);
        uint16_t type = read16(entry + 2, littleEndian);
        uint32_t count = read32(entry + 4, littleEndian);
        uint32_t value = readValue(entry, type, littleEndian);

        switch (tag) {
            case 0x0100:
                info.width = value;
                break;
            case 0x0101:
                info.height = value;
                break;
            case 0x0102: {
                if (type == 3) {
                    if (count == 1) {
                        bitsPerSample.push_back(static_cast<uint16_t>(value));
                    } else if (count > 1) {
                        std::streampos savedPos = file.tellg();
                        file.seekg(read32(entry + 8, littleEndian), std::ios::beg);
                        for (uint32_t k = 0; k < count; k++) {
                            unsigned char bs[2];
                            file.read(reinterpret_cast<char*>(bs), 2);
                            bitsPerSample.push_back(read16(bs, littleEndian));
                        }
                        file.seekg(savedPos, std::ios::beg);
                    }
                }
                break;
            }
            case 0x0103:
                switch (value) {
                    case 1: info.compression = "None"; break;
                    case 2: info.compression = "CCITT RLE"; break;
                    case 3: info.compression = "CCITT G3"; break;
                    case 4: info.compression = "CCITT G4"; break;
                    case 5: info.compression = "LZW"; break;
                    case 6: info.compression = "JPEG (old)"; break;
                    case 7: info.compression = "JPEG"; break;
                    case 8: info.compression = "Deflate"; break;
                    case 32773: info.compression = "PackBits"; break;
                    default: info.compression = "Unknown (" + std::to_string(value) + ")";
                }
                break;
            case 0x011A: {
                std::streampos savedPos = file.tellg();
                file.seekg(read32(entry + 8, littleEndian), std::ios::beg);
                unsigned char numBytes[4], denBytes[4];
                file.read(reinterpret_cast<char*>(numBytes), 4);
                file.read(reinterpret_cast<char*>(denBytes), 4);
                uint32_t numerator = read32(numBytes, littleEndian);
                uint32_t denominator = read32(denBytes, littleEndian);
                if (denominator != 0) {
                    xResolution = numerator / denominator;
                }
                file.seekg(savedPos, std::ios::beg);
                break;
            }
            case 0x011B: {
                std::streampos savedPos = file.tellg();
                file.seekg(read32(entry + 8, littleEndian), std::ios::beg);
                unsigned char numBytes[4], denBytes[4];
                file.read(reinterpret_cast<char*>(numBytes), 4);
                file.read(reinterpret_cast<char*>(denBytes), 4);
                uint32_t numerator = read32(numBytes, littleEndian);
                uint32_t denominator = read32(denBytes, littleEndian);
                if (denominator != 0) {
                    yResolution = numerator / denominator;
                }
                file.seekg(savedPos, std::ios::beg);
                break;
            }
            case 0x0128:
                resolutionUnit = static_cast<uint16_t>(value);
                break;
            default:
                break;
        }
    }

    if (!bitsPerSample.empty()) {
        uint32_t totalBits = 0;
        for (uint16_t b : bitsPerSample) totalBits += b;
        info.colorDepth = static_cast<uint16_t>(totalBits);
    }

    if (xResolution > 0) {
        if (resolutionUnit == 3) {
            info.dpi = xResolution * 2.54;
        } else {
            info.dpi = xResolution;
        }
    }

    if (info.width == 0 || info.height == 0) {
        info.status = ParseStatus::CORRUPTED;
        info.statusMessage = "Missing width/height tags";
        return info;
    }

    if (info.compression.empty()) {
        info.compression = "None";
    }

    return info;
}