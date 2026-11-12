#include "input_parser.h"

#include <cmath>
#include <fstream>
#include <sstream>

namespace io {

namespace {

struct Line {
    int number;
    std::vector<double> values;
};

std::string stripComments(const std::string& raw) {
    std::string out;
    bool inStars = false;
    for (char c : raw) {
        if (c == '#' && !inStars) break;
        if (c == '*') {
            inStars = !inStars;
            continue;
        }
        if (!inStars) out += c;
    }
    return out;
}

std::vector<Line> tokenize(const std::string& text) {
    std::vector<Line> lines;
    std::istringstream in(text);
    std::string raw;
    int number = 0;
    while (std::getline(in, raw)) {
        ++number;
        std::string s = stripComments(raw);
        for (char& c : s) {
            if (c == ',' || c == ';' || c == '\t' || c == '\r') c = ' ';
        }
        std::istringstream ls(s);
        Line line{number, {}};
        std::string tok;
        while (ls >> tok) {
            try {
                std::size_t pos = 0;
                const double v = std::stod(tok, &pos);
                if (pos != tok.size() || !std::isfinite(v)) throw std::invalid_argument(tok);
                line.values.push_back(v);
            } catch (const std::exception&) {
                throw ParseError("Строка " + std::to_string(number) +
                                 ": «" + tok + "» не является числом");
            }
        }
        if (!line.values.empty()) lines.push_back(std::move(line));
    }
    return lines;
}

int readCount(const Line& line, const char* what, int minValue) {
    if (line.values.size() != 1) {
        throw ParseError("Строка " + std::to_string(line.number) + ": ожидалось одно число — " + what);
    }
    const double v = line.values[0];
    if (v != std::floor(v) || v < minValue || v > 1e6) {
        throw ParseError("Строка " + std::to_string(line.number) + ": " + what +
                         " должно быть целым числом не меньше " + std::to_string(minValue));
    }
    return static_cast<int>(v);
}

void expectValues(const Line& line, std::size_t count, const std::string& what) {
    if (line.values.size() != count) {
        throw ParseError("Строка " + std::to_string(line.number) + ": для " + what +
                         " нужно " + std::to_string(count) + " числа, найдено " +
                         std::to_string(line.values.size()));
    }
}

}

InputData parseInput(const std::string& text) {
    const std::vector<Line> lines = tokenize(text);
    if (lines.empty()) throw ParseError("Файл пуст");

    InputData data;
    std::size_t idx = 0;

    const int n = readCount(lines[idx++], "число отрезков", 0);
    if (lines.size() < idx + n) {
        throw ParseError("Указано " + std::to_string(n) + " отрезков, но в файле только " +
                         std::to_string(lines.size() - idx) + " строк после первой");
    }
    for (int i = 0; i < n; ++i) {
        const Line& l = lines[idx++];
        expectValues(l, 4, "отрезка №" + std::to_string(i + 1));
        data.segments.push_back({{l.values[0], l.values[1]}, {l.values[2], l.values[3]}});
    }

    const std::size_t rest = lines.size() - idx;
    if (rest == 0) throw ParseError("Не задано отсекающее окно (последняя строка файла)");

    if (rest == 1 && lines[idx].values.size() == 4) {
        const Line& l = lines[idx];
        data.kind = WindowKind::Rectangle;
        data.rect = {l.values[0], l.values[1], l.values[2], l.values[3]};
        if (!(data.rect.xmin < data.rect.xmax) || !(data.rect.ymin < data.rect.ymax)) {
            throw ParseError("Строка " + std::to_string(l.number) +
                             ": для окна должно выполняться Xmin < Xmax и Ymin < Ymax");
        }
        return data;
    }

    const int m = readCount(lines[idx++], "число вершин многоугольника", 3);
    if (lines.size() - idx != static_cast<std::size_t>(m)) {
        throw ParseError("Ожидалось " + std::to_string(m) + " строк с вершинами окна, найдено " +
                         std::to_string(lines.size() - idx));
    }
    data.kind = WindowKind::ConvexPolygon;
    for (int i = 0; i < m; ++i) {
        const Line& l = lines[idx++];
        expectValues(l, 2, "вершины №" + std::to_string(i + 1));
        data.polygon.push_back({l.values[0], l.values[1]});
    }
    if (!geom::isConvex(data.polygon)) {
        throw ParseError("Отсекающий многоугольник не является выпуклым "
                         "(или вершины перечислены не в порядке обхода)");
    }
    return data;
}

InputData loadInputFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw ParseError("Не удалось открыть файл: " + path);
    std::ostringstream ss;
    ss << in.rdbuf();
    return parseInput(ss.str());
}

std::vector<clip::ClipResult> clipAll(const InputData& data) {
    std::vector<clip::ClipResult> results;
    results.reserve(data.segments.size());
    for (const auto& s : data.segments) {
        results.push_back(data.kind == WindowKind::Rectangle
                              ? clip::midpointClip(s, data.rect)
                              : clip::cyrusBeckClip(s, data.polygon));
    }
    return results;
}

const char* algorithmName(WindowKind kind) {
    return kind == WindowKind::Rectangle ? "Алгоритм средней точки"
                                         : "Алгоритм Кируса–Бека";
}

}
