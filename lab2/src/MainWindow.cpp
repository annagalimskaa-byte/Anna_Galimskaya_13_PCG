#include "MainWindow.h"
#include "BmpParser.h"
#include "PngParser.h"
#include "JpegParser.h"
#include "GifParser.h"
#include "TiffParser.h"
#include "PcxParser.h"
#include "ThreadPool.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QHeaderView>
#include <QMessageBox>
#include <QElapsedTimer>
#include <filesystem>
#include <cctype>

namespace fs = std::filesystem;

ScanWorker::ScanWorker(QString path) : scanPath(std::move(path)) {}

ScanWorker::~ScanWorker() = default;

static ImageInfo parseFileByExt(const fs::path& filePath) {
    std::string ext = filePath.extension().string();
    for (auto& c : ext) c = std::tolower(c);

    if (ext == ".bmp")  return BmpParser::parse(filePath.string());
    if (ext == ".png")  return PngParser::parse(filePath.string());
    if (ext == ".jpg" || ext == ".jpeg") return JpegParser::parse(filePath.string());
    if (ext == ".gif")  return GifParser::parse(filePath.string());
    if (ext == ".tif" || ext == ".tiff") return TiffParser::parse(filePath.string());
    if (ext == ".pcx")  return PcxParser::parse(filePath.string());

    ImageInfo info;
    info.filePath = filePath.string();
    info.fileName = filePath.filename().string();
    info.format = "UNKNOWN";
    info.status = ParseStatus::UNKNOWN_FORMAT;
    return info;
}

void ScanWorker::process() {
    QElapsedTimer timer;
    timer.start();

    fs::path path(scanPath.toStdString());
    std::vector<fs::path> files;

    if (fs::is_directory(path)) {
        for (const auto& entry : fs::recursive_directory_iterator(path)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path());
            }
        }
    } else if (fs::is_regular_file(path)) {
        files.push_back(path);
    }

    int total = static_cast<int>(files.size());
    if (total == 0) {
        emit finished({}, 0, 0, 0);
        return;
    }

    size_t numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4;

    std::vector<ImageInfo> results;
    results.reserve(files.size());
    std::mutex resultsMutex;
    std::atomic<int> processed{0};

    {
        ThreadPool pool(numThreads);
        for (const auto& filePath : files) {
            pool.enqueue([filePath, &results, &resultsMutex, &processed, total, this]() {
                ImageInfo info = parseFileByExt(filePath);
                {
                    std::lock_guard<std::mutex> lock(resultsMutex);
                    results.push_back(info);
                }
                int done = ++processed;
                if (done % 50 == 0 || done == total) {
                    emit progress(done, total);
                }
            });
        }
        pool.waitAll();
    }

    int okCount = 0;
    int errorCount = 0;
    for (const auto& info : results) {
        if (info.status == ParseStatus::OK) okCount++;
        else errorCount++;
    }

    emit finished(results, timer.elapsed(), okCount, errorCount);
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Image Parser Lab");
    resize(1100, 700);

    QWidget* central = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(central);

    QHBoxLayout* topLayout = new QHBoxLayout();
    openButton = new QPushButton("Открыть папку...", this);
    openButton->setMinimumHeight(36);
    statusLabel = new QLabel("Готов к работе", this);
    topLayout->addWidget(openButton);
    topLayout->addWidget(statusLabel);
    topLayout->addStretch();

    mainLayout->addLayout(topLayout);

    progressBar = new QProgressBar(this);
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    mainLayout->addWidget(progressBar);

    table = new QTableWidget(this);
    table->setColumnCount(7);
    table->setHorizontalHeaderLabels({
        "Файл", "Формат", "Размер", "Глубина", "Сжатие", "DPI", "Статус"
    });
    table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->verticalHeader()->setVisible(false);
    mainLayout->addWidget(table);

    setCentralWidget(central);

    connect(openButton, &QPushButton::clicked, this, &MainWindow::onOpenFolderClicked);
}

MainWindow::~MainWindow() {
    if (workerThread) {
        workerThread->quit();
        workerThread->wait();
    }
}

void MainWindow::onOpenFolderClicked() {
    QString dir = QFileDialog::getExistingDirectory(this, "Выберите папку с изображениями");
    if (dir.isEmpty()) return;

    table->setRowCount(0);
    progressBar->setValue(0);
    statusLabel->setText("Сканирование...");
    openButton->setEnabled(false);

    workerThread = new QThread(this);
    ScanWorker* worker = new ScanWorker(dir);
    worker->moveToThread(workerThread);

    connect(workerThread, &QThread::started, worker, &ScanWorker::process);
    connect(worker, &ScanWorker::progress, this, &MainWindow::onScanProgress);
    connect(worker, &ScanWorker::finished, this, &MainWindow::onScanFinished);

    connect(worker, &ScanWorker::finished, workerThread, &QThread::quit);
    connect(workerThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);

    workerThread->start();
}

void MainWindow::onScanProgress(int current, int total) {
    if (total <= 0) return;
    int percent = (current * 100) / total;
    progressBar->setValue(percent);
    statusLabel->setText(QString("Обработано %1 из %2...").arg(current).arg(total));
}

void MainWindow::onScanFinished(std::vector<ImageInfo> results, qint64 elapsedMs, int okCount, int errorCount) {
    table->setRowCount(static_cast<int>(results.size()));

    for (size_t i = 0; i < results.size(); i++) {
        const auto& info = results[i];
        int row = static_cast<int>(i);

        table->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(info.fileName)));
        table->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(info.format)));
        table->setItem(row, 2, new QTableWidgetItem(
            QString("%1 x %2").arg(info.width).arg(info.height)));
        table->setItem(row, 3, new QTableWidgetItem(
            QString("%1 bit").arg(info.colorDepth)));
        table->setItem(row, 4, new QTableWidgetItem(QString::fromStdString(info.compression)));
        table->setItem(row, 5, new QTableWidgetItem(QString::number(info.dpi, 'f', 2)));

        QString statusText;
        switch (info.status) {
            case ParseStatus::OK: statusText = "OK"; break;
            case ParseStatus::CORRUPTED: statusText = "Повреждён"; break;
            case ParseStatus::UNKNOWN_FORMAT: statusText = "Неизвестный формат"; break;
            case ParseStatus::IO_ERROR: statusText = "Ошибка чтения"; break;
        }
        QTableWidgetItem* statusItem = new QTableWidgetItem(statusText);
        if (info.status != ParseStatus::OK) {
            statusItem->setBackground(QColor(255, 200, 200));
        }
        table->setItem(row, 6, statusItem);
    }

    table->resizeColumnsToContents();

    progressBar->setValue(100);
    statusLabel->setText(QString("Готово. OK: %1, Ошибок: %2, Время: %3 мс")
                            .arg(okCount).arg(errorCount).arg(elapsedMs));

    openButton->setEnabled(true);
    workerThread = nullptr;
}