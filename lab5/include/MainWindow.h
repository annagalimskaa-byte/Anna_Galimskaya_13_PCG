#pragma once
#include <QMainWindow>
#include <QComboBox>
#include <QPlainTextEdit>
#include <QLabel>
#include <vector>
#include "Canvas.h"

// Входные данные после разбора текста
struct InputData {
    std::vector<Segment> segments;
    Rect rect{-100, -100, 100, 100};
    std::vector<Point> polygon;
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    Canvas* canvas;
    QComboBox* modeCombo;
    QPlainTextEdit* inputData;
    QPlainTextEdit* resultView;
    QLabel* coordLabel;

    void onDraw();
    void onOpenFile();
    void onLoadExample();
    bool parseInput(const QString& text, InputData& data, QString& error) const;
    void showError(const QString& message);
};
