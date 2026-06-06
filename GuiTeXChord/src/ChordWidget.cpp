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
#include <QFileDialog>
#include <QClipboard>
#include <QApplication>
#include <QMessageBox>
#include <QProcess>
#include <QPainter>
#include <QMouseEvent>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QDesktopServices>
#include <QKeySequence>
#include <QShortcut>
#include <QUrl>
#include <QFileInfo>

// ============================================================
//  Downtune helper
// ============================================================
// Note names in chromatic order (using # only, German H→B)
static const QStringList CHROMATIC = {
    "C","C#","D","D#","E","F","F#","G","G#","A","A#","H"
};
// enharmonic normalisation for lookup
static QString normalise(const QString &n) {
    if (n=="Bb"||n=="B") return "A#";
    if (n=="Eb")          return "D#";
    if (n=="Ab")          return "G#";
    if (n=="Db")          return "C#";
    if (n=="Gb")          return "F#";
    if (n=="B")           return "A#";
    return n;
}
QString ChordWidget::applyDowntune(const QString &note, int semitones)
{
    if (semitones == 0) return note;
    QString norm = normalise(note);
    // strip trailing ' or lowercase indicators for lookup
    QString base = norm;
    QString suffix;
    if (base.endsWith('\'')) { suffix = "'"; base.chop(1); }
    // find in chromatic
    int idx = CHROMATIC.indexOf(base);
    if (idx < 0) {
        // try uppercase
        idx = CHROMATIC.indexOf(base.toUpper());
    }
    if (idx < 0) return note; // unknown, leave as-is
    int newIdx = ((idx + semitones) % 12 + 12) % 12;
    QString result = CHROMATIC[newIdx];
    // preserve lowercase for middle strings (heuristic: if original was lowercase)
    if (note[0].isLower()) result = result.toLower();
    return result + suffix;
}

// ============================================================
//  FretCell
// ============================================================
FretCell::FretCell(int stringIdx, int fretRow, QWidget *parent)
    : QWidget(parent), m_string(stringIdx), m_fret(fretRow)
{
    setFixedSize(28, 28);
    setCursor(Qt::PointingHandCursor);
}

void FretCell::setState(NoteState s) {
    m_state = s;
    update();
    emit stateChanged(m_string, m_fret, m_state);
}

void FretCell::mousePressEvent(QMouseEvent *e) {
    if (e->button() != Qt::LeftButton) { QWidget::mousePressEvent(e); return; }
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
    p.fillRect(rect(), palette().window().color());
    QPointF c(width()/2.0, height()/2.0);
    double r = 9.0;
    switch (m_state) {
    case NoteState::None:
        p.setPen(QPen(QColor(200,200,200), 1));
        p.drawLine(QPointF(c.x()-4,c.y()), QPointF(c.x()+4,c.y()));
        p.drawLine(QPointF(c.x(),c.y()-4), QPointF(c.x(),c.y()+4));
        break;
    case NoteState::Tone:
        p.setPen(QPen(Qt::black,1)); p.setBrush(Qt::black);
        p.drawEllipse(c, r, r);
        break;
    case NoteState::Root:
        p.setPen(QPen(Qt::black,2)); p.setBrush(Qt::white);
        p.drawEllipse(c, r, r);
        p.setPen(Qt::NoPen); p.setBrush(Qt::black);
        p.drawEllipse(c, r*0.38, r*0.38);
        break;
    case NoteState::Mute: {
        p.setPen(QPen(Qt::black,2));
        double d = r*0.75;
        p.drawLine(QPointF(c.x()-d,c.y()-d),QPointF(c.x()+d,c.y()+d));
        p.drawLine(QPointF(c.x()-d,c.y()+d),QPointF(c.x()+d,c.y()-d));
        break;
    }
    default: break;
    }
}

// ============================================================
//  ChordWidget
// ============================================================
ChordWidget::ChordWidget(int numStrings, const QVector<QString> &tuning, QWidget *parent)
    : QWidget(parent), m_numStrings(numStrings), m_tuning(tuning),
      m_currentTuning(tuning), m_downtune(numStrings, 0)
{
    m_outputDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(6,6,6,6);
    mainLayout->setSpacing(8);

    // ===== Linke Seite: Griffbrett =====
    auto *gridGroup = new QGroupBox("Fretboard");
    auto *gridOuterLayout = new QVBoxLayout(gridGroup);

    auto *cellGrid = new QGridLayout();
    cellGrid->setSpacing(2);
    m_cells.resize(m_numStrings);
    m_downtuneSpins.resize(m_numStrings);
    m_stringLabels.resize(m_numStrings);

    // Bund-Header: Zeile 0, Spalten 1..NUM_FRETS+1
    // Spalte 0 = Saiten-Label, Spalte NUM_FRETS+2 = Downtune-Spin
    const QStringList colLabels = {"0","1","2","3","4"};
    cellGrid->addWidget(new QLabel(""), 0, 0);
    for (int f = 0; f <= ChordData::NUM_FRETS; ++f) {
        auto *lbl = new QLabel(colLabels[f]);
        lbl->setAlignment(Qt::AlignCenter); lbl->setFixedWidth(28);
        QFont fl = lbl->font(); fl.setBold(true); lbl->setFont(fl);
        cellGrid->addWidget(lbl, 0, f+1);
    }
    auto *dtHeader = new QLabel("½♭");
    dtHeader->setAlignment(Qt::AlignCenter);
    dtHeader->setToolTip("Downtune: semitones below standard");
    cellGrid->addWidget(dtHeader, 0, ChordData::NUM_FRETS+2);

    // Zeilen: stringIdx N-1 (höchste, e') in Zeile 1 oben,
    //         stringIdx 0   (tiefste, E)  in Zeile N unten
    // → Grid-Zeile g = (N - 1 - stringIdx) + 1
    for (int s = m_numStrings-1; s >= 0; --s) {
        int gridRow = (m_numStrings - 1 - s) + 1; // e' oben
        m_cells[s].resize(ChordData::NUM_FRETS + 1);

        // Saiten-Label
        auto *lbl = new QLabel(m_tuning[s]);
        lbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        lbl->setFixedWidth(26);
        QFont sf = lbl->font(); sf.setBold(true); lbl->setFont(sf);
        m_stringLabels[s] = lbl;
        cellGrid->addWidget(lbl, gridRow, 0);

        // Zellen
        for (int f = 0; f <= ChordData::NUM_FRETS; ++f) {
            auto *cell = new FretCell(s, f, this);
            connect(cell, &FretCell::stateChanged, this, &ChordWidget::onCellChanged);
            m_cells[s][f] = cell;
            cellGrid->addWidget(cell, gridRow, f+1);
        }

        // Downtune-Spin
        auto *spin = new QSpinBox();
        spin->setRange(-9, 0);
        spin->setValue(0);
        spin->setFixedWidth(44);
        spin->setToolTip("Downtune this string (semitones)");
        m_downtuneSpins[s] = spin;
        connect(spin, &QSpinBox::valueChanged, this, [this, s](int v){
            onDowntuneChanged(s, v);
        });
        cellGrid->addWidget(spin, gridRow, ChordData::NUM_FRETS+2);
    }
    gridOuterLayout->addLayout(cellGrid);

    // Barré
    auto *barreLayout = new QHBoxLayout();
    m_barreCheck    = new QCheckBox("Barré");
    m_barreFretSpin = new QSpinBox(); m_barreFretSpin->setRange(1,4); m_barreFretSpin->setPrefix("Fret ");
    m_barreFromSpin = new QSpinBox(); m_barreFromSpin->setRange(0,m_numStrings-1); m_barreFromSpin->setPrefix("from ");
    m_barreToSpin   = new QSpinBox(); m_barreToSpin->setRange(0,m_numStrings-1); m_barreToSpin->setValue(m_numStrings-1); m_barreToSpin->setPrefix("to ");
    barreLayout->addWidget(m_barreCheck);
    barreLayout->addWidget(m_barreFretSpin);
    barreLayout->addWidget(m_barreFromSpin);
    barreLayout->addWidget(m_barreToSpin);
    barreLayout->addStretch();
    gridOuterLayout->addLayout(barreLayout);

    // Reset
    m_resetBtn = new QPushButton("Reset  [Ctrl+Z]");
    m_resetBtn->setShortcut(QKeySequence("Ctrl+Z"));
    connect(m_resetBtn, &QPushButton::clicked, this, &ChordWidget::clearAll);
    gridOuterLayout->addWidget(m_resetBtn);

    mainLayout->addWidget(gridGroup);

    // ===== Rechte Seite =====
    auto *rightLayout = new QVBoxLayout();

    // --- Chord Name ---
    auto *nameGroup = new QGroupBox("Chord Name");
    auto *nameForm  = new QFormLayout(nameGroup);
    m_rootEdit        = new QLineEdit("C"); m_rootEdit->setMaximumWidth(38);
    m_accidentalCombo = new QComboBox(); m_accidentalCombo->addItems({"","#","b"}); m_accidentalCombo->setMaximumWidth(46);
    m_suffixEdit      = new QLineEdit(); m_suffixEdit->setPlaceholderText("m, maj7, sus2 …");
    m_showNameCheck   = new QCheckBox("show"); m_showNameCheck->setChecked(true);
    auto *rootRow = new QHBoxLayout();
    rootRow->addWidget(m_rootEdit); rootRow->addWidget(m_accidentalCombo); rootRow->addWidget(m_suffixEdit);
    nameForm->addRow("Root:", rootRow);
    nameForm->addRow("", m_showNameCheck);
    rightLayout->addWidget(nameGroup);

    // --- Position ---
    auto *posGroup = new QGroupBox("Position");
    auto *posForm  = new QFormLayout(posGroup);
    m_startFretSpin      = new QSpinBox(); m_startFretSpin->setRange(1,24);
    m_showStartFretCheck = new QCheckBox("show position"); m_showStartFretCheck->setChecked(false);
    posForm->addRow("Start fret:", m_startFretSpin);
    posForm->addRow("", m_showStartFretCheck);
    rightLayout->addWidget(posGroup);

    // --- Preview ---
    auto *previewGroup = new QGroupBox("Preview");
    auto *previewLayout = new QVBoxLayout(previewGroup);
    m_preview = new ChordPreview();
    previewLayout->addWidget(m_preview);
    rightLayout->addWidget(previewGroup);

    // --- Export ---
    auto *exportGroup  = new QGroupBox("Export");
    auto *exportLayout = new QVBoxLayout(exportGroup);

    m_texBtn  = new QPushButton("Save .tex  [Ctrl+S]");
    m_pdfBtn  = new QPushButton("Compile PDF  [Ctrl+P]");
    m_copyBtn = new QPushButton("Copy TikZ  [Ctrl+K]");
    m_texBtn->setShortcut(QKeySequence("Ctrl+S"));
    m_pdfBtn->setShortcut(QKeySequence("Ctrl+P"));
    m_copyBtn->setShortcut(QKeySequence("Ctrl+K"));

    auto *dirRow = new QHBoxLayout();
    auto *dirBtn = new QPushButton("Output Dir…");
    m_outputDirLabel = new QLabel(m_outputDir);
    m_outputDirLabel->setWordWrap(true);
    m_outputDirLabel->setStyleSheet("color: #555; font-size: 9px;");
    dirRow->addWidget(dirBtn); dirRow->addWidget(m_outputDirLabel, 1);

    exportLayout->addWidget(m_texBtn);
    exportLayout->addWidget(m_pdfBtn);
    exportLayout->addWidget(m_copyBtn);
    exportLayout->addLayout(dirRow);
    m_statusLabel = new QLabel();
    m_statusLabel->setWordWrap(true);
    exportLayout->addWidget(m_statusLabel);
    rightLayout->addWidget(exportGroup);

    rightLayout->addStretch();
    mainLayout->addLayout(rightLayout);

    // --- Signals ---
    auto refresh = [this]{ updatePreview(); emit chordChanged(); };
    connect(m_rootEdit,           &QLineEdit::textChanged,         this, refresh);
    connect(m_accidentalCombo,    &QComboBox::currentTextChanged,  this, refresh);
    connect(m_suffixEdit,         &QLineEdit::textChanged,         this, refresh);
    connect(m_showNameCheck,      &QCheckBox::toggled,             this, refresh);
    connect(m_startFretSpin,      &QSpinBox::valueChanged,         this, refresh);
    connect(m_showStartFretCheck, &QCheckBox::toggled,             this, refresh);
    connect(m_barreCheck,         &QCheckBox::toggled,             this, [this](bool b){ onBarreToggled(b); updatePreview(); });
    connect(m_barreFretSpin,      &QSpinBox::valueChanged,         this, refresh);
    connect(m_barreFromSpin,      &QSpinBox::valueChanged,         this, refresh);
    connect(m_barreToSpin,        &QSpinBox::valueChanged,         this, refresh);
    connect(m_texBtn,             &QPushButton::clicked,           this, &ChordWidget::onExportTex);
    connect(m_pdfBtn,             &QPushButton::clicked,           this, &ChordWidget::onCompilePdf);
    connect(m_copyBtn,            &QPushButton::clicked,           this, &ChordWidget::onCopyTikz);
    connect(dirBtn,               &QPushButton::clicked,           this, [this]{
        QString p = QFileDialog::getExistingDirectory(this, "Output Directory", m_outputDir);
        if (!p.isEmpty()) { m_outputDir = p; m_outputDirLabel->setText(p); }
    });

    updatePreview();
}

// ---------------------------------------------------------------------------
void ChordWidget::onCellChanged(int string, int fret, NoteState state)
{
    if (state != NoteState::None) {
        for (int f = 0; f <= ChordData::NUM_FRETS; ++f) {
            if (f != fret) {
                m_cells[string][f]->blockSignals(true);
                m_cells[string][f]->reset();
                m_cells[string][f]->blockSignals(false);
            }
        }
    }
    syncBarreFromCells();
    updatePreview();
    emit chordChanged();
}

void ChordWidget::syncBarreFromCells()
{
    if (m_barreCheck->isChecked()) return;
    for (int f = 1; f <= ChordData::NUM_FRETS; ++f) {
        int first=-1, last=-1, count=0;
        for (int s = 0; s < m_numStrings; ++s) {
            NoteState st = m_cells[s][f]->state();
            if (st==NoteState::Tone||st==NoteState::Root) {
                if (first<0) first=s; last=s; ++count;
            }
        }
        if (count>=3 && (last-first+1)==count) {
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

void ChordWidget::onDowntuneChanged(int stringIdx, int semitones)
{
    m_downtune[stringIdx] = semitones;
    m_currentTuning[stringIdx] = applyDowntune(m_tuning[stringIdx], semitones);
    m_stringLabels[stringIdx]->setText(m_currentTuning[stringIdx]);
    updatePreview();
    emit chordChanged();
}

// ---------------------------------------------------------------------------
ChordData ChordWidget::currentChord() const
{
    ChordData cd;
    cd.numStrings     = m_numStrings;
    cd.tuning         = m_currentTuning;
    cd.rootNote       = m_rootEdit->text().trimmed() + m_accidentalCombo->currentText();
    cd.suffix         = m_suffixEdit->text().trimmed();
    cd.showName       = m_showNameCheck->isChecked();
    cd.startFret      = m_startFretSpin->value();
    cd.showStartFret  = m_showStartFretCheck->isChecked();
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
    QString root = chord.rootNote;
    if      (root.endsWith('#')) { m_accidentalCombo->setCurrentText("#"); root.chop(1); }
    else if (root.endsWith('b')) { m_accidentalCombo->setCurrentText("b"); root.chop(1); }
    else                          { m_accidentalCombo->setCurrentText(""); }
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

void ChordWidget::updatePreview() { m_preview->setChord(currentChord()); }

// ---------------------------------------------------------------------------
void ChordWidget::retranslate(bool english)
{
    m_english = english;
    // Buttons
    if (english) {
        m_texBtn->setText("Save .tex  [Ctrl+S]");
        m_pdfBtn->setText("Compile PDF  [Ctrl+P]");
        m_copyBtn->setText("Copy TikZ  [Ctrl+K]");
        m_resetBtn->setText("Reset  [Ctrl+Z]");
    } else {
        m_texBtn->setText("Speichern .tex  [Strg+S]");
        m_pdfBtn->setText("PDF kompilieren  [Strg+P]");
        m_copyBtn->setText("TikZ kopieren  [Strg+K]");
        m_resetBtn->setText("Zurücksetzen  [Strg+Z]");
    }
}

// ---------------------------------------------------------------------------
void ChordWidget::onExportTex()
{
    ChordData cd = currentChord();
    QString defName = cd.fullName().isEmpty() ? "chord" : cd.fullName();
    defName.replace(' ','_').replace('#',"sharp").replace('/','_');
    QString path = QFileDialog::getSaveFileName(
        this,
        m_english ? "Save TeX file" : "TeX-Datei speichern",
        m_outputDir + "/" + defName + ".tex",
        "TeX (*.tex)");
    if (path.isEmpty()) return;
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly|QIODevice::Text)) {
        m_statusLabel->setText("❌ Cannot open file."); return;
    }
    QTextStream(&f) << LatexGenerator::standaloneDocument(cd);
    m_statusLabel->setText("✓ " + QFileInfo(path).fileName());
}

void ChordWidget::onCompilePdf()
{
    ChordData cd = currentChord();
    QString defName = cd.fullName().isEmpty() ? "chord" : cd.fullName();
    defName.replace(' ','_').replace('#',"sharp").replace('/','_');
    QString pdfPath = QFileDialog::getSaveFileName(
        this,
        m_english ? "Save PDF" : "PDF speichern",
        m_outputDir + "/" + defName + ".pdf",
        "PDF (*.pdf)");
    if (pdfPath.isEmpty()) return;

    QTemporaryDir tmp;
    if (!tmp.isValid()) { m_statusLabel->setText("❌ Temp dir error."); return; }
    QString texPath = tmp.filePath("chord.tex");
    { QFile f(texPath); f.open(QIODevice::WriteOnly|QIODevice::Text);
      QTextStream(&f) << LatexGenerator::standaloneDocument(cd); }

    m_statusLabel->setText("⏳ Compiling…");
    QApplication::processEvents();

    QProcess proc;
    proc.setWorkingDirectory(tmp.path());
    proc.start("pdflatex", {"-interaction=nonstopmode", "-halt-on-error", texPath});
    proc.waitForFinished(15000);

    if (proc.exitCode() != 0) {
        QMessageBox::critical(this, "pdflatex error",
            proc.readAllStandardOutput().right(1200));
        m_statusLabel->setText("❌ Compile failed."); return;
    }
    QFile::remove(pdfPath);
    if (!QFile::copy(tmp.filePath("chord.pdf"), pdfPath)) {
        m_statusLabel->setText("❌ Copy failed."); return;
    }
    m_statusLabel->setText("✓ PDF: " + QFileInfo(pdfPath).fileName());
    QDesktopServices::openUrl(QUrl::fromLocalFile(pdfPath));
}

void ChordWidget::onCopyTikz()
{
    QApplication::clipboard()->setText(LatexGenerator::tikzFragment(currentChord()));
    m_statusLabel->setText(m_english
        ? "✓ TikZ copied to clipboard.\n"
          "Paste into your LaTeX document inside \\begin{tikzpicture}…\\end{tikzpicture}."
        : "✓ TikZ in Zwischenablage.\n"
          "Einfügen ins LaTeX-Dokument innerhalb von \\begin{tikzpicture}…");
}
