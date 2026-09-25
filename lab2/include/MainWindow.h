#pragma once
#include <QMainWindow>
#include <QTableWidget>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QThread>
#include <vector>
#include "ImageInfo.h"

class ScanWorker : public QObject {
    Q_OBJECT
public:
    explicit ScanWorker(QString path);
    ~ScanWorker();

    public slots:
        void process();

    signals:
        void progress(int current, int total);
    void finished(std::vector<ImageInfo> results, qint64 elapsedMs, int okCount, int errorCount);

private:
    QString scanPath;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    private slots:
        void onOpenFolderClicked();
    void onScanProgress(int current, int total);
    void onScanFinished(std::vector<ImageInfo> results, qint64 elapsedMs, int okCount, int errorCount);

private:
    QPushButton* openButton;
    QProgressBar* progressBar;
    QLabel* statusLabel;
    QTableWidget* table;

    QThread* workerThread = nullptr;
};