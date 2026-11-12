#pragma once

#include <QMainWindow>

class CanvasWidget;
class QComboBox;
class QLabel;
class QPlainTextEdit;
class QTableWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

    bool loadFile(const QString& path);

private slots:
    void openFileDialog();
    void exampleChosen(int index);
    void applyEditorText();

private:
    void buildUi();
    bool processText(const QString& text, const QString& sourceName);

    CanvasWidget* canvas_ = nullptr;
    QComboBox* examples_ = nullptr;
    QLabel* algorithmLabel_ = nullptr;
    QPlainTextEdit* editor_ = nullptr;
    QTableWidget* table_ = nullptr;
    QLabel* coordsLabel_ = nullptr;
};
