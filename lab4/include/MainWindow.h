#pragma once
#include <QMainWindow>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include "Canvas.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    private slots:
        void onDrawLine();
    void onDrawCircle();
    void onClear();

private:
    QLineEdit* x1Edit;
    QLineEdit* y1Edit;
    QLineEdit* x2Edit;
    QLineEdit* y2Edit;
    QLineEdit* radiusEdit;
    QComboBox* algorithmCombo;
    QPushButton* drawLineButton;
    QPushButton* drawCircleButton;
    QPushButton* clearButton;
    QLabel* timeLabel;

    Canvas* canvas;
};