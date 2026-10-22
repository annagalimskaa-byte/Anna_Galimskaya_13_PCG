#pragma once
#include <QWidget>
#include <QImage>
#include <vector>
#include "Rasterizer.h"

class Canvas : public QWidget {
    Q_OBJECT

public:
    explicit Canvas(QWidget* parent = nullptr);

    void clear();
    void drawLine(int x1, int y1, int x2, int y2, int algorithm);
    void drawCircle(int cx, int cy, int radius);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QImage image;
    int gridSize = 20;
    int offsetX = 0;
    int offsetY = 0;

    void drawGrid(QPainter& painter);
    void drawAxes(QPainter& painter);
    void drawPixel(int x, int y, QColor color);
};