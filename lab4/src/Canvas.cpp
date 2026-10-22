#include "Canvas.h"
#include <QPainter>
#include <QPaintEvent>

Canvas::Canvas(QWidget* parent) : QWidget(parent) {
    setMinimumSize(800, 600);
    setStyleSheet("background-color: white;");

    image = QImage(800, 600, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
}

void Canvas::clear() {
    image.fill(Qt::transparent);
    update();
}

void Canvas::drawLine(int x1, int y1, int x2, int y2, int algorithm) {
    std::vector<Point> points;

    switch (algorithm) {
        case 0: points = Rasterizer::stepAlgorithm(x1, y1, x2, y2); break;
        case 1: points = Rasterizer::ddaAlgorithm(x1, y1, x2, y2); break;
        case 2: points = Rasterizer::bresenhamLine(x1, y1, x2, y2); break;
        default: return;
    }

    for (const auto& p : points) {
        drawPixel(p.x, p.y, Qt::black);
    }

    update();
}

void Canvas::drawCircle(int cx, int cy, int radius) {
    std::vector<Point> points = Rasterizer::bresenhamCircle(cx, cy, radius);

    for (const auto& p : points) {
        drawPixel(p.x, p.y, Qt::red);
    }

    update();
}

void Canvas::drawPixel(int x, int y, QColor color) {
    int currentOffsetX = width() / 2;
    int currentOffsetY = height() / 2;

    int screenX = currentOffsetX + x * gridSize;
    int screenY = currentOffsetY - y * gridSize;

    QPainter painter(&image);
    painter.fillRect(screenX - gridSize / 2, screenY - gridSize / 2,
                     gridSize, gridSize, color);
}

void Canvas::paintEvent(QPaintEvent* event) {
    if (image.width() != width() || image.height() != height()) {
        QImage newImage(width(), height(), QImage::Format_ARGB32);
        newImage.fill(Qt::transparent);
        image = newImage;
    }

    offsetX = width() / 2;
    offsetY = height() / 2;

    QPainter painter(this);

    // Сетка
    painter.setPen(QPen(QColor(220, 220, 220), 1));
    for (int x = offsetX; x < width(); x += gridSize) painter.drawLine(x, 0, x, height());
    for (int x = offsetX; x > 0; x -= gridSize) painter.drawLine(x, 0, x, height());
    for (int y = offsetY; y < height(); y += gridSize) painter.drawLine(0, y, width(), y);
    for (int y = offsetY; y > 0; y -= gridSize) painter.drawLine(0, y, width(), y);

    // Оси
    painter.setPen(QPen(Qt::black, 2));
    painter.drawLine(0, offsetY, width(), offsetY);
    painter.drawLine(offsetX, 0, offsetX, height());

    // Подписи
    painter.setPen(QPen(Qt::black, 1));
    painter.setFont(QFont("Arial", 10));
    painter.drawText(width() - 20, offsetY - 10, "X");
    painter.drawText(offsetX + 10, 20, "Y");
    painter.drawText(offsetX - 15, offsetY + 15, "0");

    // Рисунок
    painter.drawImage(0, 0, image);
}