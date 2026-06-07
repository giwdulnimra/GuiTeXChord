#include "ChordWidget.h"
#include "ChordPreview.h"
#include "LatexGenerator.h"

#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
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
#include <QFile>
#include <QRegularExpression>

// ============================================================
// Helpers
// ============================================================
static const QStringList CHROMATIC={"C","C#","D","D#","E","F","F#","G","G#","A","A#","H"};
static QString normalise(const QString &n){
    if(n=="B"||n=="Bb") return "A#";
    if(n=="Eb") return "D#"; if(n=="Ab") return "G#";
    if(n=="Db") return "C#"; if(n=="Gb") return "F#";
    return n;
}
QString ChordWidget::applyDowntune(const QString &note, int semitones){
    if(semitones==0) return note;
    QString base=note; QString suf;
    if(base.endsWith('\'')){ suf="'"; base.chop(1); }
    bool lower=!base.isEmpty()&&base[0].isLower();
    QString norm=normalise(lower?base.toUpper():base);
    int idx=CHROMATIC.indexOf(norm); if(idx<0) return note;
    int ni=((idx+semitones)%12+12)%12;
    return (lower?CHROMATIC[ni].toLower():CHROMATIC[ni])+suf;
}
bool ChordWidget::checkLatex(){
    QProcess p; p.start("pdflatex",{"--version"});
    p.waitForFinished(3000); return p.exitCode()==0;
}
static QString loadQrc(const QString &path){
    QFile f(path); if(!f.open(QIODevice::ReadOnly|QIODevice::Text)) return {};
    return QString::fromUtf8(f.readAll());
}
static QString mdToHtml(const QString &md){
    QString h=md.toHtmlEscaped();
    h.replace(QRegularExpression(R"(^### (.+)$)",QRegularExpression::MultilineOption),"<h4>\\1</h4>");
    h.replace(QRegularExpression(R"(^## (.+)$)", QRegularExpression::MultilineOption),"<h3>\\1</h3>");
    h.replace(QRegularExpression(R"(^# (.+)$)",  QRegularExpression::MultilineOption),"<h2>\\1</h2>");
    h.replace(QRegularExpression(R"(\*\*(.+?)\*\*)"),"<b>\\1</b>");
    h.replace(QRegularExpression(R"(\*(.+?)\*)"),"<i>\\1</i>");
    h.replace("\n","<br>"); return h;
}

// ============================================================
// FretCell
// ============================================================
FretCell::FretCell(int si,int fr,QWidget *parent)
    :QWidget(parent),m_string(si),m_fret(fr)
{ setFixedSize(26,26); setCursor(Qt::PointingHandCursor); }

void FretCell::setState(NoteState s){
    m_state=s; update(); emit stateChanged(m_string,m_fret,m_state); }

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
        p.setPen(QPen(Qt::black,1)); p.setBrush(Qt::black);
        p.drawEllipse(c,r,r); break;
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

    auto *topLayout=new QHBoxLayout(this);
    topLayout->setContentsMargins(4,4,4,4); topLayout->setSpacing(6);

    // ── LEFT ─────────────────────────────────────────────────────────────────
    auto *leftLayout=new QVBoxLayout(); leftLayout->setSpacing(4);

    m_gridGroup=new QGroupBox("Fretboard");
    auto *gg=new QVBoxLayout(m_gridGroup);
    gg->setSpacing(3); gg->setContentsMargins(4,4,4,4);

    m_orientCombo=new QComboBox();
    m_orientCombo->addItem("Vertical"); m_orientCombo->addItem("Horizontal");
    m_orientCombo->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    gg->addWidget(m_orientCombo);

    m_gridContainer=new QWidget();
    m_gridContainer->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    gg->addWidget(m_gridContainer);

    m_resetBtn=new QPushButton("Reset  [Ctrl+Z]");
    m_resetBtn->setShortcut(QKeySequence("Ctrl+Z"));
    m_resetBtn->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    connect(m_resetBtn,&QPushButton::clicked,this,&ChordWidget::clearAll);
    gg->addWidget(m_resetBtn);

    leftLayout->addWidget(m_gridGroup);

    // Chord Name
    auto *nameGroup=new QGroupBox("Chord Name");
    auto *nf=new QFormLayout(nameGroup); nf->setSpacing(4);
    m_rootCombo=new QComboBox(); m_rootCombo->addItems({"C","D","E","F","G","A","B"}); m_rootCombo->setMaximumWidth(50);
    m_accCombo=new QComboBox();  m_accCombo->addItems({"","#","b"}); m_accCombo->setMaximumWidth(46);
    m_suffixCombo=new QComboBox(); m_suffixCombo->setEditable(true);
    m_suffixCombo->addItems({"","m","maj7","m7","7","sus2","sus4","dim","add9","madd9","maj9","m9","11","13"});
    m_suffixCombo->lineEdit()->setPlaceholderText("m, maj7, sus2 …");
    m_showNameCheck=new QCheckBox("Show chord name"); m_showNameCheck->setChecked(true);
    auto *rr=new QHBoxLayout(); rr->setSpacing(3);
    rr->addWidget(m_rootCombo); rr->addWidget(m_accCombo); rr->addWidget(m_suffixCombo,1);
    nf->addRow("Root:",rr); nf->addRow("",m_showNameCheck);
    leftLayout->addWidget(nameGroup);

    // Position & Tuning
    auto *posGroup=new QGroupBox("Position & Tuning");
    auto *pf=new QFormLayout(posGroup); pf->setSpacing(4);
    m_startFretSpin=new QSpinBox(); m_startFretSpin->setRange(1,24);
    m_showPosCheck=new QCheckBox("Show position");
    m_downtuneCombo=new QComboBox(); m_downtuneCombo->addItem("Std",0);
    for(int i=1;i<=8;++i) m_downtuneCombo->addItem(QString("-%1").arg(i),-i);
    auto *pr=new QHBoxLayout(); pr->setSpacing(6);
    pr->addWidget(new QLabel("Start fret:")); pr->addWidget(m_startFretSpin);
    pr->addSpacing(8);
    pr->addWidget(new QLabel("Tuning:")); pr->addWidget(m_downtuneCombo);
    pr->addStretch();
    pf->addRow(pr); pf->addRow("",m_showPosCheck);
    leftLayout->addWidget(posGroup);
    leftLayout->addStretch();
    topLayout->addLayout(leftLayout,3);

    // ── RIGHT ────────────────────────────────────────────────────────────────
    auto *rightLayout=new QVBoxLayout(); rightLayout->setSpacing(4);

    auto *previewGroup=new QGroupBox("Preview");
    auto *pvl=new QVBoxLayout(previewGroup);
    m_preview=new ChordPreview(); pvl->addWidget(m_preview);
    rightLayout->addWidget(previewGroup,3);

    auto *exportGroup=new QGroupBox("Export");
    auto *exl=new QVBoxLayout(exportGroup); exl->setSpacing(4);
    auto *dirRow=new QHBoxLayout();
    auto *dirBtn=new QPushButton("Open Output Dir");
    m_outDirLabel=new QLabel(m_outputDir);
    m_outDirLabel->setWordWrap(false);
    m_outDirLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_outDirLabel->setStyleSheet("color:#555;font-size:9px;");
    dirRow->addWidget(dirBtn); dirRow->addWidget(m_outDirLabel,1);
    m_pdfBtn=new QPushButton("Compile PDF  [Ctrl+P]");
    m_texBtn=new QPushButton("Save .tex  [Ctrl+S]");
    m_copyBtn=new QPushButton("Copy TikZ-Code  [Ctrl+C]");
    m_pdfBtn->setShortcut(QKeySequence("Ctrl+P"));
    m_texBtn->setShortcut(QKeySequence("Ctrl+S"));
    m_copyBtn->setShortcut(QKeySequence("Ctrl+C"));
    if(!m_latexOk){ m_pdfBtn->setEnabled(false); m_pdfBtn->setToolTip("pdflatex not found"); }
    exl->addLayout(dirRow); exl->addWidget(m_pdfBtn); exl->addWidget(m_texBtn); exl->addWidget(m_copyBtn);
    m_statusLabel=new QLabel(); m_statusLabel->setWordWrap(true);
    exl->addWidget(m_statusLabel);
    rightLayout->addWidget(exportGroup,2);

    auto *helpGroup=new QGroupBox("Help");
    auto *hl=new QVBoxLayout(helpGroup);
    m_helpLabel=new QLabel(); m_helpLabel->setWordWrap(true);
    m_helpLabel->setTextFormat(Qt::RichText); m_helpLabel->setAlignment(Qt::AlignTop|Qt::AlignLeft);
    hl->addWidget(m_helpLabel);
    rightLayout->addWidget(helpGroup,2);

    topLayout->addLayout(rightLayout,2);

    // ── Signals ──────────────────────────────────────────────────────────────
    auto refresh=[this]{ updatePreview(); emit chordChanged(); };
    connect(m_orientCombo,   QOverload<int>::of(&QComboBox::currentIndexChanged),this,&ChordWidget::onOrientationChanged);
    connect(m_rootCombo,     QOverload<int>::of(&QComboBox::currentIndexChanged),this,refresh);
    connect(m_accCombo,      QOverload<int>::of(&QComboBox::currentIndexChanged),this,refresh);
    connect(m_suffixCombo,   &QComboBox::currentTextChanged,this,refresh);
    connect(m_showNameCheck, &QCheckBox::toggled,this,refresh);
    connect(m_startFretSpin, &QSpinBox::valueChanged,this,[this]{ rebuildGrid(); updatePreview(); emit chordChanged(); });
    connect(m_showPosCheck,  &QCheckBox::toggled,this,refresh);
    connect(m_downtuneCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,[this](int){ onGlobalDowntuneChanged(m_downtuneCombo->currentData().toInt()); });
    connect(m_pdfBtn,  &QPushButton::clicked,this,&ChordWidget::onCompilePdf);
    connect(m_texBtn,  &QPushButton::clicked,this,&ChordWidget::onExportTex);
    connect(m_copyBtn, &QPushButton::clicked,this,&ChordWidget::onCopyTikz);
    connect(dirBtn,    &QPushButton::clicked,this,[this]{
        QString np=QFileDialog::getExistingDirectory(this,"Output Directory",m_outputDir);
        if(!np.isEmpty()){ m_outputDir=np; m_outDirLabel->setText(np); }
    });

    rebuildGrid();
    retranslate(true);
    updatePreview();
}

// ============================================================
// rebuildGrid  –  alles in EINEM QGridLayout auf m_gridContainer
// ============================================================
void ChordWidget::rebuildGrid()
{
    // --- Altes Layout + Widgets wegräumen ---
    if(m_gridContainer->layout()){
        QLayoutItem *item;
        while((item=m_gridContainer->layout()->takeAt(0))!=nullptr){
            if(item->widget()) item->widget()->deleteLater();
            delete item;
        }
        delete m_gridContainer->layout();
    }
    for(auto &row:m_cells) for(auto *c:row) if(c) c->deleteLater();
    m_cells.clear();

    for(auto *r:m_barreRadios){
        if(m_barreGroup) m_barreGroup->removeButton(r);
        r->deleteLater();
    }
    m_barreRadios.clear();
    m_barreCheck    = nullptr;
    m_barreFromCombo= nullptr;
    m_barreToCombo  = nullptr;
    if(!m_barreGroup) m_barreGroup = new QButtonGroup(this);

    // --- Init ---
    m_cells.resize(m_numStrings);
    for(auto &row:m_cells) row.resize(ChordData::NUM_FRETS+1, nullptr);

    int sf = m_startFretSpin ? m_startFretSpin->value() : 1;
    QStringList fretLabels; fretLabels << "open";
    for(int f=1;f<=ChordData::NUM_FRETS;++f) fretLabels << QString::number(sf+f-1);

    // Ein einziges Grid für alles
    auto *gl = new QGridLayout(m_gridContainer);
    gl->setSpacing(2);
    gl->setContentsMargins(0,0,0,0);

    if(m_orientation == Orientation::Vertical) {
        // ─────────────────────────────────────────────────────────────────
        // CSV vertikal:
        //   col 0        = Barré-Radio-Spalte  (Zeilen 2..NUM_FRETS+1)
        //   col 1        = Fret-Label-Spalte   (Zeilen 1..NUM_FRETS+1)
        //   col 2..N+1   = Saiten (E=2, e'=N+1)
        //
        //   row 0        = Saiten-Header
        //   row 1        = open-Zeile   (kein Radio, fret 0)
        //   row 2..NF+1  = Bünde 1..NF  (Radio in col 0, Fret-Label in col 1)
        //   row NF+2     = Barré-Checkbox + from/to
        // ─────────────────────────────────────────────────────────────────

        // row 0: Saiten-Header (col 2..N+1)
        for(int s=0;s<m_numStrings;++s){
            auto *lbl=new QLabel(m_currentTuning[s]);
            lbl->setAlignment(Qt::AlignCenter); lbl->setFixedSize(26,18);
            QFont f=lbl->font(); f.setBold(true); lbl->setFont(f);
            gl->addWidget(lbl, 0, s+2);
        }

        // row 1..NUM_FRETS+1: fret rows
        for(int f=0;f<=ChordData::NUM_FRETS;++f){
            int row = f+1;

            // col 0: Radio für Bünde 1..NUM_FRETS; open-Zeile (f=0) bleibt leer
            if(f >= 1){
                auto *rb = new QRadioButton();
                rb->setFixedSize(18, 26);
                m_barreGroup->addButton(rb, f);
                m_barreRadios.append(rb);
                gl->addWidget(rb, row, 0, Qt::AlignCenter);
            }

            // col 1: Fret-Label
            auto *lbl=new QLabel(fretLabels[f]);
            lbl->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
            lbl->setFixedWidth(32);
            gl->addWidget(lbl, row, 1);

            // col 2..N+1: Zellen
            for(int s=0;s<m_numStrings;++s){
                auto *cell=new FretCell(s,f,m_gridContainer);
                connect(cell,&FretCell::stateChanged,this,&ChordWidget::onCellChanged);
                m_cells[s][f]=cell;
                gl->addWidget(cell, row, s+2);
            }
        }

        // row NUM_FRETS+2: Barré-Checkbox + from/to (col 0..N+1)
        int barreRow = ChordData::NUM_FRETS+2;
        m_barreCheck = new QCheckBox("Barré");
        gl->addWidget(m_barreCheck, barreRow, 0, 1, 2, Qt::AlignLeft|Qt::AlignVCenter);

        m_barreFromCombo = new QComboBox(); m_barreFromCombo->setMaximumWidth(65);
        m_barreToCombo   = new QComboBox(); m_barreToCombo->setMaximumWidth(65);
        updateBarreStringCombos();
        gl->addWidget(new QLabel("from:"), barreRow, 2, 1, 1, Qt::AlignRight);
        gl->addWidget(m_barreFromCombo,    barreRow, 3, 1, 2);
        gl->addWidget(new QLabel("to:"),   barreRow, 5, 1, 1, Qt::AlignRight);
        gl->addWidget(m_barreToCombo,      barreRow, 6, 1, 2);
        gl->setColumnStretch(m_numStrings+2, 1);

    } else {
        // ─────────────────────────────────────────────────────────────────
        // CSV horizontal:
        //   col 0          = Saiten-Label
        //   col 1          = open-Spalte   (kein Radio)
        //   col 2..NF+1    = Bünde 1..NF   (Fret-Header row 0, Radio row N+1)
        //
        //   row 0          = Fret-Header
        //   row 1..N       = Saiten (e'=1, E=N)
        //   row N+1        = Barré-Checkbox (col 0..1) + Radios (col 2..NF+1)
        //   row N+2        = from / to
        // ─────────────────────────────────────────────────────────────────

        // row 0: Fret-Header (col 1..NUM_FRETS+1)
        for(int f=0;f<=ChordData::NUM_FRETS;++f){
            auto *lbl=new QLabel(fretLabels[f]);
            lbl->setAlignment(Qt::AlignCenter);
            QFont fl=lbl->font(); fl.setBold(true); lbl->setFont(fl);
            gl->addWidget(lbl, 0, f+1);
        }

        // row 1..N: Saiten (e' oben, E unten)
        for(int row=0;row<m_numStrings;++row){
            int s = m_numStrings-1-row;
            auto *lbl=new QLabel(m_currentTuning[s]);
            lbl->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
            lbl->setFixedWidth(26);
            QFont sf2=lbl->font(); sf2.setBold(true); lbl->setFont(sf2);
            gl->addWidget(lbl, row+1, 0);
            for(int f=0;f<=ChordData::NUM_FRETS;++f){
                auto *cell=new FretCell(s,f,m_gridContainer);
                connect(cell,&FretCell::stateChanged,this,&ChordWidget::onCellChanged);
                m_cells[s][f]=cell;
                gl->addWidget(cell, row+1, f+1);
            }
        }

        // row N+1: Barré-Checkbox (col 0..1) + Radios (col 2..NUM_FRETS+1)
        int barreRow = m_numStrings+1;
        m_barreCheck = new QCheckBox("Barré");
        gl->addWidget(m_barreCheck, barreRow, 0, 1, 2, Qt::AlignLeft|Qt::AlignVCenter);

        for(int f=1;f<=ChordData::NUM_FRETS;++f){
            auto *rb = new QRadioButton();
            rb->setFixedSize(26, 18);
            m_barreGroup->addButton(rb, f);
            m_barreRadios.append(rb);
            gl->addWidget(rb, barreRow, f+1, Qt::AlignCenter);
        }

        // row N+2: from / to
        int fromRow = m_numStrings+2;
        m_barreFromCombo = new QComboBox(); m_barreFromCombo->setMaximumWidth(65);
        m_barreToCombo   = new QComboBox(); m_barreToCombo->setMaximumWidth(65);
        updateBarreStringCombos();
        gl->addWidget(new QLabel("from:"), fromRow, 0, 1, 1, Qt::AlignRight);
        gl->addWidget(m_barreFromCombo,    fromRow, 1, 1, 2);
        gl->addWidget(new QLabel("to:"),   fromRow, 3, 1, 1, Qt::AlignRight);
        gl->addWidget(m_barreToCombo,      fromRow, 4, 1, 2);
        gl->setColumnStretch(ChordData::NUM_FRETS+2, 1);
    }

    if(!m_barreRadios.isEmpty()) m_barreRadios[0]->setChecked(true);

    connect(m_barreCheck, &QCheckBox::toggled, this, &ChordWidget::onBarreToggled);
    connect(m_barreGroup, QOverload<int>::of(&QButtonGroup::idClicked), this, [this](int){
        if(m_barreCheck->isChecked()){ updatePreview(); emit chordChanged(); }
    });
    connect(m_barreFromCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int){ updatePreview(); emit chordChanged(); });
    connect(m_barreToCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int){ updatePreview(); emit chordChanged(); });

    onBarreToggled(m_barreCheck ? m_barreCheck->isChecked() : false);
}

void ChordWidget::updateBarreStringCombos()
{
    if(!m_barreFromCombo || !m_barreToCombo) return;
    m_barreFromCombo->blockSignals(true);
    m_barreToCombo->blockSignals(true);

    int prevFrom = m_barreFromCombo->currentData().toInt();
    int prevTo   = m_barreToCombo->currentData().toInt();

    m_barreFromCombo->clear();
    m_barreToCombo->clear();

    // List strings: show as displayed (e' top for horizontal, E bottom for vertical)
    // Internally always by stringIdx (0=E, N-1=e')
    for(int s=0;s<m_numStrings;++s){
        QString lbl = s < m_currentTuning.size() ? m_currentTuning[s] : QString::number(s);
        m_barreFromCombo->addItem(lbl, s);
        m_barreToCombo->addItem(lbl, s);
    }

    // Restore or set defaults
    int fromIdx = m_barreFromCombo->findData(prevFrom);
    int toIdx   = m_barreToCombo->findData(prevTo);
    m_barreFromCombo->setCurrentIndex(fromIdx >= 0 ? fromIdx : 0);
    m_barreToCombo->setCurrentIndex(toIdx >= 0 ? toIdx : m_numStrings-1);

    m_barreFromCombo->blockSignals(false);
    m_barreToCombo->blockSignals(false);
}

// ============================================================
// Slots
// ============================================================
void ChordWidget::onCellChanged(int str, int fret, NoteState state){
    if(state!=NoteState::None)
        for(int f=0;f<=ChordData::NUM_FRETS;++f)
            if(f!=fret && m_cells[str][f]){
                m_cells[str][f]->blockSignals(true);
                m_cells[str][f]->reset();
                m_cells[str][f]->blockSignals(false);
            }
    updatePreview(); emit chordChanged();
}
void ChordWidget::onBarreToggled(bool checked){
    for(auto *r:m_barreRadios) r->setEnabled(checked);
    if(m_barreFromCombo) m_barreFromCombo->setEnabled(checked);
    if(m_barreToCombo)   m_barreToCombo->setEnabled(checked);
    updatePreview(); emit chordChanged();
}
void ChordWidget::onOrientationChanged(int idx){
    m_orientation=(idx==0)?Orientation::Vertical:Orientation::Horizontal;
    rebuildGrid(); updatePreview(); emit chordChanged();
}
void ChordWidget::onGlobalDowntuneChanged(int semitones){
    m_globalDowntune=semitones;
    for(int s=0;s<m_numStrings;++s)
        m_currentTuning[s]=applyDowntune(m_tuning[s],semitones);
    rebuildGrid(); updatePreview(); emit chordChanged();
}
void ChordWidget::updatePreview(){ if(m_preview) m_preview->setChord(currentChord()); }

// ============================================================
// Data
// ============================================================
ChordData ChordWidget::currentChord() const {
    ChordData cd;
    cd.numStrings   =m_numStrings;
    cd.tuning       =m_currentTuning;
    cd.rootNote     =m_rootCombo->currentText()+m_accCombo->currentText();
    cd.suffix       =m_suffixCombo->currentText();
    cd.showName     =m_showNameCheck->isChecked();
    cd.startFret    =m_startFretSpin->value();
    cd.showStartFret=m_showPosCheck->isChecked();
    cd.orientation  =m_orientation;
    cd.barre.active =m_barreCheck && m_barreCheck->isChecked();
    int bFret = m_barreGroup ? m_barreGroup->checkedId() : 1;
    cd.barre.fret = (bFret>=1) ? bFret : 1;
    cd.barre.firstString = m_barreFromCombo ? m_barreFromCombo->currentData().toInt() : 0;
    cd.barre.lastString  = m_barreToCombo   ? m_barreToCombo->currentData().toInt()   : m_numStrings-1;
    cd.init();
    for(int s=0;s<m_numStrings;++s)
        for(int f=0;f<=ChordData::NUM_FRETS;++f)
            if(m_cells[s][f]) cd.notes[s][f]=m_cells[s][f]->state();
    return cd;
}

void ChordWidget::clearAll(){
    for(int s=0;s<m_numStrings;++s)
        for(int f=0;f<=ChordData::NUM_FRETS;++f)
            if(m_cells[s][f]){
                m_cells[s][f]->blockSignals(true);
                m_cells[s][f]->reset();
                m_cells[s][f]->blockSignals(false);
            }
    if(m_barreCheck) m_barreCheck->setChecked(false);
    updatePreview(); emit chordChanged();
}

void ChordWidget::loadChord(const ChordData &chord){
    int ri=m_rootCombo->findText(chord.rootNote.left(1));
    if(ri>=0) m_rootCombo->setCurrentIndex(ri);
    QString acc=chord.rootNote.mid(1);
    int ai=m_accCombo->findText(acc); if(ai>=0) m_accCombo->setCurrentIndex(ai);
    m_suffixCombo->setCurrentText(chord.suffix);
    m_showNameCheck->setChecked(chord.showName);
    m_startFretSpin->setValue(chord.startFret);
    m_showPosCheck->setChecked(chord.showStartFret);
    if(m_barreCheck) m_barreCheck->setChecked(chord.barre.active);
    for(int s=0;s<m_numStrings&&s<chord.notes.size();++s)
        for(int f=0;f<=ChordData::NUM_FRETS&&f<chord.notes[s].size();++f)
            if(m_cells[s][f]){
                m_cells[s][f]->blockSignals(true);
                m_cells[s][f]->setState(chord.notes[s][f]);
                m_cells[s][f]->blockSignals(false);
            }
    updatePreview();
}

// ============================================================
// Language
// ============================================================
void ChordWidget::retranslate(bool english){
    m_english=english;
    m_gridGroup->setTitle(english?"Fretboard":"Griffbrett");
    m_pdfBtn->setText(english?"Compile PDF  [Ctrl+P]":"PDF kompilieren  [Strg+P]");
    m_texBtn->setText(english?"Save .tex  [Ctrl+S]":".tex speichern  [Strg+S]");
    m_copyBtn->setText(english?"Copy TikZ-Code  [Ctrl+C]":"TikZ-Code kopieren  [Strg+C]");
    m_resetBtn->setText(english?"Reset  [Ctrl+Z]":"Zurücksetzen  [Strg+Z]");
    m_showNameCheck->setText(english?"Show chord name":"Akkordname anzeigen");
    m_showPosCheck->setText(english?"Show position":"Lage anzeigen");
    QString mdPath=english?":/help/help_inline_en.md":":/help/help_inline_de.md";
    m_helpLabel->setText(mdToHtml(loadQrc(mdPath)));
}

// ============================================================
// Export
// ============================================================
void ChordWidget::onExportTex(){
    ChordData cd=currentChord();
    QString dn=cd.fullName().isEmpty()?"chord":cd.fullName();
    dn.replace(' ','_').replace('#',"sharp").replace('/','_');
    QString path=QFileDialog::getSaveFileName(this,
        m_english?"Save TeX file":"TeX-Datei speichern",
        m_outputDir+"/"+dn+".tex","TeX (*.tex)");
    if(path.isEmpty()) return;
    QFile f(path); if(!f.open(QIODevice::WriteOnly|QIODevice::Text)){
        m_statusLabel->setText("❌ Cannot open file."); return;}
    QTextStream(&f)<<LatexGenerator::standaloneDocument(cd);
    m_statusLabel->setText("✓ "+QFileInfo(path).fileName());
}

void ChordWidget::onCompilePdf(){
    if(!m_latexOk){
        QMessageBox::warning(this,"pdflatex not found",
            "pdflatex was not found.\nInstall MiKTeX (Windows) or texlive-pictures (Linux).");
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
        QMessageBox::critical(this,"pdflatex error",proc.readAllStandardOutput().right(1200));
        m_statusLabel->setText("❌ Compile failed."); return;
    }
    QFile::remove(pdfPath);
    if(!QFile::copy(tmp.filePath("chord.pdf"),pdfPath)){
        m_statusLabel->setText("❌ Copy failed."); return;}
    m_statusLabel->setText("✓ PDF: "+QFileInfo(pdfPath).fileName());
    QDesktopServices::openUrl(QUrl::fromLocalFile(pdfPath));
}

void ChordWidget::onCopyTikz(){
    QApplication::clipboard()->setText(LatexGenerator::tikzFragment(currentChord()));
    m_statusLabel->setText(m_english
        ?"✓ TikZ copied. Paste into your LaTeX document."
        :"✓ TikZ kopiert. In LaTeX-Dokument einfügen.");
}
