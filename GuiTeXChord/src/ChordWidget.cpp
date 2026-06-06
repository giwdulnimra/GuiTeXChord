#include "ChordWidget.h"
#include "ChordPreview.h"
#include "LatexGenerator.h"

#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QPushButton>
#include <QRadioButton>
#include <QButtonGroup>
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
#include <QUrl>
#include <QFileInfo>
#include <QScrollArea>
#include <QLineEdit>

// ============================================================
// Chromatic helper
// ============================================================
static const QStringList CHROMATIC = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","H"};
static QString normalise(const QString &n){
    if(n=="B"||n=="Bb") return "A#";
    if(n=="Eb")          return "D#";
    if(n=="Ab")          return "G#";
    if(n=="Db")          return "C#";
    if(n=="Gb")          return "F#";
    return n;
}
QString ChordWidget::applyDowntune(const QString &note, int semitones)
{
    if(semitones==0) return note;
    QString base=note; QString suf;
    if(base.endsWith('\'')){ suf="'"; base.chop(1); }
    bool lower=(!base.isEmpty() && base[0].isLower());
    QString norm=normalise(lower ? base.toUpper() : base);
    int idx=CHROMATIC.indexOf(norm);
    if(idx<0) return note;
    int ni=((idx+semitones)%12+12)%12;
    QString res=CHROMATIC[ni];
    return (lower?res.toLower():res)+suf;
}

bool ChordWidget::checkLatex()
{
    QProcess p; p.start("pdflatex",{"--version"});
    p.waitForFinished(3000);
    return p.exitCode()==0;
}

// ============================================================
// FretCell
// ============================================================
FretCell::FretCell(int si,int fr,QWidget *parent)
    :QWidget(parent),m_string(si),m_fret(fr)
{ setFixedSize(26,26); setCursor(Qt::PointingHandCursor); }

void FretCell::setState(NoteState s){ m_state=s; update(); emit stateChanged(m_string,m_fret,m_state); }

void FretCell::mousePressEvent(QMouseEvent *e){
    if(e->button()!=Qt::LeftButton){QWidget::mousePressEvent(e);return;}
    switch(m_state){
    case NoteState::None: setState(NoteState::Tone); break;
    case NoteState::Tone: setState(NoteState::Root); break;
    case NoteState::Root: setState(NoteState::Mute); break;
    default:              setState(NoteState::None); break;
    }
}
void FretCell::paintEvent(QPaintEvent*){
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(),palette().window().color());
    QPointF c(width()/2.0,height()/2.0); double r=8.0;
    switch(m_state){
    case NoteState::None:
        p.setPen(QPen(QColor(200,200,200),1));
        p.drawLine(QPointF(c.x()-4,c.y()),QPointF(c.x()+4,c.y()));
        p.drawLine(QPointF(c.x(),c.y()-4),QPointF(c.x(),c.y()+4)); break;
    case NoteState::Tone:
        p.setPen(QPen(Qt::black,1)); p.setBrush(Qt::black); p.drawEllipse(c,r,r); break;
    case NoteState::Root:
        p.setPen(QPen(Qt::black,2)); p.setBrush(Qt::white); p.drawEllipse(c,r,r);
        p.setPen(Qt::NoPen); p.setBrush(Qt::black); p.drawEllipse(c,r*0.38,r*0.38);
        p.setPen(QPen(Qt::black,1.5)); p.setBrush(Qt::NoBrush); break;
    case NoteState::Mute:{
        p.setPen(QPen(Qt::black,2)); double d=r*0.75;
        p.drawLine(QPointF(c.x()-d,c.y()-d),QPointF(c.x()+d,c.y()+d));
        p.drawLine(QPointF(c.x()-d,c.y()+d),QPointF(c.x()+d,c.y()-d)); break;}
    default: break;
    }
}

// ============================================================
// ChordWidget
// ============================================================
ChordWidget::ChordWidget(int numStrings, const QVector<QString> &tuning, QWidget *parent)
    : QWidget(parent), m_numStrings(numStrings),
      m_tuning(tuning), m_currentTuning(tuning)
{
    m_outputDir=QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    m_latexOk=checkLatex();

    // ── Top-level: two columns ──────────────────────────────────────────────
    auto *topLayout = new QHBoxLayout(this);
    topLayout->setContentsMargins(4,4,4,4);
    topLayout->setSpacing(6);

    // ── LEFT COLUMN ─────────────────────────────────────────────────────────
    auto *leftLayout = new QVBoxLayout();
    leftLayout->setSpacing(4);

    // -- Griffbrett GroupBox --
    m_gridGroup = new QGroupBox("Fretboard");
    auto *gridGroupLayout = new QVBoxLayout(m_gridGroup);
    gridGroupLayout->setSpacing(3);

    // Orientation ComboBox
    auto *orientRow = new QHBoxLayout();
    m_orientCombo = new QComboBox();
    m_orientCombo->addItem("Vertical");
    m_orientCombo->addItem("Horizontal");
    orientRow->addWidget(m_orientCombo);
    orientRow->addStretch();
    gridGroupLayout->addLayout(orientRow);

    // Grid container (rebuilt on orientation change)
    m_gridContainer = new QWidget();
    gridGroupLayout->addWidget(m_gridContainer);

    // Barré row
    auto *barreOuterRow = new QHBoxLayout();
    m_barreCheck = new QCheckBox("Barré");
    barreOuterRow->addWidget(m_barreCheck);
    // Radio buttons container – rebuilt together with grid
    m_barreRow = new QWidget();
    m_barreGroup = new QButtonGroup(this);
    barreOuterRow->addWidget(m_barreRow,1);
    gridGroupLayout->addLayout(barreOuterRow);

    // Reset button
    m_resetBtn = new QPushButton("Reset  [Ctrl+Z]");
    m_resetBtn->setShortcut(QKeySequence("Ctrl+Z"));
    connect(m_resetBtn,&QPushButton::clicked,this,&ChordWidget::clearAll);
    gridGroupLayout->addWidget(m_resetBtn);

    leftLayout->addWidget(m_gridGroup);

    // -- Chord Name --
    auto *nameGroup = new QGroupBox("Chord Name");
    auto *nameForm  = new QFormLayout(nameGroup);
    nameForm->setSpacing(4);

    m_rootCombo = new QComboBox();
    m_rootCombo->addItems({"C","D","E","F","G","A","B"});
    m_rootCombo->setMaximumWidth(50);

    m_accCombo = new QComboBox();
    m_accCombo->addItems({"","#","b"});
    m_accCombo->setMaximumWidth(46);

    m_suffixCombo = new QComboBox();
    m_suffixCombo->setEditable(true);
    m_suffixCombo->addItems({"","m","maj7","m7","7","sus2","sus4","dim",
                              "add9","madd9","maj9","m9","11","13"});
    m_suffixCombo->lineEdit()->setPlaceholderText("m, maj7, sus2 …");

    m_showNameCheck = new QCheckBox("Show chord name");
    m_showNameCheck->setChecked(true);

    auto *rootRow = new QHBoxLayout();
    rootRow->setSpacing(3);
    rootRow->addWidget(m_rootCombo);
    rootRow->addWidget(m_accCombo);
    rootRow->addWidget(m_suffixCombo,1);
    nameForm->addRow("Root:", rootRow);
    nameForm->addRow("", m_showNameCheck);
    leftLayout->addWidget(nameGroup);

    // -- Lage & Tuning --
    auto *posGroup = new QGroupBox("Position & Tuning");
    auto *posForm  = new QFormLayout(posGroup);
    posForm->setSpacing(4);

    m_startFretSpin = new QSpinBox();
    m_startFretSpin->setRange(1,24);
    m_showPosCheck = new QCheckBox("Show position");

    m_downtuneCombo = new QComboBox();
    m_downtuneCombo->addItem("Std", 0);
    for(int i=1;i<=8;++i) m_downtuneCombo->addItem(QString("-%1").arg(i), -i);

    auto *posRow = new QHBoxLayout();
    posRow->setSpacing(6);
    posRow->addWidget(new QLabel("Start fret:"));
    posRow->addWidget(m_startFretSpin);
    posRow->addSpacing(8);
    posRow->addWidget(new QLabel("Tuning:"));
    posRow->addWidget(m_downtuneCombo);
    posRow->addStretch();
    posForm->addRow(posRow);
    posForm->addRow("", m_showPosCheck);
    leftLayout->addWidget(posGroup);
    leftLayout->addStretch();

    topLayout->addLayout(leftLayout, 3);

    // ── RIGHT COLUMN ────────────────────────────────────────────────────────
    auto *rightLayout = new QVBoxLayout();
    rightLayout->setSpacing(4);

    // Preview
    auto *previewGroup = new QGroupBox("Preview");
    auto *prevLayout   = new QVBoxLayout(previewGroup);
    m_preview = new ChordPreview();
    prevLayout->addWidget(m_preview);
    rightLayout->addWidget(previewGroup,3);

    // Export
    auto *exportGroup  = new QGroupBox("Export");
    auto *exportLayout = new QVBoxLayout(exportGroup);
    exportLayout->setSpacing(4);

    auto *dirRow = new QHBoxLayout();
    auto *dirBtn = new QPushButton("Open Output Dir");
    m_outDirLabel = new QLabel(m_outputDir);
    m_outDirLabel->setWordWrap(false);
    m_outDirLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_outDirLabel->setStyleSheet("color:#555;font-size:9px;");
    dirRow->addWidget(dirBtn);
    dirRow->addWidget(m_outDirLabel,1);

    m_pdfBtn  = new QPushButton("Compile PDF  [Ctrl+P]");
    m_texBtn  = new QPushButton("Save .tex  [Ctrl+S]");
    m_copyBtn = new QPushButton("Copy TikZ-Code  [Ctrl+C]");
    m_pdfBtn->setShortcut( QKeySequence("Ctrl+P"));
    m_texBtn->setShortcut( QKeySequence("Ctrl+S"));
    m_copyBtn->setShortcut(QKeySequence("Ctrl+C"));

    if(!m_latexOk){
        m_pdfBtn->setEnabled(false);
        m_pdfBtn->setToolTip("pdflatex not found – see About > Options");
    }

    exportLayout->addLayout(dirRow);
    exportLayout->addWidget(m_pdfBtn);
    exportLayout->addWidget(m_texBtn);
    exportLayout->addWidget(m_copyBtn);
    m_statusLabel = new QLabel();
    m_statusLabel->setWordWrap(true);
    exportLayout->addWidget(m_statusLabel);
    rightLayout->addWidget(exportGroup,2);

    // Help panel
    auto *helpGroup  = new QGroupBox("Help");
    auto *helpLayout = new QGridLayout(helpGroup);
    helpLayout->setSpacing(2);
    auto addHelpRow=[&](int row,const QString &sym,const QString &desc){
        auto *s=new QLabel(sym); s->setAlignment(Qt::AlignCenter);
        s->setStyleSheet("font-weight:bold;");
        auto *d=new QLabel(desc);
        helpLayout->addWidget(s,row,0);
        helpLayout->addWidget(d,row,1);
    };
    helpLayout->addWidget(new QLabel("<b>Symbol</b>"),0,0);
    helpLayout->addWidget(new QLabel("<b>Meaning</b>"),0,1);
    addHelpRow(1,"○","Root note");
    addHelpRow(2,"●","Other note");
    addHelpRow(3,"×","Muted string");
    helpLayout->setColumnStretch(1,1);

    auto *funcLabel=new QLabel(
        "<b>Compile PDF</b> – generates PDF via pdflatex<br>"
        "<b>Save .tex</b> – saves LaTeX source file<br>"
        "<b>Copy TikZ</b> – copies tikzpicture to clipboard<br>"
        "&nbsp;&nbsp;(paste into your LaTeX document)");
    funcLabel->setWordWrap(true);
    funcLabel->setTextFormat(Qt::RichText);
    helpLayout->addWidget(funcLabel,4,0,1,2);
    rightLayout->addWidget(helpGroup,2);

    topLayout->addLayout(rightLayout,2);

    // ── Signals ─────────────────────────────────────────────────────────────
    auto refresh=[this]{ updatePreview(); emit chordChanged(); };
    connect(m_orientCombo,   QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ChordWidget::onOrientationChanged);
    connect(m_rootCombo,     QOverload<int>::of(&QComboBox::currentIndexChanged), this, refresh);
    connect(m_accCombo,      QOverload<int>::of(&QComboBox::currentIndexChanged), this, refresh);
    connect(m_suffixCombo,   &QComboBox::currentTextChanged, this, refresh);
    connect(m_showNameCheck, &QCheckBox::toggled, this, refresh);
    connect(m_startFretSpin, &QSpinBox::valueChanged, this, refresh);
    connect(m_showPosCheck,  &QCheckBox::toggled, this, refresh);
    connect(m_downtuneCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int){ onGlobalDowntuneChanged(m_downtuneCombo->currentData().toInt()); });
    connect(m_barreCheck,    &QCheckBox::toggled, this, &ChordWidget::onBarreToggled);
    connect(m_pdfBtn,        &QPushButton::clicked, this, &ChordWidget::onCompilePdf);
    connect(m_texBtn,        &QPushButton::clicked, this, &ChordWidget::onExportTex);
    connect(m_copyBtn,       &QPushButton::clicked, this, &ChordWidget::onCopyTikz);
    connect(dirBtn,          &QPushButton::clicked, this, [this]{
        QString np=QFileDialog::getExistingDirectory(this,"Output Directory",m_outputDir);
        if(!np.isEmpty()){ m_outputDir=np; m_outDirLabel->setText(np); }
    });

    // Build initial grid
    rebuildGrid();
    updatePreview();
}

// ---------------------------------------------------------------------------
// Grid rebuild (called on construction and orientation change)
// ---------------------------------------------------------------------------
void ChordWidget::rebuildGrid()
{
    // Clear old layout/cells
    delete m_gridContainer->layout();
    for(auto &row:m_cells) for(auto *c:row) c->deleteLater();
    m_cells.clear();

    // Clear barré radios
    for(auto *r:m_barreRadios){ m_barreGroup->removeButton(r); r->deleteLater(); }
    m_barreRadios.clear();
    delete m_barreRow->layout();

    m_cells.resize(m_numStrings);
    for(auto &row:m_cells) row.resize(ChordData::NUM_FRETS+1);

    if(m_orientation==Orientation::Vertical){
        // ── Vertical: Saiten=Spalten, Bünde=Zeilen ───────────────────────
        // Grid: col 0=fret labels, col 1..N=strings
        // Row 0=header(string names), rows 1..NUM_FRETS+1=fret rows
        // Barré radios: left column, rows 1..NUM_FRETS+1

        auto *gl = new QGridLayout(m_gridContainer);
        gl->setSpacing(2);

        // String name headers (col 1..N)
        // Vertical: E(idx 0) leftmost col 1, e'(idx N-1) rightmost col N
        for(int s=0;s<m_numStrings;++s){
            auto *lbl=new QLabel(m_currentTuning[s]);
            lbl->setAlignment(Qt::AlignCenter);
            lbl->setFixedWidth(26);
            QFont f=lbl->font(); f.setBold(true); lbl->setFont(f);
            gl->addWidget(lbl,0,s+1);
        }

        // Fret row labels + cells
        // fretRow 0 = open, fretRow 1..NUM_FRETS = startFret..startFret+NUM_FRETS-1
        int sf=m_startFretSpin ? m_startFretSpin->value() : 1;
        QStringList fretLabels; fretLabels<<"open";
        for(int f=1;f<=ChordData::NUM_FRETS;++f) fretLabels<<QString::number(sf+f-1);

        for(int f=0;f<=ChordData::NUM_FRETS;++f){
            auto *lbl=new QLabel(fretLabels[f]);
            lbl->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
            lbl->setFixedWidth(32);
            gl->addWidget(lbl,f+1,0);
            for(int s=0;s<m_numStrings;++s){
                auto *cell=new FretCell(s,f,m_gridContainer);
                connect(cell,&FretCell::stateChanged,this,&ChordWidget::onCellChanged);
                m_cells[s][f]=cell;
                gl->addWidget(cell,f+1,s+1);
            }
        }

        // Barré radios: one per fretRow 1..NUM_FRETS, laid out horizontally
        // matching the fret columns (col 1..NUM_FRETS+1 offset by open col)
        auto *brl=new QHBoxLayout(m_barreRow);
        brl->setSpacing(0); brl->setContentsMargins(0,0,0,0);
        // Spacer for open column label width
        brl->addSpacing(38);
        for(int f=1;f<=ChordData::NUM_FRETS;++f){
            auto *rb=new QRadioButton();
            rb->setFixedSize(26,20);
            m_barreGroup->addButton(rb,f);
            m_barreRadios.append(rb);
            brl->addWidget(rb);
        }
        brl->addStretch();
        if(!m_barreRadios.isEmpty()) m_barreRadios[0]->setChecked(true);

    } else {
        // ── Horizontal: Saiten=Zeilen, Bünde=Spalten ─────────────────────
        // Row 0=header(fret labels), col 0=string name labels, cols 1..NUM_FRETS+1=frets
        // Barré radios: right of each fret row (after the cells)

        auto *gl=new QGridLayout(m_gridContainer);
        gl->setSpacing(2);

        // Fret column headers
        int sf=m_startFretSpin ? m_startFretSpin->value() : 1;
        QStringList fretLabels; fretLabels<<"open";
        for(int f=1;f<=ChordData::NUM_FRETS;++f) fretLabels<<QString::number(sf+f-1);

        for(int f=0;f<=ChordData::NUM_FRETS;++f){
            auto *lbl=new QLabel(fretLabels[f]);
            lbl->setAlignment(Qt::AlignCenter);
            QFont fl=lbl->font(); fl.setBold(true); lbl->setFont(fl);
            gl->addWidget(lbl,0,f+1);
        }

        // String rows: e'(idx N-1) top, E(idx 0) bottom
        for(int row=0;row<m_numStrings;++row){
            int s=m_numStrings-1-row; // s: N-1 downto 0
            auto *lbl=new QLabel(m_currentTuning[s]);
            lbl->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
            lbl->setFixedWidth(26);
            QFont sf2=lbl->font(); sf2.setBold(true); lbl->setFont(sf2);
            gl->addWidget(lbl,row+1,0);
            for(int f=0;f<=ChordData::NUM_FRETS;++f){
                auto *cell=new FretCell(s,f,m_gridContainer);
                connect(cell,&FretCell::stateChanged,this,&ChordWidget::onCellChanged);
                m_cells[s][f]=cell;
                gl->addWidget(cell,row+1,f+1);
            }
        }

        // Barré radios: one per fret-row position, in a vertical layout
        // placed in a column next to the grid via the horizontal barreRow layout
        auto *brl=new QVBoxLayout(m_barreRow);
        brl->setSpacing(2); brl->setContentsMargins(2,0,0,0);
        brl->addSpacing(22); // skip header row
        for(int f=1;f<=ChordData::NUM_FRETS;++f){
            auto *rb=new QRadioButton();
            rb->setFixedHeight(26);
            m_barreGroup->addButton(rb,f);
            m_barreRadios.append(rb);
            brl->addWidget(rb);
        }
        brl->addStretch();
        if(!m_barreRadios.isEmpty()) m_barreRadios[0]->setChecked(true);
    }

    // Barré radio connections
    connect(m_barreGroup,QOverload<int>::of(&QButtonGroup::idClicked),
            this,[this](int id){
                if(m_barreCheck->isChecked()){
                    updatePreview(); emit chordChanged();
                }
            });

    onBarreToggled(m_barreCheck ? m_barreCheck->isChecked() : false);
    updatePreview();
}

// ---------------------------------------------------------------------------
void ChordWidget::onCellChanged(int str, int fret, NoteState state)
{
    if(state!=NoteState::None){
        for(int f=0;f<=ChordData::NUM_FRETS;++f){
            if(f!=fret){
                m_cells[str][f]->blockSignals(true);
                m_cells[str][f]->reset();
                m_cells[str][f]->blockSignals(false);
            }
        }
    }
    updatePreview(); emit chordChanged();
}

void ChordWidget::onBarreToggled(bool checked)
{
    for(auto *r:m_barreRadios) r->setEnabled(checked);
    updatePreview(); emit chordChanged();
}

void ChordWidget::onOrientationChanged(int idx)
{
    m_orientation = (idx==0) ? Orientation::Vertical : Orientation::Horizontal;
    rebuildGrid();
    updatePreview(); emit chordChanged();
}

void ChordWidget::onGlobalDowntuneChanged(int semitones)
{
    m_globalDowntune=semitones;
    for(int s=0;s<m_numStrings;++s)
        m_currentTuning[s]=applyDowntune(m_tuning[s],semitones);
    // Rebuild grid to refresh string labels
    rebuildGrid();
    updatePreview(); emit chordChanged();
}

void ChordWidget::updatePreview(){ if(m_preview) m_preview->setChord(currentChord()); }

// ---------------------------------------------------------------------------
ChordData ChordWidget::currentChord() const
{
    ChordData cd;
    cd.numStrings    = m_numStrings;
    cd.tuning        = m_currentTuning;
    cd.rootNote      = m_rootCombo->currentText()+m_accCombo->currentText();
    cd.suffix        = m_suffixCombo->currentText();
    cd.showName      = m_showNameCheck->isChecked();
    cd.startFret     = m_startFretSpin->value();
    cd.showStartFret = m_showPosCheck->isChecked();
    cd.orientation   = m_orientation;

    // Barré
    cd.barre.active = m_barreCheck->isChecked();
    int barFret = m_barreGroup->checkedId();
    cd.barre.fret = (barFret>0) ? barFret : 1;
    // Derive string range from notes on that fret
    int first=-1, last=-1;
    for(int s=0;s<m_numStrings;++s){
        if(!m_cells[s].isEmpty() && m_cells[s][cd.barre.fret]){
            NoteState st=m_cells[s][cd.barre.fret]->state();
            if(st==NoteState::Tone||st==NoteState::Root){
                if(first<0) first=s; last=s;
            }
        }
    }
    cd.barre.firstString = (first>=0) ? first : 0;
    cd.barre.lastString  = (last>=0)  ? last  : m_numStrings-1;

    cd.init();
    for(int s=0;s<m_numStrings;++s)
        for(int f=0;f<=ChordData::NUM_FRETS;++f)
            if(!m_cells[s].isEmpty() && m_cells[s][f])
                cd.notes[s][f]=m_cells[s][f]->state();
    return cd;
}

void ChordWidget::clearAll()
{
    for(int s=0;s<m_numStrings;++s)
        for(int f=0;f<=ChordData::NUM_FRETS;++f)
            if(!m_cells[s].isEmpty()&&m_cells[s][f]){
                m_cells[s][f]->blockSignals(true);
                m_cells[s][f]->reset();
                m_cells[s][f]->blockSignals(false);
            }
    m_barreCheck->setChecked(false);
    updatePreview(); emit chordChanged();
}

void ChordWidget::loadChord(const ChordData &chord)
{
    // Root
    int ri=m_rootCombo->findText(chord.rootNote.left(1));
    if(ri>=0) m_rootCombo->setCurrentIndex(ri);
    QString acc=chord.rootNote.mid(1);
    int ai=m_accCombo->findText(acc); if(ai>=0) m_accCombo->setCurrentIndex(ai);
    m_suffixCombo->setCurrentText(chord.suffix);
    m_showNameCheck->setChecked(chord.showName);
    m_startFretSpin->setValue(chord.startFret);
    m_showPosCheck->setChecked(chord.showStartFret);
    m_barreCheck->setChecked(chord.barre.active);

    for(int s=0;s<m_numStrings&&s<chord.notes.size();++s)
        for(int f=0;f<=ChordData::NUM_FRETS&&f<chord.notes[s].size();++f)
            if(!m_cells[s].isEmpty()&&m_cells[s][f]){
                m_cells[s][f]->blockSignals(true);
                m_cells[s][f]->setState(chord.notes[s][f]);
                m_cells[s][f]->blockSignals(false);
            }
    updatePreview();
}

void ChordWidget::retranslate(bool english)
{
    m_english=english;
    if(english){
        m_gridGroup->setTitle("Fretboard");
        m_pdfBtn->setText("Compile PDF  [Ctrl+P]");
        m_texBtn->setText("Save .tex  [Ctrl+S]");
        m_copyBtn->setText("Copy TikZ-Code  [Ctrl+C]");
        m_resetBtn->setText("Reset  [Ctrl+Z]");
        m_showNameCheck->setText("Show chord name");
        m_showPosCheck->setText("Show position");
    } else {
        m_gridGroup->setTitle("Griffbrett");
        m_pdfBtn->setText("PDF kompilieren  [Strg+P]");
        m_texBtn->setText(".tex speichern  [Strg+S]");
        m_copyBtn->setText("TikZ-Code kopieren  [Strg+C]");
        m_resetBtn->setText("Zurücksetzen  [Strg+Z]");
        m_showNameCheck->setText("Akkordname anzeigen");
        m_showPosCheck->setText("Lage anzeigen");
    }
}

// ---------------------------------------------------------------------------
// Export
// ---------------------------------------------------------------------------
void ChordWidget::onExportTex()
{
    ChordData cd=currentChord();
    QString dn=cd.fullName().isEmpty()?"chord":cd.fullName();
    dn.replace(' ','_').replace('#',"sharp").replace('/','_');
    QString path=QFileDialog::getSaveFileName(this,
        m_english?"Save TeX file":"TeX-Datei speichern",
        m_outputDir+"/"+dn+".tex","TeX (*.tex)");
    if(path.isEmpty()) return;
    QFile f(path); if(!f.open(QIODevice::WriteOnly|QIODevice::Text)){
        m_statusLabel->setText("❌ Cannot open file."); return; }
    QTextStream(&f)<<LatexGenerator::standaloneDocument(cd);
    m_statusLabel->setText("✓ "+QFileInfo(path).fileName());
}

void ChordWidget::onCompilePdf()
{
    if(!m_latexOk){
        QMessageBox::warning(this,"pdflatex not found",
            "pdflatex was not found on this system.\n"
            "Please install a LaTeX distribution (e.g. MiKTeX, TeX Live)\n"
            "and restart the application.");
        return;
    }
    ChordData cd=currentChord();
    QString dn=cd.fullName().isEmpty()?"chord":cd.fullName();
    dn.replace(' ','_').replace('#',"sharp").replace('/','_');
    QString pdfPath=QFileDialog::getSaveFileName(this,
        m_english?"Save PDF":"PDF speichern",
        m_outputDir+"/"+dn+".pdf","PDF (*.pdf)");
    if(pdfPath.isEmpty()) return;

    QTemporaryDir tmp; if(!tmp.isValid()){m_statusLabel->setText("❌ Temp error.");return;}
    QString texPath=tmp.filePath("chord.tex");
    {QFile f(texPath);f.open(QIODevice::WriteOnly|QIODevice::Text);
     QTextStream(&f)<<LatexGenerator::standaloneDocument(cd);}

    m_statusLabel->setText("⏳ Compiling…"); QApplication::processEvents();
    QProcess proc; proc.setWorkingDirectory(tmp.path());
    proc.start("pdflatex",{"-interaction=nonstopmode","-halt-on-error",texPath});
    proc.waitForFinished(15000);
    if(proc.exitCode()!=0){
        QMessageBox::critical(this,"pdflatex error",
            proc.readAllStandardOutput().right(1200));
        m_statusLabel->setText("❌ Compile failed."); return;
    }
    QFile::remove(pdfPath);
    if(!QFile::copy(tmp.filePath("chord.pdf"),pdfPath)){
        m_statusLabel->setText("❌ Copy failed."); return;}
    m_statusLabel->setText("✓ PDF: "+QFileInfo(pdfPath).fileName());
    QDesktopServices::openUrl(QUrl::fromLocalFile(pdfPath));
}

void ChordWidget::onCopyTikz()
{
    QApplication::clipboard()->setText(LatexGenerator::tikzFragment(currentChord()));
    m_statusLabel->setText(m_english
        ? "✓ TikZ copied. Paste into your LaTeX document."
        : "✓ TikZ kopiert. In LaTeX-Dokument einfügen.");
}
