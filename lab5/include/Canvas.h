#pragma once
#include <QWidget>
#include <QImage>
#include <QColor>
#include <QString>
#include <vector>
#include <optional>
#include "Clipper.h"

enum class ClipMode { Midpoint = 0, CyrusBeck = 1 };

class Canvas : public QWidget {
    Q_OBJECT
public:
    explicit Canvas(QWidget* parent = nullptr);

    void setScene(ClipMode mode, const Rect& rect, const std::vector<Point>& poly,
                  const std::vector<Segment>& segs, const std::vector<std::optional<Segment>>& clipped);
    void clear();

signals:
    void cursorMoved(double x, double y);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    ClipMode mode = ClipMode::Midpoint;
    Rect clipRect{-100, -100, 100, 100};
    std::vector<Point> polygon;
    std::vector<Segment> segments;
    std::vector<std::optional<Segment>> clippedSegments;
    bool hasData = false;

    // Преобразование мировых координат в экранные (единый масштаб по обеим осям)
    double scale = 1.0, offsetX = 0.0, offsetY = 0.0;
    void updateTransform();
    QPointF toScreen(const Point& p) const;
    Point toWorld(const QPointF& p) const;

    void drawAxes(QImage& img);
    void drawLegend(QImage& img);
    // Растеризация отрезка алгоритмом Брезенхэма; width — толщина в пикселях
    void drawLine(QImage& img, const QPointF& a, const QPointF& b, const QColor& color, int width = 1, bool dashed = false);
    void drawPolygon(QImage& img, const std::vector<Point>& poly, const QColor& color, int width);
    void setPixel(QImage& img, int x, int y, const QColor& color);
};
