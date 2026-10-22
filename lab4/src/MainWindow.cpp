#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QIntValidator>
#include <chrono>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Лабораторная №4 — Растровые алгоритмы");
    resize(1000, 750);

    QWidget* central = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(central);

    QHBoxLayout* controlsLayout = new QHBoxLayout();

    x1Edit = new QLineEdit("0", this);
    y1Edit = new QLineEdit("0", this);
    x2Edit = new QLineEdit("100", this);
    y2Edit = new QLineEdit("100", this);
    radiusEdit = new QLineEdit("50", this);

    x1Edit->setValidator(new QIntValidator(-1000, 1000, this));
    y1Edit->setValidator(new QIntValidator(-1000, 1000, this));
    x2Edit->setValidator(new QIntValidator(-1000, 1000, this));
    y2Edit->setValidator(new QIntValidator(-1000, 1000, this));
    radiusEdit->setValidator(new QIntValidator(1, 1000, this));

    x1Edit->setMaximumWidth(60);
    y1Edit->setMaximumWidth(60);
    x2Edit->setMaximumWidth(60);
    y2Edit->setMaximumWidth(60);
    radiusEdit->setMaximumWidth(60);

    algorithmCombo = new QComboBox(this);
    algorithmCombo->addItem("Пошаговый");
    algorithmCombo->addItem("ЦДА");
    algorithmCombo->addItem("Брезенхем (линия)");

    drawLineButton = new QPushButton("Нарисовать линию", this);
    drawCircleButton = new QPushButton("Нарисовать окружность", this);
    clearButton = new QPushButton("Очистить", this);

    timeLabel = new QLabel("Время: —", this);

    controlsLayout->addWidget(new QLabel("X1:", this));
    controlsLayout->addWidget(x1Edit);
    controlsLayout->addWidget(new QLabel("Y1:", this));
    controlsLayout->addWidget(y1Edit);
    controlsLayout->addWidget(new QLabel("X2:", this));
    controlsLayout->addWidget(x2Edit);
    controlsLayout->addWidget(new QLabel("Y2:", this));
    controlsLayout->addWidget(y2Edit);
    controlsLayout->addWidget(algorithmCombo);
    controlsLayout->addWidget(drawLineButton);

    controlsLayout->addSpacing(20);

    controlsLayout->addWidget(new QLabel("R:", this));
    controlsLayout->addWidget(radiusEdit);
    controlsLayout->addWidget(drawCircleButton);

    controlsLayout->addSpacing(20);
    controlsLayout->addWidget(clearButton);
    controlsLayout->addStretch();

    mainLayout->addLayout(controlsLayout);

    canvas = new Canvas(this);
    mainLayout->addWidget(canvas);

    mainLayout->addWidget(timeLabel);

    setCentralWidget(central);

    connect(drawLineButton, &QPushButton::clicked, this, &MainWindow::onDrawLine);
    connect(drawCircleButton, &QPushButton::clicked, this, &MainWindow::onDrawCircle);
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::onClear);
}

MainWindow::~MainWindow() = default;

void MainWindow::onDrawLine() {
    int x1 = x1Edit->text().toInt();
    int y1 = y1Edit->text().toInt();
    int x2 = x2Edit->text().toInt();
    int y2 = y2Edit->text().toInt();
    int method = algorithmCombo->currentIndex();

    auto start = std::chrono::high_resolution_clock::now();

    canvas->drawLine(x1, y1, x2, y2, method);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    timeLabel->setText(QString("Время: %1 мкс").arg(duration));
}

void MainWindow::onDrawCircle() {
    int cx = x1Edit->text().toInt();
    int cy = y1Edit->text().toInt();
    int radius = radiusEdit->text().toInt();

    auto start = std::chrono::high_resolution_clock::now();

    canvas->drawCircle(cx, cy, radius);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    timeLabel->setText(QString("Время: %1 мкс").arg(duration));
}

void MainWindow::onClear() {
    canvas->clear();
    timeLabel->setText("Время: —");
}