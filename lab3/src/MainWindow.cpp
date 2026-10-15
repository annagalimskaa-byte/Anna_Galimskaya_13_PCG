#include "MainWindow.h"
#include "ImageProcessor.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QImage>
#include <QPixmap>
#include <QDebug>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    qDebug() << "=== MainWindow constructor START ===";

    setWindowTitle("Лабораторная №3 — Пороговая обработка");
    resize(1200, 700);

    QWidget* central = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(central);

    // Верхняя панель
    QHBoxLayout* topLayout = new QHBoxLayout();
    openButton = new QPushButton("Открыть...", this);
    saveButton = new QPushButton("Сохранить...", this);
    processButton = new QPushButton("Обработать", this);

    methodCombo = new QComboBox(this);
    methodCombo->addItem("Оцу (глобальный)");
    methodCombo->addItem("Итеративный (глобальный)");
    methodCombo->addItem("Адаптивный");

    topLayout->addWidget(openButton);
    topLayout->addWidget(saveButton);
    topLayout->addWidget(processButton);
    topLayout->addWidget(new QLabel("Метод:", this));
    topLayout->addWidget(methodCombo);
    topLayout->addStretch();
    mainLayout->addLayout(topLayout);

    // Панели изображений
    QHBoxLayout* imagesLayout = new QHBoxLayout();
    originalLabel = new QLabel("Исходное", this);
    processedLabel = new QLabel("Обработанное", this);
    originalLabel->setMinimumSize(500, 500);
    processedLabel->setMinimumSize(500, 500);
    originalLabel->setAlignment(Qt::AlignCenter);
    processedLabel->setAlignment(Qt::AlignCenter);
    originalLabel->setStyleSheet("border: 1px solid gray;");
    processedLabel->setStyleSheet("border: 1px solid gray;");
    imagesLayout->addWidget(originalLabel);
    imagesLayout->addWidget(processedLabel);
    mainLayout->addLayout(imagesLayout);

    // Статус
    statusLabel = new QLabel("Готов к работе", this);
    mainLayout->addWidget(statusLabel);

    setCentralWidget(central);

    // Подключение кнопок
    connect(openButton, &QPushButton::clicked, this, &MainWindow::onOpenImage);
    connect(saveButton, &QPushButton::clicked, this, &MainWindow::onSaveImage);
    connect(processButton, &QPushButton::clicked, this, &MainWindow::onProcess);

    qDebug() << "=== MainWindow constructor END, connections done ===";
}

MainWindow::~MainWindow() = default;

void MainWindow::onOpenImage() {
    qDebug() << "=== onOpenImage CALLED ===";

    QString file = QFileDialog::getOpenFileName(
        this,
        "Открыть изображение",
        QString(),
        "Images (*.png *.jpg *.jpeg *.bmp)",
        nullptr,
        QFileDialog::DontUseNativeDialog);   // ← отключаем нативный диалог macOS

    qDebug() << "=== file selected:" << file;

    if (file.isEmpty()) {
        qDebug() << "=== file is empty, returning ===";
        return;
    }

    QImage qimg(file);
    if (qimg.isNull()) {
        qDebug() << "=== QImage is null ===";
        QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл");
        return;
    }

    qDebug() << "=== image loaded:" << qimg.width() << "x" << qimg.height();

    qimg = qimg.convertToFormat(QImage::Format_RGBA8888);
    originalImage.fromQImage(qimg.constBits(), qimg.width(), qimg.height());
    processedImage = originalImage;

    displayImage(originalImage, originalLabel);
    displayImage(processedImage, processedLabel);

    statusLabel->setText(QString("Загружено: %1x%2").arg(qimg.width()).arg(qimg.height()));

    qDebug() << "=== onOpenImage END ===";
}

void MainWindow::onSaveImage() {
    qDebug() << "=== onSaveImage CALLED ===";

    if (processedImage.data.empty()) {
        QMessageBox::warning(this, "Ошибка", "Нет обработанного изображения");
        return;
    }

    QString file = QFileDialog::getSaveFileName(
        this,
        "Сохранить изображение",
        QString(),
        "PNG (*.png)",
        nullptr,
        QFileDialog::DontUseNativeDialog);   // ← тоже отключаем нативный

    if (file.isEmpty()) return;

    QImage qimg(processedImage.width, processedImage.height, QImage::Format_Grayscale8);
    for (int y = 0; y < processedImage.height; y++) {
        for (int x = 0; x < processedImage.width; x++) {
            qimg.setPixel(x, y, processedImage.at(x, y));
        }
    }

    qimg.save(file);
    statusLabel->setText("Сохранено: " + file);
}

void MainWindow::onProcess() {
    qDebug() << "=== onProcess CALLED ===";

    if (originalImage.data.empty()) {
        QMessageBox::warning(this, "Ошибка", "Сначала загрузите изображение");
        return;
    }

    int method = methodCombo->currentIndex();
    QString methodName;

    switch (method) {
        case 0:
            processedImage = ImageProcessor::otsuThreshold(originalImage);
            methodName = QString("Оцу, порог = %1").arg(ImageProcessor::findOtsuThreshold(originalImage));
            break;
        case 1:
            processedImage = ImageProcessor::iterativeThreshold(originalImage);
            methodName = QString("Итеративный, порог = %1").arg(ImageProcessor::findIterativeThreshold(originalImage));
            break;
        case 2:
            processedImage = ImageProcessor::adaptiveThreshold(originalImage, 15, 5);
            methodName = "Адаптивный (окно 15, C=5)";
            break;
    }

    displayImage(processedImage, processedLabel);
    statusLabel->setText("Обработано: " + methodName);

    qDebug() << "=== onProcess END:" << methodName;
}

void MainWindow::displayImage(const Image& img, QLabel* label) {
    if (img.data.empty()) return;

    QImage qimg(img.width, img.height, QImage::Format_Grayscale8);
    for (int y = 0; y < img.height; y++) {
        for (int x = 0; x < img.width; x++) {
            qimg.setPixel(x, y, img.at(x, y));
        }
    }

    label->setPixmap(QPixmap::fromImage(qimg).scaled(
        label->width(), label->height(),
        Qt::KeepAspectRatio, Qt::SmoothTransformation));
}