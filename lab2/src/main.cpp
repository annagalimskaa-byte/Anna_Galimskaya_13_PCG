#include <iostream>
#include <filesystem>
#include <vector>
#include <cctype>
#include <mutex>
#include <chrono>
#include <thread>
#include "ImageInfo.h"
#include "BmpParser.h"
#include "PngParser.h"
#include "JpegParser.h"
#include "GifParser.h"
#include "TiffParser.h"
#include "PcxParser.h"
#include "ThreadPool.h"

namespace fs = std::filesystem;

ImageInfo parseFile(const fs::path& filePath) {
    std::string ext = filePath.extension().string();
    for (auto& c : ext) c = std::tolower(c);

    if (ext == ".bmp")  return BmpParser::parse(filePath.string());
    if (ext == ".png")  return PngParser::parse(filePath.string());
    if (ext == ".jpg" || ext == ".jpeg") return JpegParser::parse(filePath.string());
    if (ext == ".gif")  return GifParser::parse(filePath.string());
    if (ext == ".tif" || ext == ".tiff") return TiffParser::parse(filePath.string());
    if (ext == ".pcx")  return PcxParser::parse(filePath.string());

    ImageInfo info;
    info.filePath = filePath.string();
    info.fileName = filePath.filename().string();
    info.format = "UNKNOWN";
    info.status = ParseStatus::UNKNOWN_FORMAT;
    return info;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <path_to_folder_or_file>" << std::endl;
        return 1;
    }

    std::string inputPath = argv[1];
    fs::path path(inputPath);

    std::vector<fs::path> files;

    if (fs::is_directory(path)) {
        for (const auto& entry : fs::recursive_directory_iterator(path)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path());
            }
        }
    } else if (fs::is_regular_file(path)) {
        files.push_back(path);
    } else {
        std::cout << "Invalid path: " << inputPath << std::endl;
        return 1;
    }

    std::cout << "Found " << files.size() << " files" << std::endl;

    size_t numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4;
    std::cout << "Using " << numThreads << " threads" << std::endl;

    auto startTime = std::chrono::high_resolution_clock::now();

    std::vector<ImageInfo> results;
    results.reserve(files.size());
    std::mutex resultsMutex;
    int okCount = 0;
    int errorCount = 0;
    std::mutex countersMutex;

    {
        ThreadPool pool(numThreads);
        for (const auto& filePath : files) {
            pool.enqueue([filePath, &results, &resultsMutex, &okCount, &errorCount, &countersMutex]() {
                ImageInfo info = parseFile(filePath);
                {
                    std::lock_guard<std::mutex> lock(resultsMutex);
                    results.push_back(info);
                }
                {
                    std::lock_guard<std::mutex> lock(countersMutex);
                    if (info.status == ParseStatus::OK) okCount++;
                    else errorCount++;
                }
            });
        }
        pool.waitAll();
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    std::cout << "\nResults:" << std::endl;
    for (const auto& info : results) {
        std::cout << info.fileName
                  << " | " << info.width << "x" << info.height
                  << " | " << info.colorDepth << " bit"
                  << " | " << info.compression
                  << " | DPI=" << info.dpi
                  << " | status=" << static_cast<int>(info.status)
                  << std::endl;
    }

    std::cout << "\nSummary:" << std::endl;
    std::cout << "  OK:       " << okCount << std::endl;
    std::cout << "  Errors:   " << errorCount << std::endl;
    std::cout << "  Time:     " << duration << " ms" << std::endl;

    return 0;
}