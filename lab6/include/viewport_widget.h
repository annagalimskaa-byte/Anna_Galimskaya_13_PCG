#pragma once

#include <QPoint>
#include <QWidget>

#include "renderer.h"
#include "scene.h"

class QPainter;

class ViewportWidget : public QWidget {
    Q_OBJECT

public:
    ViewportWidget(Scene* scene, render::ViewKind kind, QWidget* parent = nullptr);

    double lastRenderMs() const { return lastRenderMs_; }

signals:
    void cameraChanged();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    QString title() const;
    void drawOrthoGrid(QPainter& p, const render::View& v, double dpr);
    void drawOverlay(QPainter& p, const render::View& v, double dpr);

    Scene* scene_;
    render::ViewKind kind_;
    render::Framebuffer fb_;
    QPoint lastMouse_;
    bool dragging_ = false;
    double lastRenderMs_ = 0;
};
