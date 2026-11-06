#include "Canvas.h"
#include <QPainter>
#include <QMouseEvent>
#include <QFont>
#include <algorithm>
#include <cmath>

namespace {
const QColor COLOR_BG       (255, 255, 255);
const QColor COLOR_GRID     (236, 238, 242);
const QColor COLOR_AXIS     ( 60,  60,  70);
const QColor COLOR_TEXT     ( 90,  90, 100);
const QColor COLOR_WINDOW   ( 35,  95, 200);   // отсекающее окно / многоугольник
const QColor COLOR_SEGMENT  (225, 120,  60);   // исходные отрезки
const QColor COLOR_VISIBLE  ( 20, 150,  80);   // видимые части

// "Красивый" шаг сетки: 1, 2 или 5 · 10^k
double niceStep(double raw) {
    double p = std::pow(10.0, std::floor(std::log10(raw)));
    double f = raw / p;
    if (f < 1.5) return p;
    if (f < 3.5) return 2 * p;
    if (f < 7.5) return 5 * p;
    return 10 * p;
}

QString formatNumber(double v) {
    if (std::abs(v) < 1e-9) v = 0;
    return QString::number(v, 'g', 6);
}
}

Canvas::Canvas(QWidget* parent) : QWidget(parent) {
    setMinimumSize(500, 400);
    setMouseTracking(true);
}

void Canvas::clear() {
    segments.clear(); clippedSegments.clear(); polygon.clear();
    hasData = false;
    update();
}

void Canvas::setScene(ClipMode m, const Rect& rect, const std::vector<Point>& poly,
                      const std::vector<Segment>& segs, const std::vector<std::optional<Segment>>& clipped) {
    mode = m; clipRect = rect; polygon = poly; segments = segs; clippedSegments = clipped;
    hasData = true;
    update();
}

// ---------------------------------------------------------------------------
// Масштаб подбирается автоматически так, чтобы поместились все отрезки, окно и начало координат.
// Масштаб одинаков по обеим осям, поэтому фигуры не искажаются.
// ---------------------------------------------------------------------------
void Canvas::updateTransform() {
    const double dpr = devicePixelRatioF();
    const double w = width() * dpr, h = height() * dpr, margin = 40 * dpr;

    double minX = 0, maxX = 0, minY = 0, maxY = 0;
    auto include = [&](const Point& p) {
        minX = std::min(minX, p.x); maxX = std::max(maxX, p.x);
        minY = std::min(minY, p.y); maxY = std::max(maxY, p.y);
    };
    for (const auto& s : segments) { include(s.p1); include(s.p2); }
    if (mode == ClipMode::Midpoint) { include({clipRect.xmin, clipRect.ymin}); include({clipRect.xmax, clipRect.ymax}); }
    else for (const auto& p : polygon) include(p);
    if (!hasData) { minX = minY = -300; maxX = maxY = 300; }

    double rangeX = std::max(maxX - minX, 1e-6), rangeY = std::max(maxY - minY, 1e-6);
    scale = std::min((w - 2 * margin) / rangeX, (h - 2 * margin) / rangeY);
    offsetX = w / 2.0 - (minX + maxX) / 2.0 * scale;
    offsetY = h / 2.0 + (minY + maxY) / 2.0 * scale;
}

QPointF Canvas::toScreen(const Point& p) const { return {offsetX + p.x * scale, offsetY - p.y * scale}; }
Point Canvas::toWorld(const QPointF& p) const { return {(p.x() - offsetX) / scale, (offsetY - p.y()) / scale}; }

void Canvas::setPixel(QImage& img, int x, int y, const QColor& color) {
    if (x >= 0 && x < img.width() && y >= 0 && y < img.height()) img.setPixelColor(x, y, color);
}

// Алгоритм Брезенхэма (целочисленный, для всех октантов)
void Canvas::drawLine(QImage& img, const QPointF& a, const QPointF& b, const QColor& color, int width, bool dashed) {
    // Отрезки далеко за экраном не растеризуем попиксельно (защита от зависания при огромных координатах)
    const double lim = 1e5;
    if (std::abs(a.x()) > lim || std::abs(a.y()) > lim || std::abs(b.x()) > lim || std::abs(b.y()) > lim) return;

    int x1 = (int)std::lround(a.x()), y1 = (int)std::lround(a.y());
    int x2 = (int)std::lround(b.x()), y2 = (int)std::lround(b.y());
    int dx = std::abs(x2 - x1), dy = std::abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1, sy = y1 < y2 ? 1 : -1;
    int err = dx - dy;
    int r0 = -(width - 1) / 2, r1 = width / 2;
    const double dpr = devicePixelRatioF();
    const int dashLen = std::max(1, (int)std::lround(6 * dpr));

    for (int step = 0;; ++step) {
        if (!dashed || (step / dashLen) % 2 == 0)
            for (int ox = r0; ox <= r1; ++ox)
                for (int oy = r0; oy <= r1; ++oy)
                    setPixel(img, x1 + ox, y1 + oy, color);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 <  dx) { err += dx; y1 += sy; }
    }
}

void Canvas::drawPolygon(QImage& img, const std::vector<Point>& poly, const QColor& color, int width) {
    for (size_t i = 0; i < poly.size(); ++i)
        drawLine(img, toScreen(poly[i]), toScreen(poly[(i + 1) % poly.size()]), color, width);
}

// ---------------------------------------------------------------------------
// Система координат: сетка, оси со стрелками, засечки и подписи
// ---------------------------------------------------------------------------
void Canvas::drawAxes(QImage& img) {
    const double dpr = devicePixelRatioF();
    const int w = img.width(), h = img.height();
    const double step = niceStep(70 * dpr / scale);           // шаг сетки ≈ 70 логических пикселей

    Point tl = toWorld({0, 0}), br = toWorld({(double)w, (double)h});
    QPointF origin = toScreen({0, 0});
    int ox = (int)std::lround(origin.x()), oy = (int)std::lround(origin.y());

    // сетка
    for (double x = std::ceil(tl.x / step) * step; x <= br.x; x += step) {
        int sx = (int)std::lround(toScreen({x, 0}).x());
        drawLine(img, QPointF(sx, 0), QPointF(sx, h - 1), COLOR_GRID);
    }
    for (double y = std::ceil(br.y / step) * step; y <= tl.y; y += step) {
        int sy = (int)std::lround(toScreen({0, y}).y());
        drawLine(img, QPointF(0, sy), QPointF(w - 1, sy), COLOR_GRID);
    }

    // оси
    const int t = (int)std::lround(4 * dpr);
    drawLine(img, QPointF(0, oy), QPointF(w - 1, oy), COLOR_AXIS);
    drawLine(img, QPointF(ox, 0), QPointF(ox, h - 1), COLOR_AXIS);
    const int arrow = (int)std::lround(8 * dpr);
    drawLine(img, QPointF(w - 1, oy), QPointF(w - 1 - arrow, oy - arrow / 2), COLOR_AXIS);
    drawLine(img, QPointF(w - 1, oy), QPointF(w - 1 - arrow, oy + arrow / 2), COLOR_AXIS);
    drawLine(img, QPointF(ox, 0), QPointF(ox - arrow / 2, arrow), COLOR_AXIS);
    drawLine(img, QPointF(ox, 0), QPointF(ox + arrow / 2, arrow), COLOR_AXIS);

    // засечки
    for (double x = std::ceil(tl.x / step) * step; x <= br.x; x += step) {
        int sx = (int)std::lround(toScreen({x, 0}).x());
        if (std::abs(x) >= step / 2 && sx <= w - 3 * arrow) drawLine(img, QPointF(sx, oy - t), QPointF(sx, oy + t), COLOR_AXIS);
    }
    for (double y = std::ceil(br.y / step) * step; y <= tl.y; y += step) {
        int sy = (int)std::lround(toScreen({0, y}).y());
        if (std::abs(y) >= step / 2 && sy >= 3 * arrow) drawLine(img, QPointF(ox - t, sy), QPointF(ox + t, sy), COLOR_AXIS);
    }

    // подписи (QPainter создаётся только после попиксельного рисования)
    QPainter p(&img);
    p.setRenderHint(QPainter::TextAntialiasing);
    QFont f = p.font(); f.setPixelSize((int)std::lround(11 * dpr)); p.setFont(f);
    p.setPen(COLOR_TEXT);
    const int pad = (int)std::lround(3 * dpr);

    for (double x = std::ceil(tl.x / step) * step; x <= br.x; x += step) {
        if (std::abs(x) < step / 2) continue;
        int sx = (int)std::lround(toScreen({x, 0}).x());
        if (sx > w - 3 * arrow) continue;
        p.drawText(QRectF(sx - 40 * dpr, oy + t + pad, 80 * dpr, 14 * dpr), Qt::AlignHCenter | Qt::AlignTop, formatNumber(x));
    }
    for (double y = std::ceil(br.y / step) * step; y <= tl.y; y += step) {
        if (std::abs(y) < step / 2) continue;
        int sy = (int)std::lround(toScreen({0, y}).y());
        if (sy < 3 * arrow) continue;
        p.drawText(QRectF(ox - t - pad - 80 * dpr, sy - 7 * dpr, 80 * dpr, 14 * dpr), Qt::AlignRight | Qt::AlignVCenter, formatNumber(y));
    }
    p.drawText(QPointF(ox - 12 * dpr, oy + 14 * dpr), "0");
    f.setBold(true); f.setPixelSize((int)std::lround(13 * dpr)); p.setFont(f); p.setPen(COLOR_AXIS);
    p.drawText(QPointF(w - 14 * dpr, oy - 10 * dpr), "X");
    p.drawText(QPointF(ox + 10 * dpr, 16 * dpr), "Y");
}

void Canvas::drawLegend(QImage& img) {
    const double dpr = devicePixelRatioF();
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    QFont f = p.font(); f.setPixelSize((int)std::lround(12 * dpr)); p.setFont(f);

    const QString windowName = mode == ClipMode::Midpoint ? "Отсекающее окно" : "Отсекающий многоугольник";
    const std::vector<std::pair<QColor, QString>> items = {
        {COLOR_WINDOW, windowName},
        {COLOR_SEGMENT, "Исходные отрезки"},
        {COLOR_VISIBLE, "Видимые части"},
    };
    const double x = 12 * dpr, y = 12 * dpr, lineH = 20 * dpr, boxW = 215 * dpr;
    QRectF box(x, y, boxW, lineH * items.size() + 10 * dpr);
    p.setPen(QPen(QColor(210, 210, 215), dpr));
    p.setBrush(QColor(255, 255, 255, 230));
    p.drawRoundedRect(box, 6 * dpr, 6 * dpr);

    for (size_t i = 0; i < items.size(); ++i) {
        double cy = y + 5 * dpr + lineH * i + lineH / 2;
        p.setPen(QPen(items[i].first, 3 * dpr));
        p.drawLine(QPointF(x + 10 * dpr, cy), QPointF(x + 34 * dpr, cy));
        p.setPen(COLOR_TEXT);
        p.drawText(QPointF(x + 44 * dpr, cy + 4 * dpr), items[i].second);
    }
}

void Canvas::paintEvent(QPaintEvent*) {
    const double dpr = devicePixelRatioF();   // Retina-экраны на Mac: рисуем в реальных пикселях
    QImage img((int)std::lround(width() * dpr), (int)std::lround(height() * dpr), QImage::Format_ARGB32_Premultiplied);
    img.fill(COLOR_BG);
    updateTransform();
    drawAxes(img);

    const int thin = std::max(1, (int)std::lround(dpr));

    // 1. Исходные отрезки (пунктир — чтобы было видно, какая часть отброшена)
    for (const auto& s : segments)
        drawLine(img, toScreen(s.p1), toScreen(s.p2), COLOR_SEGMENT, thin, true);

    // 2. Отсекающее окно
    if (mode == ClipMode::Midpoint) {
        std::vector<Point> r = {{clipRect.xmin, clipRect.ymin}, {clipRect.xmax, clipRect.ymin},
                                {clipRect.xmax, clipRect.ymax}, {clipRect.xmin, clipRect.ymax}};
        drawPolygon(img, r, COLOR_WINDOW, 2 * thin);
    } else {
        drawPolygon(img, polygon, COLOR_WINDOW, 2 * thin);
    }

    // 3. Видимые части + точки пересечения с границей
    for (const auto& c : clippedSegments) {
        if (!c) continue;
        drawLine(img, toScreen(c->p1), toScreen(c->p2), COLOR_VISIBLE, 3 * thin);
        for (const Point& e : {c->p1, c->p2}) {
            QPointF q = toScreen(e);
            int r = 3 * thin;
            for (int dx = -r; dx <= r; ++dx)
                for (int dy = -r; dy <= r; ++dy)
                    if (dx * dx + dy * dy <= r * r) setPixel(img, (int)std::lround(q.x()) + dx, (int)std::lround(q.y()) + dy, COLOR_VISIBLE.darker(140));
        }
    }

    drawLegend(img);

    img.setDevicePixelRatio(dpr);
    QPainter painter(this);
    painter.drawImage(0, 0, img);
}

void Canvas::mouseMoveEvent(QMouseEvent* event) {
    Point w = toWorld(event->position() * devicePixelRatioF());
    emit cursorMoved(w.x, w.y);
}
