#include "ImageProcessor.h"
#include <cmath>
#include <vector>


int ImageProcessor::findOtsuThreshold(const Image& src) {
    std::vector<int> hist = src.histogram();
    int total = src.width * src.height;

    double sum = 0;
    for (int i = 0; i < 256; i++) {
        sum += i * hist[i];
    }

    double sumB = 0;
    int wB = 0;
    double maxVariance = 0;
    int bestThreshold = 0;

    for (int t = 0; t < 256; t++) {
        wB += hist[t];
        if (wB == 0) continue;

        int wF = total - wB;
        if (wF == 0) break;

        sumB += t * hist[t];

        double mB = sumB / wB;
        double mF = (sum - sumB) / wF;

        double variance = (double)wB * wF * (mB - mF) * (mB - mF);

        if (variance > maxVariance) {
            maxVariance = variance;
            bestThreshold = t;
        }
    }

    return bestThreshold;
}

Image ImageProcessor::otsuThreshold(const Image& src) {
    int threshold = findOtsuThreshold(src);

    Image result(src.width, src.height);
    for (int y = 0; y < src.height; y++) {
        for (int x = 0; x < src.width; x++) {
            result.at(x, y) = (src.at(x, y) > threshold) ? 255 : 0;
        }
    }
    return result;
}


int ImageProcessor::findIterativeThreshold(const Image& src) {
    std::vector<int> hist = src.histogram();

    double sum = 0;
    for (int i = 0; i < 256; i++) {
        sum += i * hist[i];
    }
    double T = sum / (src.width * src.height);

    for (int iter = 0; iter < 100; iter++) {
        double sum1 = 0, sum2 = 0;
        int count1 = 0, count2 = 0;

        for (int i = 0; i < 256; i++) {
            if (i < T) {
                sum1 += i * hist[i];
                count1 += hist[i];
            } else {
                sum2 += i * hist[i];
                count2 += hist[i];
            }
        }

        double m1 = (count1 > 0) ? sum1 / count1 : 0;
        double m2 = (count2 > 0) ? sum2 / count2 : 0;

        double newT = (m1 + m2) / 2.0;

        if (std::abs(newT - T) < 0.5) {
            T = newT;
            break;
        }
        T = newT;
    }

    return static_cast<int>(T);
}

Image ImageProcessor::iterativeThreshold(const Image& src) {
    int threshold = findIterativeThreshold(src);

    Image result(src.width, src.height);
    for (int y = 0; y < src.height; y++) {
        for (int x = 0; x < src.width; x++) {
            result.at(x, y) = (src.at(x, y) > threshold) ? 255 : 0;
        }
    }
    return result;
}


Image ImageProcessor::adaptiveThreshold(const Image& src, int windowSize, int C) {
    Image result(src.width, src.height);
    int half = windowSize / 2;

    for (int y = 0; y < src.height; y++) {
        for (int x = 0; x < src.width; x++) {
            long long sum = 0;
            int count = 0;

            for (int dy = -half; dy <= half; dy++) {
                for (int dx = -half; dx <= half; dx++) {
                    int ny = y + dy;
                    int nx = x + dx;

                    if (ny >= 0 && ny < src.height && nx >= 0 && nx < src.width) {
                        sum += src.at(nx, ny);
                        count++;
                    }
                }
            }

            double localMean = (double)sum / count;
            int localThreshold = static_cast<int>(localMean - C);

            result.at(x, y) = (src.at(x, y) > localThreshold) ? 255 : 0;
        }
    }
    return result;
}