#include "main_window.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFontDatabase>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QRandomGenerator>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <cmath>

#include "viewport_widget.h"

using m3d::Mat4;
using m3d::Vec3;

namespace {

QDoubleSpinBox* spin(double min, double max, double value, double step, int decimals = 1) {
    auto* s = new QDoubleSpinBox;
    s->setRange(min, max);
    s->setDecimals(decimals);
    s->setSingleStep(step);
    s->setValue(value);
    s->setKeyboardTracking(false);
    s->setMinimumWidth(68);
    return s;
}

// Строка «x [ ] y [ ] z [ ]»
QWidget* xyzRow(QDoubleSpinBox* (&boxes)[3], double min, double max, const double values[3], double step,
                int decimals = 1) {
    auto* w = new QWidget;
    auto* l = new QHBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(4);
    const char* names[3] = {"x", "y", "z"};
    for (int i = 0; i < 3; ++i) {
        boxes[i] = spin(min, max, values[i], step, decimals);
        l->addWidget(new QLabel(names[i]));
        l->addWidget(boxes[i], 1);
    }
    return w;
}

QString num(double v, int prec = 3) {
    if (std::fabs(v) < 0.5 * std::pow(10.0, -prec)) v = 0;
    return QString::number(v, 'f', prec);
}

QString matrixHtml(const Mat4& m) {
    QString s = "<table cellspacing=0 cellpadding=2>";
    for (int i = 0; i < 4; ++i) {
        s += "<tr>";
        for (int j = 0; j < 4; ++j) s += "<td align=right>&nbsp;" + num(m(i, j)) + "&nbsp;</td>";
        s += "</tr>";
    }
    return s + "</table>";
}

}  // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Лабораторная работа 6 — генератор 3D-детали, матрицы 4×4, Z-буфер");
    scene_.generate(42, 1);
    scene_.resetCamera();

    auto* splitter = new QSplitter;
    splitter->addWidget(buildViews());
    auto* scroll = new QScrollArea;
    scroll->setWidget(buildSidePanel());
    scroll->setWidgetResizable(true);
    scroll->setMinimumWidth(400);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    splitter->addWidget(scroll);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);
    splitter->setSizes({1050, 420});
    setCentralWidget(splitter);
    resize(1480, 900);

    timer_ = new QTimer(this);
    timer_->setInterval(16);
    connect(timer_, &QTimer::timeout, this, &MainWindow::animationStep);

    statusBar()->showMessage("Введите Seed и нажмите «Сгенерировать». ЛКМ в главном окне — облёт камеры, "
                             "колесо — приближение, Shift + колесо — угол обзора.");
    refreshAll();
}

QWidget* MainWindow::buildViews() {
    auto* w = new QWidget;
    auto* grid = new QGridLayout(w);
    grid->setContentsMargins(6, 6, 6, 6);
    grid->setSpacing(6);
    views_[0] = new ViewportWidget(&scene_, render::ViewKind::Perspective);
    views_[1] = new ViewportWidget(&scene_, render::ViewKind::Front);
    views_[2] = new ViewportWidget(&scene_, render::ViewKind::Top);
    views_[3] = new ViewportWidget(&scene_, render::ViewKind::Side);
    grid->addWidget(views_[0], 0, 0);
    grid->addWidget(views_[1], 0, 1);
    grid->addWidget(views_[2], 1, 0);
    grid->addWidget(views_[3], 1, 1);
    grid->setColumnStretch(0, 3);
    grid->setColumnStretch(1, 2);
    grid->setRowStretch(0, 3);
    grid->setRowStretch(1, 2);
    connect(views_[0], &ViewportWidget::cameraChanged, this, [this] {
        const QSignalBlocker b(fov_);
        fov_->setValue(scene_.camera.fovDeg);
        refreshInfo();
    });
    return w;
}

QWidget* MainWindow::buildSidePanel() {
    auto* panel = new QWidget;
    auto* lay = new QVBoxLayout(panel);
    lay->setSpacing(8);

    // --- Генерация ---
    auto* gen = new QGroupBox("Генерация детали по Seed");
    auto* gl = new QGridLayout(gen);
    seed_ = new QSpinBox;
    seed_->setRange(-2000000000, 2000000000);
    seed_->setValue(42);
    layers_ = new QSpinBox;
    layers_->setRange(1, 200);
    layers_->setValue(1);
    layers_->setToolTip("1 — как в задании (передняя и задняя грань). Больше — плавная винтовая поверхность.");
    auto* genBtn = new QPushButton("Сгенерировать");
    genBtn->setDefault(true);
    auto* rndBtn = new QPushButton("Случайный Seed");
    gl->addWidget(new QLabel("Seed:"), 0, 0);
    gl->addWidget(seed_, 0, 1);
    gl->addWidget(genBtn, 0, 2);
    gl->addWidget(new QLabel("Слоёв по Z:"), 1, 0);
    gl->addWidget(layers_, 1, 1);
    gl->addWidget(rndBtn, 1, 2);
    paramsLabel_ = new QLabel;
    paramsLabel_->setWordWrap(true);
    paramsLabel_->setTextFormat(Qt::RichText);
    gl->addWidget(paramsLabel_, 2, 0, 1, 3);
    connect(genBtn, &QPushButton::clicked, this, &MainWindow::generate);
    connect(rndBtn, &QPushButton::clicked, this, &MainWindow::randomSeed);
    connect(seed_, &QSpinBox::editingFinished, this, &MainWindow::generate);
    connect(layers_, &QSpinBox::valueChanged, this, &MainWindow::generate);
    lay->addWidget(gen);

    // --- Самодиагностика ---
    auto* diag = new QGroupBox("Информационная панель");
    auto* dl = new QVBoxLayout(diag);
    topoLabel_ = new QLabel;
    topoLabel_->setTextFormat(Qt::RichText);
    checksumLabel_ = new QLabel;
    checksumLabel_->setTextFormat(Qt::RichText);
    checksumLabel_->setWordWrap(true);
    dl->addWidget(topoLabel_);
    dl->addWidget(checksumLabel_);
    lay->addWidget(diag);

    // --- Преобразования ---
    auto* tr = new QGroupBox("Преобразования (M ← Op · M)");
    auto* tl = new QFormLayout(tr);
    const double one3[3] = {1, 1, 1};
    const double t0[3] = {30, 0, 0};
    tl->addRow("Перенос:", xyzRow(t_, -1000, 1000, t0, 5));
    auto* tBtn = new QPushButton("Применить перенос T(Δx, Δy, Δz)");
    tl->addRow(tBtn);
    tl->addRow("Масштаб:", xyzRow(s_, -10, 10, one3, 0.1, 2));
    auto* sBtn = new QPushButton("Применить масштаб S(Sx, Sy, Sz)");
    sBtn->setToolTip("Отрицательный коэффициент — зеркальное отражение");
    tl->addRow(sBtn);
    auto* rotRow = new QWidget;
    auto* rl = new QHBoxLayout(rotRow);
    rl->setContentsMargins(0, 0, 0, 0);
    rotAxis_ = new QComboBox;
    rotAxis_->addItems({"Ox", "Oy", "Oz"});
    rotAxis_->setCurrentIndex(2);
    rotAngle_ = spin(-360, 360, 30, 5);
    rl->addWidget(rotAxis_);
    rl->addWidget(new QLabel("угол°"));
    rl->addWidget(rotAngle_, 1);
    tl->addRow("Поворот:", rotRow);
    auto* rBtn = new QPushButton("Применить поворот вокруг оси");
    tl->addRow(rBtn);
    connect(tBtn, &QPushButton::clicked, this, &MainWindow::applyTranslation);
    connect(sBtn, &QPushButton::clicked, this, &MainWindow::applyScaling);
    connect(rBtn, &QPushButton::clicked, this, &MainWindow::applyAxisRotation);
    lay->addWidget(tr);

    // --- Произвольная ось ---
    auto* ax = new QGroupBox("Вращение вокруг произвольной оси P1P2");
    auto* al = new QFormLayout(ax);
    const double p1v[3] = {scene_.p1.x, scene_.p1.y, scene_.p1.z};
    const double p2v[3] = {scene_.p2.x, scene_.p2.y, scene_.p2.z};
    al->addRow("P1:", xyzRow(p1_, -1000, 1000, p1v, 5));
    al->addRow("P2:", xyzRow(p2_, -1000, 1000, p2v, 5));
    lineAngle_ = spin(-360, 360, 15, 5);
    al->addRow("Угол θ°:", lineAngle_);
    auto* lBtn = new QPushButton("Повернуть вокруг P1P2");
    al->addRow(lBtn);
    auto* animRow = new QWidget;
    auto* anl = new QHBoxLayout(animRow);
    anl->setContentsMargins(0, 0, 0, 0);
    animate_ = new QCheckBox("Анимация");
    speed_ = spin(-360, 360, 40, 5);
    anl->addWidget(animate_);
    anl->addWidget(new QLabel("°/с"));
    anl->addWidget(speed_, 1);
    al->addRow(animRow);
    connect(lBtn, &QPushButton::clicked, this, &MainWindow::applyLineRotation);
    connect(animate_, &QCheckBox::toggled, this, &MainWindow::toggleAnimation);
    for (int i = 0; i < 3; ++i) {
        connect(p1_[i], &QDoubleSpinBox::valueChanged, this, [this] { optionsChanged(); });
        connect(p2_[i], &QDoubleSpinBox::valueChanged, this, [this] { optionsChanged(); });
    }
    lay->addWidget(ax);

    // --- Матрица ---
    auto* mx = new QGroupBox("Результирующая матрица моделирования M (4×4)");
    auto* ml = new QVBoxLayout(mx);
    matrix_ = new QTableWidget(4, 4);
    matrix_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    matrix_->horizontalHeader()->setVisible(false);
    matrix_->verticalHeader()->setVisible(false);
    matrix_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    matrix_->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    matrix_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    matrix_->setSelectionMode(QAbstractItemView::NoSelection);
    matrix_->setFixedHeight(4 * 26 + 4);
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j) {
            auto* item = new QTableWidgetItem;
            item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            matrix_->setItem(i, j, item);
        }
    ml->addWidget(matrix_);
    auto* btnRow = new QWidget;
    auto* bl = new QHBoxLayout(btnRow);
    bl->setContentsMargins(0, 0, 0, 0);
    auto* resetBtn = new QPushButton("Сброс преобразований (M = E)");
    auto* ncBtn = new QPushButton("T·R ≠ R·T ?");
    ncBtn->setToolTip("Проверка некоммутативности: перенос и поворот из полей выше в разном порядке");
    bl->addWidget(resetBtn, 2);
    bl->addWidget(ncBtn, 1);
    ml->addWidget(btnRow);
    ml->addWidget(new QLabel("Цепочка преобразований:"));
    history_ = new QListWidget;
    history_->setFixedHeight(90);
    ml->addWidget(history_);
    connect(resetBtn, &QPushButton::clicked, this, &MainWindow::resetTransform);
    connect(ncBtn, &QPushButton::clicked, this, &MainWindow::showNonCommutativity);
    lay->addWidget(mx);

    // --- Отображение ---
    auto* vis = new QGroupBox("Отображение и освещение");
    auto* vl = new QGridLayout(vis);
    auto mk = [&](const QString& text, bool on) {
        auto* c = new QCheckBox(text);
        c->setChecked(on);
        connect(c, &QCheckBox::toggled, this, &MainWindow::optionsChanged);
        return c;
    };
    fill_ = mk("Заливка (иначе каркас)", true);
    edges_ = mk("Рёбра треугольников", true);
    cull_ = mk("Back-Face Culling", true);
    zbuf_ = mk("Z-буфер", true);
    light_ = mk("Освещение (Ламберт)", true);
    groups_ = mk("Цвета по типам граней", true);
    showAxis_ = mk("Ось P1P2", true);
    showWorldAxes_ = mk("Оси координат", true);
    vl->addWidget(fill_, 0, 0);
    vl->addWidget(edges_, 0, 1);
    vl->addWidget(cull_, 1, 0);
    vl->addWidget(zbuf_, 1, 1);
    vl->addWidget(light_, 2, 0);
    vl->addWidget(groups_, 2, 1);
    vl->addWidget(showAxis_, 3, 0);
    vl->addWidget(showWorldAxes_, 3, 1);
    const double lp[3] = {scene_.options.light.x, scene_.options.light.y, scene_.options.light.z};
    vl->addWidget(new QLabel("Источник света:"), 4, 0, 1, 2);
    vl->addWidget(xyzRow(lightPos_, -2000, 2000, lp, 20, 0), 5, 0, 1, 2);
    for (auto* b : lightPos_) connect(b, &QDoubleSpinBox::valueChanged, this, [this] { optionsChanged(); });
    lay->addWidget(vis);

    // --- Камера ---
    auto* cam = new QGroupBox("Камера (перспектива)");
    auto* cl = new QHBoxLayout(cam);
    fov_ = spin(10, 120, scene_.camera.fovDeg, 1, 0);
    near_ = spin(0.1, 100, scene_.camera.zNear, 0.5, 1);
    far_ = spin(100, 20000, scene_.camera.zFar, 100, 0);
    cl->addWidget(new QLabel("FOV°"));
    cl->addWidget(fov_);
    cl->addWidget(new QLabel("Near"));
    cl->addWidget(near_);
    cl->addWidget(new QLabel("Far"));
    cl->addWidget(far_);
    auto* camReset = new QPushButton("Сброс");
    cl->addWidget(camReset);
    for (auto* b : {fov_, near_, far_})
        connect(b, &QDoubleSpinBox::valueChanged, this, [this] { optionsChanged(); });
    connect(camReset, &QPushButton::clicked, this, [this] {
        scene_.resetCamera();
        const QSignalBlocker b1(fov_), b2(near_), b3(far_);
        fov_->setValue(scene_.camera.fovDeg);
        near_->setValue(scene_.camera.zNear);
        far_->setValue(scene_.camera.zFar);
        refreshAll();
    });
    lay->addWidget(cam);
    lay->addStretch(1);
    return panel;
}

void MainWindow::generate() {
    scene_.generate(seed_->value(), layers_->value());
    scene_.camera.target = {0, 0, scene_.mesh.params.H / 2};
    refreshAll();
    statusBar()->showMessage(QString("Сгенерирована деталь для Seed = %1").arg(seed_->value()), 4000);
}

void MainWindow::randomSeed() {
    seed_->setValue(static_cast<int>(QRandomGenerator::global()->bounded(100000)));
    generate();
}

void MainWindow::applyTranslation() {
    const double dx = t_[0]->value(), dy = t_[1]->value(), dz = t_[2]->value();
    scene_.apply(m3d::translation(dx, dy, dz),
                 QString("T(%1, %2, %3)").arg(dx).arg(dy).arg(dz).toStdString());
    refreshAll();
}

void MainWindow::applyScaling() {
    const double sx = s_[0]->value(), sy = s_[1]->value(), sz = s_[2]->value();
    if (std::fabs(sx * sy * sz) < 1e-9) {
        QMessageBox::warning(this, "Масштабирование", "Коэффициенты не должны быть равны нулю.");
        return;
    }
    scene_.apply(m3d::scaling(sx, sy, sz), QString("S(%1, %2, %3)").arg(sx).arg(sy).arg(sz).toStdString());
    refreshAll();
}

void MainWindow::applyAxisRotation() {
    const double a = rotAngle_->value();
    const int axis = rotAxis_->currentIndex();
    const Mat4 r = axis == 0 ? m3d::rotationX(a) : (axis == 1 ? m3d::rotationY(a) : m3d::rotationZ(a));
    const char* names[3] = {"Rx", "Ry", "Rz"};
    scene_.apply(r, QString("%1(%2°)").arg(QString::fromUtf8(names[axis])).arg(a).toStdString());
    refreshAll();
}

bool MainWindow::readAxis(Vec3& p1, Vec3& p2) {
    p1 = {p1_[0]->value(), p1_[1]->value(), p1_[2]->value()};
    p2 = {p2_[0]->value(), p2_[1]->value(), p2_[2]->value()};
    return m3d::length(p2 - p1) > 1e-9;
}

void MainWindow::applyLineRotation() {
    Vec3 p1, p2;
    if (!readAxis(p1, p2)) {
        QMessageBox::warning(this, "Ось вращения", "Точки P1 и P2 совпадают — ось не задана.");
        return;
    }
    const double a = lineAngle_->value();
    scene_.apply(m3d::rotationAroundLine(p1, p2, a), QString("R_P1P2(%1°)").arg(a).toStdString());
    refreshAll();
}

void MainWindow::resetTransform() {
    animate_->setChecked(false);
    scene_.resetTransform();
    refreshAll();
}

void MainWindow::toggleAnimation(bool on) {
    if (on) {
        Vec3 p1, p2;
        if (!readAxis(p1, p2)) {
            animate_->setChecked(false);
            return;
        }
        animAngle_ = 0;
        clock_.start();
        timer_->start();
    } else {
        timer_->stop();
        if (std::fabs(animAngle_) > 1e-9) {
            scene_.history.push_back(
                QString("R_P1P2(%1°) — анимация").arg(std::fmod(animAngle_, 360.0), 0, 'f', 1).toStdString());
            refreshAll();
        }
        animAngle_ = 0;
    }
}

void MainWindow::animationStep() {
    const double dt = clock_.restart() / 1000.0;
    const double a = speed_->value() * dt;
    animAngle_ += a;
    scene_.applySilently(m3d::rotationAroundLine(scene_.p1, scene_.p2, a));
    refreshAll();
}

void MainWindow::showNonCommutativity() {
    const Mat4 T = m3d::translation(t_[0]->value(), t_[1]->value(), t_[2]->value());
    const double a = rotAngle_->value();
    const int axis = rotAxis_->currentIndex();
    const Mat4 R = axis == 0 ? m3d::rotationX(a) : (axis == 1 ? m3d::rotationY(a) : m3d::rotationZ(a));
    const Mat4 TR = T * R, RT = R * T;
    const Vec3 q{100, 0, 0};
    const Vec3 q1 = m3d::transformPoint(TR, q), q2 = m3d::transformPoint(RT, q);
    const QString rName = QString("R%1(%2°)").arg(QString("xyz")[axis]).arg(a);
    QString html = "<p>Перенос T и поворот " + rName + " из полей панели «Преобразования».</p>";
    html += "<p><b>T · R</b> (сначала поворот, затем перенос):</p>" + matrixHtml(TR);
    html += "<p><b>R · T</b> (сначала перенос, затем поворот):</p>" + matrixHtml(RT);
    html += QString("<p>Точка (100, 0, 0): T·R → (%1, %2, %3), R·T → (%4, %5, %6)</p>")
                .arg(num(q1.x, 2), num(q1.y, 2), num(q1.z, 2), num(q2.x, 2), num(q2.y, 2), num(q2.z, 2));
    const double d = m3d::maxAbsDiff(TR, RT);
    html += d > 1e-9 ? QString("<p><b style='color:#b3261e'>max |T·R − R·T| = %1 — матрицы различны, "
                               "умножение некоммутативно.</b></p>").arg(num(d))
                     : "<p><b>Матрицы совпали</b> (например, при нулевом переносе или угле).</p>";
    QMessageBox box(this);
    box.setWindowTitle("Проверка некоммутативности");
    box.setTextFormat(Qt::RichText);
    box.setText(html);
    box.exec();
}

void MainWindow::optionsChanged() {
    auto& o = scene_.options;
    o.fill = fill_->isChecked();
    o.edges = edges_->isChecked();
    o.cull = cull_->isChecked();
    o.zbuffer = zbuf_->isChecked();
    o.lighting = light_->isChecked();
    o.groupColors = groups_->isChecked();
    o.light = {lightPos_[0]->value(), lightPos_[1]->value(), lightPos_[2]->value()};
    scene_.showAxisLine = showAxis_->isChecked();
    scene_.showWorldAxes = showWorldAxes_->isChecked();
    Vec3 p1, p2;
    if (readAxis(p1, p2)) {
        scene_.p1 = p1;
        scene_.p2 = p2;
    }
    scene_.camera.fovDeg = fov_->value();
    scene_.camera.zNear = near_->value();
    scene_.camera.zFar = std::max(far_->value(), near_->value() + 1);
    refreshAll();
}

void MainWindow::refreshAll() {
    refreshInfo();
    refreshMatrix();
    for (auto* v : views_) v->update();
}

void MainWindow::refreshInfo() {
    const auto& p = scene_.mesh.params;
    paramsLabel_->setText(
        QString("N = <b>%1</b> зубьев, M = <b>%2</b> (отверстие)<br>"
                "R<sub>out</sub> = %3, R<sub>in</sub> = %4, r<sub>hole</sub> = %5<br>"
                "Δx = %6, Δy = %7, H = %8<br>"
                "α<sub>twist</sub> = %9°, k<sub>taper</sub> = %10")
            .arg(p.N).arg(p.M)
            .arg(num(p.rOut, 1), num(p.rIn, 2), num(p.rHole, 2), num(p.dx, 2), num(p.dy, 2), num(p.H, 2),
                 num(p.twistDeg, 2), num(p.taper, 3)));

    const auto& st = scene_.stats;
    const bool ok = st.euler == 0;
    topoLabel_->setText(
        QString("V = <b>%1</b>, E = <b>%2</b>, F = <b>%3</b><br>"
                "V − E + F = %1 − %2 + %3 = <b style='color:%4'>%5 %6</b> (2 − 2g, g = 1)<br>"
                "Замкнутость (каждое ребро — в 2 гранях): <b style='color:%7'>%8</b>")
            .arg(st.V).arg(st.E).arg(st.F)
            .arg(QString(ok ? "#16794a" : "#b3261e")).arg(st.euler).arg(QString(ok ? "✓" : "✗"))
            .arg(QString(st.closedManifold ? "#16794a" : "#b3261e"))
            .arg(QString(st.closedManifold ? "да ✓" : "нет ✗")));
    checksumLabel_->setText(
        QString("Центр масс C = (<b>%1</b>; <b>%2</b>; <b>%3</b>)<br>"
                "Площадь поверхности S = <b>%4</b><br>"
                "det M<sub>3×3</sub> = %5%6<br>"
                "<span style='color:#6b7280'>Кадр перспективы: %7 мс, камера: d = %8, FOV = %9°</span>")
            .arg(num(st.centroid.x), num(st.centroid.y), num(st.centroid.z), num(st.area, 2), num(st.det, 4))
            .arg(QString(st.det < 0 ? " — <b>зеркальное отражение</b> (нормали развёрнуты)" : ""))
            .arg(num(views_[0] ? views_[0]->lastRenderMs() : 0, 1))
            .arg(num(scene_.camera.distance, 0))
            .arg(num(scene_.camera.fovDeg, 0)));
}

void MainWindow::refreshMatrix() {
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j) matrix_->item(i, j)->setText(num(scene_.model(i, j), 4));
    if (history_->count() != static_cast<int>(scene_.history.size())) {
        history_->clear();
        int k = 1;
        for (const auto& h : scene_.history)
            history_->addItem(QString("%1. %2").arg(k++).arg(QString::fromStdString(h)));
        history_->scrollToBottom();
    }
}
