#pragma once

#include <QPointF>
#include <QWidget>
#include <vector>

#include "clipping.h"
#include "input_parser.h"

class CanvasWidget : public QWidget {
    Q_OBJECT

public:
    explicit CanvasWidget(QWidget* parent = nullptr);

    void setScene(const io::InputData& data, const std::vector<clip::ClipResult>& results);
    void setShowOriginal(bool on);
    void setShowGrid(bool on);

public slots:
    void fitToScene();

signals:
    void mouseWorldPosition(QPointF p);

protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    QPointF toScreen(geom::Point p) const;
    QPointF toScreen(double x, double y) const { return toScreen(geom::Point{x, y}); }
    geom::Point toWorld(QPointF s) const;

    void drawGrid(QPainter& p) const;
    void drawAxes(QPainter& p) const;
    void drawWindow(QPainter& p) const;
    void drawSegments(QPainter& p) const;
    void drawLegend(QPainter& p) const;

    io::InputData data_;
    std::vector<clip::ClipResult> results_;
    bool hasData_ = false;
    bool showOriginal_ = true;
    bool showGrid_ = true;

    double centerX_ = 0.0;
    double centerY_ = 0.0;
    double scale_ = 40.0;

    bool autoFit_ = true;
    bool dragging_ = false;
    QPointF lastMouse_;
};
