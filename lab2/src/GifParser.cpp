#include "GifParser.h"
#include <fstream>
#include <cstring>
#include <iostream>

namespace {

uint16_t readLittleEndian16(const unsigned char* data) {
    return static_cast<uint16_t>(data[0]) |
           (static_cast<uint16_t>(data[1]) << 8);
}

}

ImageInfo GifParser::parse(const std::string& filePath) {
    ImageInfo info;
    info.filePath = filePath;
    info.format = "GIF";

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

    unsigned char header[6];
    file.read(reinterpret_cast<char*>(header), 6);
    if (file.gcount() != 6) {
        info.status = ParseStatus::IO_ERROR;
        info.statusMessage = "File too small";
        return info;
    }

    if (std::memcmp(header, "GIF87a", 6) != 0 && std::memcmp(header, "GIF89a", 6) != 0) {
        info.status = ParseStatus::UNKNOWN_FORMAT;
        info.statusMessage = "Not a GIF file";
        return info;
    }

    info.extraInfo = std::string(reinterpret_cast<char*>(header), 6);

    unsigned char lsd[7];
    file.read(reinterpret_cast<char*>(lsd), 7);
    if (file.gcount() != 7) {
        info.status = ParseStatus::CORRUPTED;
        info.statusMessage = "Truncated logical screen descriptor";
        return info;
    }

    info.width  = readLittleEndian16(lsd);
    info.height = readLittleEndian16(lsd + 2);

    unsigned char packed = lsd[4];
    bool hasGlobalColorTable = (packed & 0x80) != 0;
    uint8_t colorTableSizeBits = (packed & 0x07);
    uint32_t colorTableSize = 1u << (colorTableSizeBits + 1);

    if (hasGlobalColorTable) {
        info.colorDepth = colorTableSizeBits + 1;
        info.extraInfo += ", " + std::to_string(colorTableSize) + " colors";

        uint32_t gctBytes = colorTableSize * 3;
        file.seekg(gctBytes, std::ios::cur);
    }

    info.compression = "LZW";

    bool foundTrailer = false;
    while (file.good()) {
        unsigned char blockType;
        file.read(reinterpret_cast<char*>(&blockType), 1);
        if (!file) break;

        std::streampos pos = file.tellg();
        std::cerr << "[DEBUG] block=0x" << std::hex << (int)blockType
                  << std::dec << " offset=" << (long long)pos - 1 << std::endl;

        if (blockType == 0x3B) {
            foundTrailer = true;
            break;
        }

        if (blockType == 0x21) {
            unsigned char label;
            file.read(reinterpret_cast<char*>(&label), 1);
            if (!file) break;

            std::cerr << "[DEBUG]   extension label=0x" << std::hex << (int)label
                      << std::dec << std::endl;

            if (label == 0xFF) {
                unsigned char blockSize;
                file.read(reinterpret_cast<char*>(&blockSize), 1);
                if (!file) break;
                file.seekg(blockSize, std::ios::cur);
            }

            while (file.good()) {
                unsigned char blockSize;
                file.read(reinterpret_cast<char*>(&blockSize), 1);
                if (!file) break;
                if (blockSize == 0) break;
                file.seekg(blockSize, std::ios::cur);
            }
        }
        else if (blockType == 0x2C) {
            unsigned char desc[9];
            file.read(reinterpret_cast<char*>(desc), 9);
            if (file.gcount() != 9) break;

            unsigned char imgPacked = desc[8];
            if (imgPacked & 0x80) {
                uint8_t localColorBits = imgPacked & 0x07;
                uint32_t localTableSize = (1u << (localColorBits + 1)) * 3;
                file.seekg(localTableSize, std::ios::cur);
            }

            unsigned char lzwMinCodeSize;
            file.read(reinterpret_cast<char*>(&lzwMinCodeSize), 1);
            if (!file) break;

            while (file.good()) {
                unsigned char blockSize;
                file.read(reinterpret_cast<char*>(&blockSize), 1);
                if (!file) break;
                if (blockSize == 0) break;
                file.seekg(blockSize, std::ios::cur);
            }
        }
        else {
            std::cerr << "[DEBUG] Unknown block type: 0x" << std::hex
                      << (int)blockType << std::dec << std::endl;
            info.status = ParseStatus::CORRUPTED;
            info.statusMessage = "Unknown block type";
            return info;
        }
    }

    if (!foundTrailer) {
        info.status = ParseStatus::CORRUPTED;
        info.statusMessage = "Missing trailer";
        return info;
    }

    return info;
}