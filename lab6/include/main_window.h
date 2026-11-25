#pragma once

#include <QElapsedTimer>
#include <QMainWindow>

#include "scene.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QListWidget;
class QSpinBox;
class QTableWidget;
class QTimer;
class ViewportWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void generate();
    void randomSeed();
    void applyTranslation();
    void applyScaling();
    void applyAxisRotation();
    void applyLineRotation();
    void resetTransform();
    void toggleAnimation(bool on);
    void animationStep();
    void showNonCommutativity();
    void optionsChanged();
    void refreshAll();

private:
    QWidget* buildViews();
    QWidget* buildSidePanel();
    void refreshInfo();
    void refreshMatrix();
    bool readAxis(m3d::Vec3& p1, m3d::Vec3& p2);

    Scene scene_;
    ViewportWidget* views_[4] = {};

    QSpinBox* seed_ = nullptr;
    QSpinBox* layers_ = nullptr;
    QLabel* paramsLabel_ = nullptr;
    QLabel* topoLabel_ = nullptr;
    QLabel* checksumLabel_ = nullptr;

    QDoubleSpinBox* t_[3] = {};
    QDoubleSpinBox* s_[3] = {};
    QComboBox* rotAxis_ = nullptr;
    QDoubleSpinBox* rotAngle_ = nullptr;
    QDoubleSpinBox* p1_[3] = {};
    QDoubleSpinBox* p2_[3] = {};
    QDoubleSpinBox* lineAngle_ = nullptr;
    QDoubleSpinBox* speed_ = nullptr;
    QCheckBox* animate_ = nullptr;
    QTableWidget* matrix_ = nullptr;
    QListWidget* history_ = nullptr;

    QCheckBox* fill_ = nullptr;
    QCheckBox* edges_ = nullptr;
    QCheckBox* cull_ = nullptr;
    QCheckBox* zbuf_ = nullptr;
    QCheckBox* light_ = nullptr;
    QCheckBox* groups_ = nullptr;
    QCheckBox* showAxis_ = nullptr;
    QCheckBox* showWorldAxes_ = nullptr;
    QDoubleSpinBox* lightPos_[3] = {};
    QDoubleSpinBox* fov_ = nullptr;
    QDoubleSpinBox* near_ = nullptr;
    QDoubleSpinBox* far_ = nullptr;

    QTimer* timer_ = nullptr;
    QElapsedTimer clock_;
    double animAngle_ = 0;  // накопленный угол анимации (для истории)
};
