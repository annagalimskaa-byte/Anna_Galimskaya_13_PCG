#include "main_window.h"

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QStatusBar>
#include <QTableWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <cmath>

#include "canvas_widget.h"
#include "input_parser.h"

namespace {

QString formatNumber(double v) {
    if (std::fabs(v) < 5e-10) v = 0.0;
    QString t = QString::number(v, 'f', 3);
    if (t.contains('.')) {
        while (t.endsWith('0')) t.chop(1);
        if (t.endsWith('.')) t.chop(1);
    }
    return t;
}

QString pointToString(const geom::Point& p) {
    return QString("(%1; %2)").arg(formatNumber(p.x), formatNumber(p.y));
}

QString segmentToString(const geom::Point& a, const geom::Point& b) {
    return pointToString(a) + " –\n" + pointToString(b);
}

}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    buildUi();
    setWindowTitle("Лабораторная работа 5 — отсечение отрезков (вариант 6)");
    resize(1280, 780);
}

void MainWindow::buildUi() {
    QToolBar* tb = addToolBar("Главная");
    tb->setMovable(false);

    QAction* openAct = tb->addAction("Открыть файл…");
    openAct->setShortcut(QKeySequence::Open);
    connect(openAct, &QAction::triggered, this, &MainWindow::openFileDialog);

    tb->addSeparator();
    tb->addWidget(new QLabel(" Пример: "));
    examples_ = new QComboBox;
    const QStringList files = QDir(":/examples").entryList(QDir::Files, QDir::Name);
    for (const QString& f : files) examples_->addItem(f, ":/examples/" + f);
    examples_->setCurrentIndex(examples_->findText("part1_basic.txt"));
    tb->addWidget(examples_);

    tb->addSeparator();
    auto* showOriginal = new QCheckBox("Исходные отрезки");
    showOriginal->setChecked(true);
    tb->addWidget(showOriginal);
    auto* showGrid = new QCheckBox("Сетка");
    showGrid->setChecked(true);
    tb->addWidget(showGrid);

    tb->addSeparator();
    QAction* fitAct = tb->addAction("Вписать в окно");

    canvas_ = new CanvasWidget;
    connect(showOriginal, &QCheckBox::toggled, canvas_, &CanvasWidget::setShowOriginal);
    connect(showGrid, &QCheckBox::toggled, canvas_, &CanvasWidget::setShowGrid);
    connect(fitAct, &QAction::triggered, canvas_, &CanvasWidget::fitToScene);

    auto* side = new QWidget;
    side->setMinimumWidth(430);
    auto* lay = new QVBoxLayout(side);

    algorithmLabel_ = new QLabel;
    algorithmLabel_->setWordWrap(true);
    QFont bold = algorithmLabel_->font();
    bold.setBold(true);
    bold.setPointSizeF(bold.pointSizeF() + 1);
    algorithmLabel_->setFont(bold);
    lay->addWidget(algorithmLabel_);

    lay->addWidget(new QLabel("Входные данные (можно редактировать):"));
    editor_ = new QPlainTextEdit;
    editor_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    lay->addWidget(editor_, 2);

    auto* applyBtn = new QPushButton("Применить");
    applyBtn->setShortcut(QKeySequence("Ctrl+Return"));
    connect(applyBtn, &QPushButton::clicked, this, &MainWindow::applyEditorText);
    lay->addWidget(applyBtn);

    lay->addWidget(new QLabel("Результат отсечения:"));
    table_ = new QTableWidget(0, 4);
    table_->setHorizontalHeaderLabels({"№", "Исходный отрезок", "Видимая часть", "Шагов"});
    table_->verticalHeader()->setVisible(false);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table_->setWordWrap(true);
    lay->addWidget(table_, 3);

    auto* splitter = new QSplitter;
    splitter->addWidget(canvas_);
    splitter->addWidget(side);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    splitter->setSizes({840, 440});
    setCentralWidget(splitter);

    coordsLabel_ = new QLabel;
    statusBar()->addPermanentWidget(coordsLabel_);
    statusBar()->showMessage("Колесо мыши — масштаб, перетаскивание — сдвиг");
    connect(canvas_, &CanvasWidget::mouseWorldPosition, this, [this](QPointF p) {
        coordsLabel_->setText(QString("x = %1   y = %2").arg(p.x(), 0, 'f', 2).arg(p.y(), 0, 'f', 2));
    });

    connect(examples_, &QComboBox::currentIndexChanged, this, &MainWindow::exampleChosen);
}

void MainWindow::openFileDialog() {
    const QString path = QFileDialog::getOpenFileName(this, "Открыть входной файл", QString(),
                                                      "Текстовые файлы (*.txt);;Все файлы (*)");
    if (!path.isEmpty()) loadFile(path);
}

void MainWindow::exampleChosen(int index) {
    if (index >= 0) loadFile(examples_->itemData(index).toString());
}

bool MainWindow::loadFile(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл:\n" + path);
        return false;
    }
    const QString text = QString::fromUtf8(f.readAll());
    editor_->setPlainText(text);
    return processText(text, QFileInfo(path).fileName());
}

void MainWindow::applyEditorText() {
    processText(editor_->toPlainText(), "редактор");
}

bool MainWindow::processText(const QString& text, const QString& sourceName) {
    io::InputData data;
    try {
        data = io::parseInput(text.toStdString());
    } catch (const io::ParseError& e) {
        QMessageBox::warning(this, "Ошибка во входных данных",
                             QString::fromUtf8(e.what()));
        return false;
    }
    const auto results = io::clipAll(data);
    canvas_->setScene(data, results);

    QString header = QString::fromUtf8(io::algorithmName(data.kind)) + "\n";
    if (data.kind == io::WindowKind::Rectangle) {
        header += QString("Окно: x ∈ [%1; %2], y ∈ [%3; %4]")
                      .arg(data.rect.xmin).arg(data.rect.xmax).arg(data.rect.ymin).arg(data.rect.ymax);
    } else {
        header += QString("Окно: выпуклый многоугольник, вершин: %1").arg(static_cast<int>(data.polygon.size()));
    }
    algorithmLabel_->setText(header);

    table_->setRowCount(static_cast<int>(results.size()));
    int visibleCount = 0;
    for (int i = 0; i < static_cast<int>(results.size()); ++i) {
        const auto& s = data.segments[i];
        const auto& r = results[i];
        table_->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
        table_->setItem(i, 1, new QTableWidgetItem(segmentToString(s.a, s.b)));
        auto* vis = new QTableWidgetItem(r.visible ? segmentToString(r.visible->a, r.visible->b)
                                                   : QString("невидим"));
        vis->setForeground(r.visible ? QColor(22, 120, 60) : QColor(160, 60, 60));
        table_->setItem(i, 2, vis);
        table_->setItem(i, 3, new QTableWidgetItem(QString::number(r.steps)));
        if (r.visible) ++visibleCount;
    }
    statusBar()->showMessage(QString("%1: отрезков %2, видимых %3")
                                 .arg(sourceName).arg(static_cast<int>(results.size())).arg(visibleCount));
    return true;
}
