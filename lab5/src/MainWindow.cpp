#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QSplitter>
#include <QStatusBar>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QFontDatabase>
#include <QRegularExpression>
#include <QShortcut>
#include <QKeySequence>

namespace {
const char* EXAMPLE_INPUT =
    "5\n"
    "-200 -100 200 100\n"
    "-50 150 50 -150\n"
    "0 0 300 300\n"
    "-250 120 250 160\n"
    "-60 -40 40 60\n"
    "-100 -100 100 100\n"
    "6\n"
    "-150 -100\n"
    "0 -150\n"
    "150 -100\n"
    "150 100\n"
    "0 150\n"
    "-150 100\n";

const std::vector<Point> DEFAULT_POLYGON = {{-150, -100}, {0, -150}, {150, -100}, {150, 100}, {0, 150}, {-150, 100}};

QString pointToString(const Point& p) {
    return QString("(%1; %2)").arg(p.x, 0, 'f', 2).arg(p.y, 0, 'f', 2);
}
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Лабораторная 5 — Вариант 6: отсечение отрезков");
    resize(1200, 800);

    // ---------- левая панель ----------
    auto* panel = new QWidget(this);
    auto* panelLayout = new QVBoxLayout(panel);

    auto* algoBox = new QGroupBox("Алгоритм", panel);
    auto* algoLayout = new QVBoxLayout(algoBox);
    modeCombo = new QComboBox(algoBox);
    modeCombo->addItem("Ч.1: Алгоритм средней точки (окно)");
    modeCombo->addItem("Ч.2: Кирус-Бек (выпуклый многоугольник)");
    algoLayout->addWidget(modeCombo);
    panelLayout->addWidget(algoBox);

    auto* buttons = new QHBoxLayout();
    auto* openBtn = new QPushButton("Открыть файл…", panel);
    auto* exampleBtn = new QPushButton("Пример", panel);
    buttons->addWidget(openBtn);
    buttons->addWidget(exampleBtn);
    panelLayout->addLayout(buttons);

    auto* inputBox = new QGroupBox("Входные данные", panel);
    auto* inputLayout = new QVBoxLayout(inputBox);
    auto* hint = new QLabel(
        "n — число отрезков<br>"
        "X1 Y1 X2 Y2 — n строк<br>"
        "Xmin Ymin Xmax Ymax — окно<br>"
        "<i>для части 2 (необязательно):</i><br>"
        "m — число вершин, затем m строк X Y", inputBox);
    hint->setStyleSheet("color: gray;");
    inputLayout->addWidget(hint);
    inputData = new QPlainTextEdit(inputBox);
    inputData->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    inputData->setPlainText(EXAMPLE_INPUT);
    inputLayout->addWidget(inputData);
    auto* drawBtn = new QPushButton("Отрисовать  (⌘↵)", inputBox);
    drawBtn->setDefault(true);
    inputLayout->addWidget(drawBtn);
    panelLayout->addWidget(inputBox, 3);

    auto* resultBox = new QGroupBox("Результат отсечения", panel);
    auto* resultLayout = new QVBoxLayout(resultBox);
    resultView = new QPlainTextEdit(resultBox);
    resultView->setReadOnly(true);
    resultView->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    resultLayout->addWidget(resultView);
    panelLayout->addWidget(resultBox, 2);

    // ---------- холст ----------
    canvas = new Canvas(this);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(panel);
    splitter->addWidget(canvas);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({330, 870});
    setCentralWidget(splitter);

    coordLabel = new QLabel(this);
    statusBar()->addPermanentWidget(coordLabel);

    // ---------- сигналы ----------
    connect(drawBtn, &QPushButton::clicked, this, &MainWindow::onDraw);
    connect(openBtn, &QPushButton::clicked, this, &MainWindow::onOpenFile);
    connect(exampleBtn, &QPushButton::clicked, this, &MainWindow::onLoadExample);
    connect(modeCombo, &QComboBox::currentIndexChanged, this, &MainWindow::onDraw);
    connect(new QShortcut(QKeySequence("Ctrl+Return"), this), &QShortcut::activated, this, &MainWindow::onDraw);
    connect(canvas, &Canvas::cursorMoved, this, [this](double x, double y) {
        coordLabel->setText(QString("x = %1   y = %2").arg(x, 0, 'f', 1).arg(y, 0, 'f', 1));
    });

    onDraw();
}

// ---------------------------------------------------------------------------
// Разбор входных данных. Числа читаются подряд, поэтому переносы строк не важны.
// ---------------------------------------------------------------------------
bool MainWindow::parseInput(const QString& text, InputData& data, QString& error) const {
    const QStringList tokens = text.split(QRegularExpression("[\\s,;]+"), Qt::SkipEmptyParts);
    qsizetype pos = 0;

    auto readNumber = [&](double& value, const QString& what) -> bool {
        if (pos >= tokens.size()) { error = QString("Не хватает данных: ожидается %1").arg(what); return false; }
        bool ok = false;
        value = tokens[pos].toDouble(&ok);
        if (!ok) { error = QString("«%1» — не число (%2)").arg(tokens[pos], what); return false; }
        ++pos;
        return true;
    };

    double nValue;
    if (!readNumber(nValue, "число отрезков n")) return false;
    int n = (int)nValue;
    if (n < 0 || n != nValue) { error = "n должно быть целым неотрицательным числом"; return false; }

    data.segments.clear();
    for (int i = 0; i < n; ++i) {
        double x1, y1, x2, y2;
        QString what = QString("координаты отрезка %1").arg(i + 1);
        if (!readNumber(x1, what) || !readNumber(y1, what) || !readNumber(x2, what) || !readNumber(y2, what)) return false;
        data.segments.push_back({{x1, y1}, {x2, y2}});
    }

    double xmin, ymin, xmax, ymax;
    if (!readNumber(xmin, "Xmin окна") || !readNumber(ymin, "Ymin окна") ||
        !readNumber(xmax, "Xmax окна") || !readNumber(ymax, "Ymax окна")) return false;
    if (xmin > xmax) std::swap(xmin, xmax);
    if (ymin > ymax) std::swap(ymin, ymax);
    if (xmin == xmax || ymin == ymax) { error = "Окно вырождено: ширина или высота равна нулю"; return false; }
    data.rect = {xmin, ymin, xmax, ymax};

    // Многоугольник (необязательный блок)
    data.polygon = DEFAULT_POLYGON;
    if (pos < tokens.size()) {
        double mValue;
        if (!readNumber(mValue, "число вершин m")) return false;
        int m = (int)mValue;
        if (m < 3 || m != mValue) { error = "Число вершин многоугольника должно быть целым и не меньше 3"; return false; }
        data.polygon.clear();
        for (int i = 0; i < m; ++i) {
            double x, y;
            QString what = QString("вершина %1").arg(i + 1);
            if (!readNumber(x, what) || !readNumber(y, what)) return false;
            data.polygon.push_back({x, y});
        }
    }
    if (pos < tokens.size()) { error = QString("Лишние данные в конце: «%1»").arg(tokens[pos]); return false; }
    return true;
}

void MainWindow::onDraw() {
    InputData data;
    QString error;
    if (!parseInput(inputData->toPlainText(), data, error)) { showError(error); return; }

    const ClipMode mode = static_cast<ClipMode>(modeCombo->currentIndex());
    if (mode == ClipMode::CyrusBeck && !Clipper::isConvex(data.polygon)) {
        showError("Многоугольник не выпуклый — алгоритм Кируса-Бека к нему неприменим");
        return;
    }

    std::vector<std::optional<Segment>> clipped;
    QString report;
    int visibleCount = 0;
    for (size_t i = 0; i < data.segments.size(); ++i) {
        const Segment& s = data.segments[i];
        std::optional<Segment> r = mode == ClipMode::Midpoint ? Clipper::midpoint(s, data.rect)
                                                              : Clipper::cyrusBeck(s, data.polygon);
        clipped.push_back(r);
        report += QString("%1) %2 – %3\n").arg(i + 1).arg(pointToString(s.p1), pointToString(s.p2));
        if (r) { report += QString("   видим: %1 – %2\n").arg(pointToString(r->p1), pointToString(r->p2)); ++visibleCount; }
        else   { report += "   полностью невидим\n"; }
    }
    resultView->setPlainText(report);
    canvas->setScene(mode, data.rect, data.polygon, data.segments, clipped);
    statusBar()->showMessage(QString("Отрезков: %1, видимых: %2").arg(data.segments.size()).arg(visibleCount));
}

void MainWindow::onOpenFile() {
    QString path = QFileDialog::getOpenFileName(this, "Открыть входной файл", QString(), "Текстовые файлы (*.txt);;Все файлы (*)");
    if (path.isEmpty()) return;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) { showError("Не удалось открыть файл: " + file.errorString()); return; }
    inputData->setPlainText(QTextStream(&file).readAll());
    onDraw();
}

void MainWindow::onLoadExample() {
    inputData->setPlainText(EXAMPLE_INPUT);
    onDraw();
}

void MainWindow::showError(const QString& message) {
    resultView->setPlainText("Ошибка: " + message);
    statusBar()->showMessage("Ошибка: " + message);
}
