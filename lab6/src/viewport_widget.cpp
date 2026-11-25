#include "viewport_widget.h"

#include <QElapsedTimer>
#include <QImage>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>

namespace {

double niceStep(double raw) {
    const double p = std::pow(10.0, std::floor(std::log10(raw)));
    for (double m : {1.0, 2.0, 5.0, 10.0})
        if (m * p >= raw) return m * p;
    return 10 * p;
}

QString fmtTick(double v) {
    if (std::fabs(v) < 1e-9) v = 0;
    return QString::number(v, 'g', 6);
}

const QColor kAxisColor[3] = {QColor(205, 50, 50), QColor(40, 150, 60), QColor(40, 90, 210)};
const char* kAxisName[3] = {"X", "Y", "Z"};

}

ViewportWidget::ViewportWidget(Scene* scene, render::ViewKind kind, QWidget* parent)
    : QWidget(parent), scene_(scene), kind_(kind) {
    setMinimumSize(260, 200);
    setAttribute(Qt::WA_OpaquePaintEvent);
    if (kind_ == render::ViewKind::Perspective) setCursor(Qt::OpenHandCursor);
}

QString ViewportWidget::title() const {
    switch (kind_) {
        case render::ViewKind::Front: return "Вид спереди — проекция на Oxy";
        case render::ViewKind::Top: return "Вид сверху — проекция на Oxz";
        case render::ViewKind::Side: return "Вид слева — проекция на Oyz";
        default: return "Перспектива (LookAt + P·V·M)  ·  ЛКМ — облёт, колесо — приближение";
    }
}

void ViewportWidget::paintEvent(QPaintEvent*) {
    QElapsedTimer timer;
    timer.start();

    const double dpr = std::min(2.0, devicePixelRatioF());
    const int W = std::max(1, static_cast<int>(std::lround(width() * dpr)));
    const int H = std::max(1, static_cast<int>(std::lround(height() * dpr)));
    if (fb_.width != W || fb_.height != H) fb_.resize(W, H);
    else fb_.clear();

    const render::View v = kind_ == render::ViewKind::Perspective
                               ? scene_->camera.makeView(W, H)
                               : render::makeOrthoView(kind_, W, H, scene_->fitPoints());

    render::drawMesh(fb_, v, scene_->mesh, scene_->world, scene_->stats.det, scene_->options);

    if (scene_->showWorldAxes && kind_ == render::ViewKind::Perspective) {
        const double len = 140;
        render::drawLine3D(fb_, v, {0, 0, 0}, {len, 0, 0}, render::rgba(205, 50, 50), render::rgba(205, 50, 50, 70), 0, 2);
        render::drawLine3D(fb_, v, {0, 0, 0}, {0, len, 0}, render::rgba(40, 150, 60), render::rgba(40, 150, 60, 70), 0, 2);
        render::drawLine3D(fb_, v, {0, 0, 0}, {0, 0, len}, render::rgba(40, 90, 210), render::rgba(40, 90, 210, 70), 0, 2);
    }
    if (scene_->showAxisLine) {
        const m3d::Vec3 d = scene_->p2 - scene_->p1;
        render::drawLine3D(fb_, v, scene_->p1 - d * 0.6, scene_->p2 + d * 0.6, render::rgba(214, 30, 140),
                           render::rgba(214, 30, 140, 80), 9 * dpr, std::max(2, static_cast<int>(2 * dpr)));
    }

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), kind_ == render::ViewKind::Perspective ? QColor(246, 248, 251) : QColor(255, 255, 255));
    if (kind_ != render::ViewKind::Perspective) drawOrthoGrid(p, v, dpr);

    QImage img(reinterpret_cast<const uchar*>(fb_.color.data()), W, H, QImage::Format_ARGB32);
    img.setDevicePixelRatio(dpr);
    p.drawImage(QPointF(0, 0), img);

    drawOverlay(p, v, dpr);
    p.setPen(QColor(210, 214, 220));
    p.setBrush(Qt::NoBrush);
    p.drawRect(rect().adjusted(0, 0, -1, -1));
    lastRenderMs_ = timer.nsecsElapsed() / 1e6;
}

void ViewportWidget::drawOrthoGrid(QPainter& p, const render::View& v, double dpr) {
    const double ppu = v.pixelsPerUnit / dpr, ox = v.originX / dpr, oy = v.originY / dpr;
    const double step = niceStep(70.0 / ppu);
    const double w = width(), h = height();

    QFont small = font();
    small.setPointSizeF(std::max(7.0, small.pointSizeF() - 2));
    p.setFont(small);

    p.setPen(QPen(QColor(234, 236, 240), 1));
    for (long k = static_cast<long>(std::ceil(-ox / ppu / step)); k * step <= (w - ox) / ppu; ++k) {
        const double x = ox + k * step * ppu;
        p.drawLine(QPointF(x, 0), QPointF(x, h));
    }
    for (long k = static_cast<long>(std::ceil((oy - h) / ppu / step)); k * step <= oy / ppu; ++k) {
        const double y = oy - k * step * ppu;
        p.drawLine(QPointF(0, y), QPointF(w, y));
    }
    const QColor hc = kAxisColor[v.hAxis], vc = kAxisColor[v.vAxis];
    p.setPen(QPen(hc, 1.4));
    p.drawLine(QPointF(0, oy), QPointF(w, oy));
    p.setPen(QPen(vc, 1.4));
    p.drawLine(QPointF(ox, 0), QPointF(ox, h));

    p.setPen(QColor(110, 116, 128));
    for (long k = static_cast<long>(std::ceil(-ox / ppu / step)); k * step <= (w - ox) / ppu; ++k) {
        const double x = ox + k * step * ppu;
        if (k == 0 || x > w - 26 || x < 26) continue;
        p.drawLine(QPointF(x, oy - 3), QPointF(x, oy + 3));
        p.drawText(QPointF(x + 2, oy + 13), fmtTick(k * step * v.hSign));
    }
    for (long k = static_cast<long>(std::ceil((oy - h) / ppu / step)); k * step <= oy / ppu; ++k) {
        const double y = oy - k * step * ppu;
        if (k == 0 || y < 52 || y > h - 26) continue;
        p.drawLine(QPointF(ox - 3, y), QPointF(ox + 3, y));
        p.drawText(QPointF(ox + 5, y - 2), fmtTick(k * step * v.vSign));
    }
    p.drawText(QPointF(ox + 4, oy + 13), "0");

    QFont bold = font();
    bold.setBold(true);
    p.setFont(bold);
    auto arrow = [&](QPointF tip, QPointF dir, const QColor& c) {
        const QPointF n(-dir.y(), dir.x());
        QPainterPath path;
        path.moveTo(tip);
        path.lineTo(tip - dir * 9 + n * 4);
        path.lineTo(tip - dir * 9 - n * 4);
        path.closeSubpath();
        p.fillPath(path, c);
    };
    const bool hRight = v.hSign > 0;
    const QPointF hTip(hRight ? w - 4 : 4, oy);
    arrow(hTip, QPointF(hRight ? 1 : -1, 0), hc);
    p.setPen(hc);
    p.drawText(QPointF(hRight ? w - 18 : 8, oy - 8), kAxisName[v.hAxis]);
    const bool vUp = v.vSign > 0;
    const QPointF vTip(ox, vUp ? 4 : h - 4);
    arrow(vTip, QPointF(0, vUp ? -1 : 1), vc);
    p.setPen(vc);
    p.drawText(QPointF(ox + 8, vUp ? 46 : h - 10), kAxisName[v.vAxis]);

    p.setFont(small);
    p.setPen(QColor(130, 136, 148));
    p.drawText(QPointF(8, h - 8), QString("шаг сетки: %1").arg(fmtTick(step)));
}

void ViewportWidget::drawOverlay(QPainter& p, const render::View& v, double dpr) {
    auto proj = [&](m3d::Vec3 w, QPointF& out) {
        const render::ScreenPoint s = render::project(v, w);
        out = QPointF(s.x / dpr, s.y / dpr);
        return s.visible;
    };
    QFont bold = font();
    bold.setBold(true);
    p.setFont(bold);

    if (kind_ == render::ViewKind::Perspective && scene_->showWorldAxes) {
        const m3d::Vec3 tips[3] = {{150, 0, 0}, {0, 150, 0}, {0, 0, 150}};
        for (int i = 0; i < 3; ++i) {
            QPointF q;
            if (proj(tips[i], q)) {
                p.setPen(kAxisColor[i]);
                p.drawText(q + QPointF(-4, 4), kAxisName[i]);
            }
        }
    }
    if (scene_->showAxisLine) {
        const QColor c(214, 30, 140);
        const m3d::Vec3 pts[2] = {scene_->p1, scene_->p2};
        for (int i = 0; i < 2; ++i) {
            QPointF q;
            if (!proj(pts[i], q)) continue;
            p.setPen(QPen(Qt::white, 1.5));
            p.setBrush(c);
            p.drawEllipse(q, 4.5, 4.5);
            p.setPen(c);
            p.drawText(q + QPointF(7, -6), i == 0 ? "P1" : "P2");
        }
    }
    if (scene_->options.lighting) {
        QPointF q;
        if (proj(scene_->options.light, q) && rect().contains(q.toPoint())) {
            p.setPen(QPen(QColor(200, 140, 0), 1.5));
            p.setBrush(QColor(255, 210, 40));
            p.drawEllipse(q, 6, 6);
            for (int k = 0; k < 8; ++k) {
                const double a = k * m3d::kPi / 4;
                p.drawLine(q + QPointF(std::cos(a), std::sin(a)) * 8, q + QPointF(std::cos(a), std::sin(a)) * 12);
            }
        }
    }
    const QString t = title();
    QFont tf = font();
    tf.setBold(true);
    p.setFont(tf);
    const QRectF br = p.fontMetrics().boundingRect(t).adjusted(-6, -3, 6, 3);
    const QRectF box(8, 8, br.width(), br.height());
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255, 225));
    p.drawRoundedRect(box, 5, 5);
    p.setPen(QColor(45, 52, 66));
    p.drawText(box, Qt::AlignCenter, t);
}

void ViewportWidget::mousePressEvent(QMouseEvent* e) {
    if (kind_ == render::ViewKind::Perspective && e->button() == Qt::LeftButton) {
        dragging_ = true;
        lastMouse_ = e->position().toPoint();
        setCursor(Qt::ClosedHandCursor);
    }
}

void ViewportWidget::mouseMoveEvent(QMouseEvent* e) {
    if (!dragging_) return;
    const QPoint pos = e->position().toPoint();
    const QPoint d = pos - lastMouse_;
    lastMouse_ = pos;
    auto& cam = scene_->camera;
    cam.yawDeg -= d.x() * 0.4;
    cam.pitchDeg = std::clamp(cam.pitchDeg + d.y() * 0.4, -89.0, 89.0);
    update();
    emit cameraChanged();
}

void ViewportWidget::mouseReleaseEvent(QMouseEvent*) {
    if (!dragging_) return;
    dragging_ = false;
    setCursor(Qt::OpenHandCursor);
}

void ViewportWidget::wheelEvent(QWheelEvent* e) {
    if (kind_ != render::ViewKind::Perspective) return;
    auto& cam = scene_->camera;
    const QPoint ad = e->angleDelta();
    const double steps = (ad.y() != 0 ? ad.y() : ad.x()) / 120.0;
    if (e->modifiers() & Qt::ShiftModifier)
        cam.fovDeg = std::clamp(cam.fovDeg - steps * 3.0, 10.0, 120.0);
    else
        cam.distance = std::clamp(cam.distance * std::pow(0.88, steps), 40.0, 4000.0);
    update();
    emit cameraChanged();
    e->accept();
}
