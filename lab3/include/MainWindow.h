#pragma once
#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QSlider>
#include "Image.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    private slots:
        void onOpenImage();
    void onSaveImage();
    void onProcess();

private:
    QPushButton* openButton;
    QPushButton* saveButton;
    QPushButton* processButton;
    QComboBox* methodCombo;
    QSlider* windowSlider;
    QSlider* cSlider;

    QLabel* originalLabel;
    QLabel* processedLabel;
    QLabel* statusLabel;

    Image originalImage;
    Image processedImage;

    void displayImage(const Image& img, QLabel* label);
};