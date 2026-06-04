#include "ChordWidget.h"
#include "ChordPreview.h"
#include "LatexGenerator.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QPushButton>
#include <QToolButton>
#include <QFileDialog>
#include <QClipboard>
#include <QApplication>
#include <QMessageBox>
#include <QProcess>
#include <QDir>
#include <QPainter>
#include <QMouseEvent>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QDesktopServices>
#include <QUrl>

// ============================================================
//  FretCell
// ============================================================
FretCell::FretCell(int stringIdx, int fretRow, QWidget *parent)
    : QWidget(parent), m_string(stringIdx), m_fret(fretRow)
{
    setFixedSize(28, 28);
    setCursor(Qt::PointingHandCursor);
    setToolTip("Klicken zum Durchschalten: leer → Ton (●) → Grundton (◎) → gedämpft (×)");
}

void FretCell::setState(NoteState s) {
    m_state = s;
    update();
    emit stateChanged(m_string, m_fret, m_state);
}

void FretCell::mousePressEvent(QMouseEvent *e) {
    if (e->button() != Qt::LeftButton) { QWidget::mousePressEvent(e); return; }

    // Zustandsfolge auf allen Zeilen gleich: None → Tone → Root → Mute → None
    switch (m_state) {
    case NoteState::None: setState(NoteState::Tone); break;
    case NoteState::Tone: setState(NoteState::Root); break;
    case NoteState::Root: setState(NoteState::Mute); break;
    default:              setState(NoteState::None); break;
    }
}

void FretCell::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QColor bg = palette().window().color();
    p.fillRect(rect(), bg);

    // Leichte Hintergrundmarkierung für Bund 0 (Leerstring-Zeile)
    if (m_fret == 0) {
        p.fillRect(rect(), QColor(245, 245, 255));
    }

    QPointF center(width()/2.0, height()/2.0);
    double r = 9.0;

    switch (m_state) {
    case NoteState::None:
        // kleines graues Kreuz als Orientierungshilfe
        p.setPen(QPen(QColor(200,200,200), 1));
        p.drawLine(QPointF(center.x()-4, center.y()), QPointF(center.x()+4, center.y()));
        p.drawLine(QPointF(center.x(), center.y()-4), QPointF(center.x(), center.y()+4));
        break;

    case NoteState::Tone:
        p.setPen(QPen(Qt::black, 1));
        p.setBrush(Qt::black);
        p.drawEllipse(center, r, r);
        break;

    case NoteState::Root:
        p.setPen(QPen(Qt::black, 2));
        p.setBrush(Qt::white);
        p.drawEllipse(center, r, r);
        // innerer Punkt zur Unterscheidung
        p.setBrush(QColor(180,0,0));
        p.setPen(Qt::NoPen);
        p.drawEllipse(center, r*0.35, r*0.35);
        break;

    case NoteState::Mute: {
        p.setPen(QPen(Qt::black, 2));
        double d = r * 0.75;
        p.drawLine(QPointF(center.x()-d, center.y()-d), QPointF(center.x()+d, center.y()+d));
        p.drawLine(QPointF(center.x()-d, center.y()+d), QPointF(center.x()+d, center.y()-d));
        break;
    }
    default: break;
    }
}

// ============================================================
//  ChordWidget
// ============================================================
ChordWidget::ChordWidget(int numStrings, const QVector<QString> &tuning, QWidget *parent)
    : QWidget(parent), m_numStrings(numStrings), m_tuning(tuning)
{
    m_outputDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(6, 6, 6, 6);
    mainLayout->setSpacing(8);

    // ---- Linke Seite: Griffbrett-Grid ----
    auto *gridGroup = new QGroupBox("Griffbrett");
    auto *gridOuterLayout = new QVBoxLayout(gridGroup);

    // Saiten-Header
    auto *headerLayout = new QHBoxLayout();
    headerLayout->addSpacing(30); // Platz für Zeilenbeschriftung
    for (int s = 0; s < m_numStrings; ++s) {
        auto *lbl = new QLabel(s < m_tuning.size() ? m_tuning[s] : "?");
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setFixedWidth(28);
        QFont f = lbl->font(); f.setBold(true); lbl->setFont(f);
        headerLayout->addWidget(lbl);
    }
    gridOuterLayout->addLayout(headerLayout);

    // Zellen-Grid
    auto *cellGrid = new QGridLayout();
    cellGrid->setSpacing(2);
    m_cells.resize(m_numStrings);
    const QStringList rowLabels = {"0", "1", "2", "3", "4"};

    for (int f = 0; f <= ChordData::NUM_FRETS; ++f) {
        auto *rowLbl = new QLabel(rowLabels.value(f, "?"));
        rowLbl->setFixedWidth(22);
        rowLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (f == 0) {
            rowLbl->setToolTip("Leerstring: offen oder abgedämpft");
            rowLbl->setStyleSheet("color: #666;");
        }
        cellGrid->addWidget(rowLbl, f, 0);

        for (int s = 0; s < m_numStrings; ++s) {
            if (f == 0) m_cells[s].resize(ChordData::NUM_FRETS + 1);
            auto *cell = new FretCell(s, f, this);
            connect(cell, &FretCell::stateChanged, this, &ChordWidget::onCellChanged);
            m_cells[s][f] = cell;
            cellGrid->addWidget(cell, f, s + 1);
        }
    }
    gridOuterLayout->addLayout(cellGrid);

    // Barré-Steuerung
    auto *barreLayout = new QHBoxLayout();
    m_barreCheck   = new QCheckBox("Barré");
    m_barreFretSpin = new QSpinBox(); m_barreFretSpin->setRange(1, 4); m_barreFretSpin->setPrefix("Bund ");
    m_barreFromSpin = new QSpinBox(); m_barreFromSpin->setRange(0, m_numStrings-1); m_barreFromSpin->setPrefix("von ");
    m_barreToSpin   = new QSpinBox(); m_barreToSpin->setRange(0, m_numStrings-1);
    m_barreToSpin->setValue(m_numStrings - 1);                          m_barreToSpin->setPrefix("bis ");
    barreLayout->addWidget(m_barreCheck);
    barreLayout->addWidget(m_barreFretSpin);
    barreLayout->addWidget(m_barreFromSpin);
    barreLayout->addWidget(m_barreToSpin);
    barreLayout->addStretch();
    gridOuterLayout->addLayout(barreLayout);

    // Reset-Button
    auto *resetBtn = new QPushButton("Alle zurücksetzen");
    connect(resetBtn, &QPushButton::clicked, this, &ChordWidget::clearAll);
    gridOuterLayout->addWidget(resetBtn);

    mainLayout->addWidget(gridGroup);

    // ---- Rechte Seite: Optionen + Vorschau + Export ----
    auto *rightLayout = new QVBoxLayout();

    // Akkord-Name
    auto *nameGroup = new QGroupBox("Akkord-Name");
    auto *nameLayout = new QFormLayout(nameGroup);
    m_rootEdit       = new QLineEdit("C");
    m_rootEdit->setMaximumWidth(40);
    m_accidentalCombo = new QComboBox();
    m_accidentalCombo->addItems({"", "#", "b"});
    m_accidentalCombo->setMaximumWidth(50);
    m_suffixEdit     = new QLineEdit();
    m_suffixEdit->setPlaceholderText("m, maj7, sus2 …");
    m_showNameCheck  = new QCheckBox("anzeigen");
    m_showNameCheck->setChecked(true);

    auto *rootRow = new QHBoxLayout();
    rootRow->addWidget(m_rootEdit);
    rootRow->addWidget(m_accidentalCombo);
    rootRow->addWidget(m_suffixEdit);
    nameLayout->addRow("Grundton:", rootRow);
    nameLayout->addRow("", m_showNameCheck);
    rightLayout->addWidget(nameGroup);

    // Startbund
    auto *fretGroup = new QGroupBox("Lage");
    auto *fretLayout = new QFormLayout(fretGroup);
    m_startFretSpin = new QSpinBox(); m_startFretSpin->setRange(1, 24);
    m_showStartFretCheck = new QCheckBox("Lage anzeigen");
    m_showStartFretCheck->setChecked(false);
    fretLayout->addRow("Startbund:", m_startFretSpin);
    fretLayout->addRow("",          m_showStartFretCheck);
    rightLayout->addWidget(fretGroup);

    // Vorschau
    auto *previewGroup = new QGroupBox("Vorschau");
    auto *previewLayout = new QVBoxLayout(previewGroup);
    m_preview = new ChordPreview();
    previewLayout->addWidget(m_preview);
    rightLayout->addWidget(previewGroup);

    // Export
    auto *exportGroup = new QGroupBox("Export");
    auto *exportLayout = new QVBoxLayout(exportGroup);
    auto *texBtn   = new QPushButton("Speichern als .tex …");
    auto *pdfBtn   = new QPushButton("Kompilieren → PDF …");
    auto *copyBtn  = new QPushButton("TikZ in Zwischenablage");
    auto *dirBtn   = new QPushButton("Ausgabe-Verzeichnis …");
    exportLayout->addWidget(texBtn);
    exportLayout->addWidget(pdfBtn);
    exportLayout->addWidget(copyBtn);
    exportLayout->addWidget(dirBtn);
    m_statusLabel = new QLabel();
    m_statusLabel->setWordWrap(true);
    exportLayout->addWidget(m_statusLabel);
    rightLayout->addWidget(exportGroup);

    rightLayout->addStretch();
    mainLayout->addLayout(rightLayout);

    // --- Signale verdrahten ---
    auto refresh = [this]{ updatePreview(); emit chordChanged(); };
    connect(m_rootEdit,           &QLineEdit::textChanged,    this, refresh);
    connect(m_accidentalCombo,    &QComboBox::currentTextChanged, this, refresh);
    connect(m_suffixEdit,         &QLineEdit::textChanged,    this, refresh);
    connect(m_showNameCheck,      &QCheckBox::toggled,        this, refresh);
    connect(m_startFretSpin,      &QSpinBox::valueChanged,    this, refresh);
    connect(m_showStartFretCheck, &QCheckBox::toggled,        this, refresh);
    connect(m_barreCheck,         &QCheckBox::toggled,        this, [this](bool b){ onBarreToggled(b); updatePreview(); });
    connect(m_barreFretSpin,      &QSpinBox::valueChanged,    this, refresh);
    connect(m_barreFromSpin,      &QSpinBox::valueChanged,    this, refresh);
    connect(m_barreToSpin,        &QSpinBox::valueChanged,    this, refresh);

    connect(texBtn,  &QPushButton::clicked, this, &ChordWidget::onExportTex);
    connect(pdfBtn,  &QPushButton::clicked, this, &ChordWidget::onCompilePdf);
    connect(copyBtn, &QPushButton::clicked, this, &ChordWidget::onCopyTikz);
    connect(dirBtn,  &QPushButton::clicked, this, [this]{
        QString p = QFileDialog::getExistingDirectory(this, "Ausgabe-Verzeichnis", m_outputDir);
        if (!p.isEmpty()) m_outputDir = p;
    });

    updatePreview();
}

// ---------------------------------------------------------------------------
void ChordWidget::onCellChanged(int string, int fret, NoteState state)
{
    // Nur eine Note pro Saite im gegriffenen Bereich (Bünde 1-4)
    if (fret >= 1 && state != NoteState::None) {
        for (int f = 1; f <= ChordData::NUM_FRETS; ++f) {
            if (f != fret) m_cells[string][f]->blockSignals(true),
                           m_cells[string][f]->reset(),
                           m_cells[string][f]->blockSignals(false);
        }
    }
    syncBarreFromCells();
    updatePreview();
    emit chordChanged();
}

void ChordWidget::syncBarreFromCells()
{
    // Einfache Auto-Erkennung: 3+ Saiten am gleichen Bund mit Ton/Grundton
    if (m_barreCheck->isChecked()) return; // manuell gesetzt

    for (int f = 1; f <= ChordData::NUM_FRETS; ++f) {
        int first = -1, last = -1, count = 0;
        for (int s = 0; s < m_numStrings; ++s) {
            NoteState st = m_cells[s][f]->state();
            if (st == NoteState::Tone || st == NoteState::Root) {
                if (first < 0) first = s;
                last = s;
                ++count;
            }
        }
        if (count >= 3 && (last - first + 1) == count) {
            // Zusammenhängende Saiten → Barré vorschlagen
            m_barreCheck->blockSignals(true);
            m_barreCheck->setChecked(true);
            m_barreFretSpin->setValue(f);
            m_barreFromSpin->setValue(first);
            m_barreToSpin->setValue(last);
            m_barreCheck->blockSignals(false);
            return;
        }
    }
}

void ChordWidget::onBarreToggled(bool checked)
{
    m_barreFretSpin->setEnabled(checked);
    m_barreFromSpin->setEnabled(checked);
    m_barreToSpin->setEnabled(checked);
}

// ---------------------------------------------------------------------------
ChordData ChordWidget::currentChord() const
{
    ChordData cd;
    cd.numStrings    = m_numStrings;
    cd.tuning        = m_tuning;
    cd.rootNote      = m_rootEdit->text().trimmed() + m_accidentalCombo->currentText();
    cd.suffix        = m_suffixEdit->text().trimmed();
    cd.showName      = m_showNameCheck->isChecked();
    cd.startFret     = m_startFretSpin->value();
    cd.showStartFret = m_showStartFretCheck->isChecked();
    cd.barre.active      = m_barreCheck->isChecked();
    cd.barre.fret        = m_barreFretSpin->value();
    cd.barre.firstString = m_barreFromSpin->value();
    cd.barre.lastString  = m_barreToSpin->value();
    cd.init();
    for (int s = 0; s < m_numStrings; ++s)
        for (int f = 0; f <= ChordData::NUM_FRETS; ++f)
            cd.notes[s][f] = m_cells[s][f]->state();
    return cd;
}

void ChordWidget::loadChord(const ChordData &chord)
{
    m_rootEdit->blockSignals(true);
    // Root + Vorzeichen trennen
    QString root = chord.rootNote;
    if (root.endsWith('#'))      { m_accidentalCombo->setCurrentText("#"); root.chop(1); }
    else if (root.endsWith('b')){ m_accidentalCombo->setCurrentText("b"); root.chop(1); }
    else                         { m_accidentalCombo->setCurrentText(""); }
    m_rootEdit->setText(root);
    m_rootEdit->blockSignals(false);

    m_suffixEdit->setText(chord.suffix);
    m_showNameCheck->setChecked(chord.showName);
    m_startFretSpin->setValue(chord.startFret);
    m_showStartFretCheck->setChecked(chord.showStartFret);
    m_barreCheck->setChecked(chord.barre.active);
    m_barreFretSpin->setValue(chord.barre.fret);
    m_barreFromSpin->setValue(chord.barre.firstString);
    m_barreToSpin->setValue(chord.barre.lastString);

    for (int s = 0; s < m_numStrings && s < chord.notes.size(); ++s)
        for (int f = 0; f <= ChordData::NUM_FRETS && f < chord.notes[s].size(); ++f) {
            m_cells[s][f]->blockSignals(true);
            m_cells[s][f]->setState(chord.notes[s][f]);
            m_cells[s][f]->blockSignals(false);
        }
    updatePreview();
}

void ChordWidget::clearAll()
{
    for (int s = 0; s < m_numStrings; ++s)
        for (int f = 0; f <= ChordData::NUM_FRETS; ++f) {
            m_cells[s][f]->blockSignals(true);
            m_cells[s][f]->reset();
            m_cells[s][f]->blockSignals(false);
        }
    m_barreCheck->setChecked(false);
    updatePreview();
    emit chordChanged();
}

void ChordWidget::updatePreview()
{
    m_preview->setChord(currentChord());
}

// ---------------------------------------------------------------------------
void ChordWidget::onExportTex()
{
    ChordData cd = currentChord();
    QString defaultName = cd.fullName().isEmpty() ? "chord" : cd.fullName();
    defaultName.replace(' ', '_').replace('#', "sharp").replace('/', '-');
    QString path = QFileDialog::getSaveFileName(
        this, "TeX-Datei speichern", m_outputDir + "/" + defaultName + ".tex",
        "TeX-Dateien (*.tex)");
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_statusLabel->setText("❌ Datei konnte nicht geöffnet werden.");
        return;
    }
    QTextStream out(&f);
    out << LatexGenerator::standaloneDocument(cd);
    m_statusLabel->setText("✓ Gespeichert: " + QFileInfo(path).fileName());
}

void ChordWidget::onCompilePdf()
{
    ChordData cd = currentChord();
    QString defaultName = cd.fullName().isEmpty() ? "chord" : cd.fullName();
    defaultName.replace(' ', '_').replace('#', "sharp").replace('/', '-');
    QString pdfPath = QFileDialog::getSaveFileName(
        this, "PDF speichern", m_outputDir + "/" + defaultName + ".pdf",
        "PDF (*.pdf)");
    if (pdfPath.isEmpty()) return;

    QTemporaryDir tmpDir;
    if (!tmpDir.isValid()) { m_statusLabel->setText("❌ Temp-Verzeichnis Fehler."); return; }

    QString texPath = tmpDir.filePath("chord.tex");
    {
        QFile f(texPath);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            m_statusLabel->setText("❌ Temp-Datei Fehler.");
            return;
        }
        QTextStream out(&f);
        out << LatexGenerator::standaloneDocument(cd);
    }

    m_statusLabel->setText("⏳ Kompiliere …");
    QApplication::processEvents();

    QProcess proc;
    proc.setWorkingDirectory(tmpDir.path());
    proc.start("pdflatex", {"-interaction=nonstopmode", "-halt-on-error", texPath});
    proc.waitForFinished(15000);

    if (proc.exitCode() != 0) {
        QString errOut = proc.readAllStandardOutput() + proc.readAllStandardError();
        QMessageBox::critical(this, "pdflatex Fehler",
            "pdflatex schlug fehl. Ausgabe:\n\n" + errOut.right(1200));
        m_statusLabel->setText("❌ Kompilierung fehlgeschlagen.");
        return;
    }

    QString tmpPdf = tmpDir.filePath("chord.pdf");
    QFile::remove(pdfPath);
    if (!QFile::copy(tmpPdf, pdfPath)) {
        m_statusLabel->setText("❌ PDF konnte nicht kopiert werden.");
        return;
    }

    m_statusLabel->setText("✓ PDF: " + QFileInfo(pdfPath).fileName());
    QDesktopServices::openUrl(QUrl::fromLocalFile(pdfPath));
}

void ChordWidget::onCopyTikz()
{
    QString tikz = LatexGenerator::tikzFragment(currentChord());
    QApplication::clipboard()->setText(tikz);
    m_statusLabel->setText("✓ TikZ in Zwischenablage kopiert.");
}
