#pragma once
#include <string>
#include <cstdint>

enum class ParseStatus {
    OK,
    CORRUPTED,
    UNKNOWN_FORMAT,
    IO_ERROR
};

struct ImageInfo {
    std::string fileName;
    std::string filePath;
    std::string format;

    uint32_t width = 0;
    uint32_t height = 0;
    double dpi = 0.0;
    uint16_t colorDepth = 0;
    std::string compression;

    ParseStatus status = ParseStatus::OK;
    std::string statusMessage;

    std::string extraInfo;
};