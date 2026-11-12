#include "canvas_widget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>
#include <limits>

namespace {

const QColor kWindowFill(31, 111, 235, 40);
const QColor kWindowBorder(31, 111, 235);
const QColor kOriginal(214, 69, 80);
const QColor kVisible(22, 150, 70);
const QColor kAxis(40, 40, 40);
const QColor kGridMinor(236, 236, 236);
const QColor kGridMajor(215, 215, 215);

double niceStep(double raw) {
    const double p = std::pow(10.0, std::floor(std::log10(raw)));
    const double m = raw / p;
    if (m < 1.5) return p;
    if (m < 3.5) return 2 * p;
    if (m < 7.5) return 5 * p;
    return 10 * p;
}

QString formatNumber(double v, double step) {
    if (std::fabs(v) < step * 1e-6) v = 0.0;
    const int decimals = step >= 1 ? 0 : static_cast<int>(std::ceil(-std::log10(step)));
    return QString::number(v, 'f', decimals);
}

}

CanvasWidget::CanvasWidget(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true);
    setMinimumSize(400, 300);
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::white);
    setPalette(pal);
}

void CanvasWidget::setScene(const io::InputData& data, const std::vector<clip::ClipResult>& results) {
    data_ = data;
    results_ = results;
    hasData_ = true;
    fitToScene();
}

void CanvasWidget::setShowOriginal(bool on) {
    showOriginal_ = on;
    update();
}

void CanvasWidget::setShowGrid(bool on) {
    showGrid_ = on;
    update();
}

void CanvasWidget::fitToScene() {
    autoFit_ = true;
    double minX = std::numeric_limits<double>::max(), minY = minX;
    double maxX = std::numeric_limits<double>::lowest(), maxY = maxX;
    auto extend = [&](geom::Point p) {
        minX = std::min(minX, p.x);
        maxX = std::max(maxX, p.x);
        minY = std::min(minY, p.y);
        maxY = std::max(maxY, p.y);
    };
    extend({0, 0});
    if (hasData_) {
        for (const auto& s : data_.segments) {
            extend(s.a);
            extend(s.b);
        }
        if (data_.kind == io::WindowKind::Rectangle) {
            for (const auto& v : geom::rectToPolygon(data_.rect)) extend(v);
        } else {
            for (const auto& v : data_.polygon) extend(v);
        }
    }
    const double w = std::max(maxX - minX, 1e-6);
    const double h = std::max(maxY - minY, 1e-6);
    centerX_ = (minX + maxX) / 2;
    centerY_ = (minY + maxY) / 2;
    const double margin = 0.85;
    scale_ = std::min(width() * margin / w, height() * margin / h);
    if (!std::isfinite(scale_) || scale_ <= 0) scale_ = 40;
    update();
}

QPointF CanvasWidget::toScreen(geom::Point p) const {
    return {width() / 2.0 + (p.x - centerX_) * scale_, height() / 2.0 - (p.y - centerY_) * scale_};
}

geom::Point CanvasWidget::toWorld(QPointF s) const {
    return {centerX_ + (s.x() - width() / 2.0) / scale_, centerY_ - (s.y() - height() / 2.0) / scale_};
}

void CanvasWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    if (showGrid_) drawGrid(p);
    drawAxes(p);
    if (hasData_) {
        drawWindow(p);
        drawSegments(p);
    }
    drawLegend(p);
}

void CanvasWidget::drawGrid(QPainter& p) const {
    const double step = niceStep(60.0 / scale_);
    const geom::Point tl = toWorld({0, 0});
    const geom::Point br = toWorld(QPointF(width(), height()));

    for (int pass = 0; pass < 2; ++pass) {
        const double st = pass == 0 ? step / 5 : step;
        if (st * scale_ < 8) continue;
        p.setPen(QPen(pass == 0 ? kGridMinor : kGridMajor, 1));
        for (double x = std::floor(tl.x / st) * st; x <= br.x; x += st) {
            const double sx = toScreen(x, 0).x();
            p.drawLine(QPointF(sx, 0), QPointF(sx, height()));
        }
        for (double y = std::floor(br.y / st) * st; y <= tl.y; y += st) {
            const double sy = toScreen(0, y).y();
            p.drawLine(QPointF(0, sy), QPointF(width(), sy));
        }
    }
}

void CanvasWidget::drawAxes(QPainter& p) const {
    const double step = niceStep(60.0 / scale_);
    const QPointF origin = toScreen(0, 0);
    const double ox = std::clamp(origin.x(), 0.0, static_cast<double>(width()));
    const double oy = std::clamp(origin.y(), 0.0, static_cast<double>(height()));

    p.setPen(QPen(kAxis, 1.5));
    p.drawLine(QPointF(0, oy), QPointF(width(), oy));
    p.drawLine(QPointF(ox, 0), QPointF(ox, height()));

    QPainterPath arrows;
    arrows.moveTo(width() - 1, oy);
    arrows.lineTo(width() - 11, oy - 5);
    arrows.lineTo(width() - 11, oy + 5);
    arrows.closeSubpath();
    arrows.moveTo(ox, 1);
    arrows.lineTo(ox - 5, 11);
    arrows.lineTo(ox + 5, 11);
    arrows.closeSubpath();
    p.fillPath(arrows, kAxis);

    QFont f = font();
    f.setPointSizeF(9);
    p.setFont(f);
    p.drawText(QPointF(width() - 16, oy - 10), "X");
    p.drawText(QPointF(ox + 10, 14), "Y");

    const geom::Point tl = toWorld({0, 0});
    const geom::Point br = toWorld(QPointF(width(), height()));
    p.setPen(QPen(QColor(90, 90, 90), 1));
    for (double x = std::ceil(tl.x / step) * step; x <= br.x; x += step) {
        if (std::fabs(x) < step / 2) continue;
        const double sx = toScreen(x, 0).x();
        p.drawLine(QPointF(sx, oy - 3), QPointF(sx, oy + 3));
        p.drawText(QRectF(sx - 30, oy + 4, 60, 14), Qt::AlignHCenter | Qt::AlignTop, formatNumber(x, step));
    }
    for (double y = std::ceil(br.y / step) * step; y <= tl.y; y += step) {
        if (std::fabs(y) < step / 2) continue;
        const double sy = toScreen(0, y).y();
        p.drawLine(QPointF(ox - 3, sy), QPointF(ox + 3, sy));
        p.drawText(QRectF(ox - 64, sy - 7, 58, 14), Qt::AlignRight | Qt::AlignVCenter, formatNumber(y, step));
    }
    p.drawText(QRectF(ox - 20, oy + 4, 16, 14), Qt::AlignRight | Qt::AlignTop, "0");
}

void CanvasWidget::drawWindow(QPainter& p) const {
    const geom::Polygon poly = data_.kind == io::WindowKind::Rectangle ? geom::rectToPolygon(data_.rect)
                                                                      : data_.polygon;
    QPolygonF q;
    for (const auto& v : poly) q << toScreen(v);
    p.setPen(QPen(kWindowBorder, 2));
    p.setBrush(kWindowFill);
    p.drawPolygon(q);
    p.setBrush(Qt::NoBrush);
}

void CanvasWidget::drawSegments(QPainter& p) const {
    if (showOriginal_) {
        QPen pen(kOriginal, 1.5, Qt::DashLine);
        p.setPen(pen);
        for (const auto& s : data_.segments) {
            p.drawLine(toScreen(s.a), toScreen(s.b));
        }
        p.setPen(Qt::NoPen);
        p.setBrush(kOriginal);
        for (const auto& s : data_.segments) {
            p.drawEllipse(toScreen(s.a), 2.5, 2.5);
            p.drawEllipse(toScreen(s.b), 2.5, 2.5);
        }
    }
    p.setPen(QPen(kVisible, 3.5, Qt::SolidLine, Qt::RoundCap));
    for (const auto& r : results_) {
        if (!r.visible) continue;
        p.drawLine(toScreen(r.visible->a), toScreen(r.visible->b));
    }
    p.setPen(QPen(Qt::white, 1.5));
    p.setBrush(kVisible);
    for (const auto& r : results_) {
        if (!r.visible) continue;
        p.drawEllipse(toScreen(r.visible->a), 4, 4);
        p.drawEllipse(toScreen(r.visible->b), 4, 4);
    }
    p.setBrush(Qt::NoBrush);
}

void CanvasWidget::drawLegend(QPainter& p) const {
    const QRectF box(12, 12, 190, 78);
    p.setPen(QPen(QColor(200, 200, 200)));
    p.setBrush(QColor(255, 255, 255, 230));
    p.drawRoundedRect(box, 6, 6);

    QFont f = font();
    f.setPointSizeF(9.5);
    p.setFont(f);
    auto row = [&](int i, const QPen& pen, const QString& text) {
        const double y = box.top() + 18 + i * 21;
        p.setPen(pen);
        p.drawLine(QPointF(box.left() + 12, y), QPointF(box.left() + 42, y));
        p.setPen(QColor(40, 40, 40));
        p.drawText(QPointF(box.left() + 52, y + 4), text);
    };
    row(0, QPen(kWindowBorder, 2), "Отсекающее окно");
    row(1, QPen(kOriginal, 1.5, Qt::DashLine), "Исходные отрезки");
    row(2, QPen(kVisible, 3.5), "Видимые части");
}

void CanvasWidget::wheelEvent(QWheelEvent* event) {
    const QPointF pos = event->position();
    const geom::Point before = toWorld(pos);
    const double factor = std::pow(1.0015, event->angleDelta().y());
    scale_ = std::clamp(scale_ * factor, 1e-3, 1e7);
    const geom::Point after = toWorld(pos);
    centerX_ += before.x - after.x;
    centerY_ += before.y - after.y;
    autoFit_ = false;
    update();
}

void CanvasWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        dragging_ = true;
        lastMouse_ = event->position();
        setCursor(Qt::ClosedHandCursor);
    }
}

void CanvasWidget::mouseMoveEvent(QMouseEvent* event) {
    const QPointF pos = event->position();
    if (dragging_) {
        const QPointF d = pos - lastMouse_;
        centerX_ -= d.x() / scale_;
        centerY_ += d.y() / scale_;
        lastMouse_ = pos;
        autoFit_ = false;
        update();
    }
    const geom::Point w = toWorld(pos);
    emit mouseWorldPosition(QPointF(w.x, w.y));
}

void CanvasWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        dragging_ = false;
        unsetCursor();
    }
}

void CanvasWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (autoFit_) fitToScene();
    else update();
}
