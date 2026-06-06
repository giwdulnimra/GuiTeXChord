#include "MainWindow.h"
#include "ChordWidget.h"

#include <QTabWidget>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QMessageBox>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    resize(900, 640);

    m_tabs = new QTabWidget(this);
    m_tabs->setTabPosition(QTabWidget::North);

    struct TuningDef { int n; QVector<QString> t; QString name; };
    const QVector<TuningDef> tunings = {
        { 6, {"E","A","d","g","h","e'"}, "6-Str. Standard" },
        { 6, {"D","A","d","g","h","e'"}, "6-Str. Drop-D"   },
        { 7, {"B","E","A","d","g","h","e'"}, "7-Str. Standard" },
        { 7, {"A","E","A","d","g","h","e'"}, "7-Str. Drop-A"   },
        { 8, {"F#","B","E","A","d","g","h","e'"}, "8-Str. Standard" },
    };
    for (const auto &td : tunings)
        m_tabs->addTab(new ChordWidget(td.n, td.t), td.name);

    setCentralWidget(m_tabs);
    setupMenu();
    setLanguage(true);
}

// ---------------------------------------------------------------------------
void MainWindow::setupMenu()
{
    // ── File ──
    auto *fileMenu = menuBar()->addMenu("&File");

    auto *actTex = fileMenu->addAction("Save .tex\tCtrl+S");
    connect(actTex, &QAction::triggered, this, [this]{
        if (auto *cw = qobject_cast<ChordWidget*>(m_tabs->currentWidget()))
            cw->onExportTex();
    });

    auto *actPdf = fileMenu->addAction("Compile PDF\tCtrl+P");
    connect(actPdf, &QAction::triggered, this, [this]{
        if (auto *cw = qobject_cast<ChordWidget*>(m_tabs->currentWidget()))
            cw->onCompilePdf();
    });

    auto *actCopy = fileMenu->addAction("Copy TikZ\tCtrl+K");
    connect(actCopy, &QAction::triggered, this, [this]{
        if (auto *cw = qobject_cast<ChordWidget*>(m_tabs->currentWidget()))
            cw->onCopyTikz();
    });

    fileMenu->addSeparator();
    auto *actClear = fileMenu->addAction("Reset chord\tCtrl+Z");
    connect(actClear, &QAction::triggered, this, [this]{
        if (auto *cw = qobject_cast<ChordWidget*>(m_tabs->currentWidget()))
            cw->clearAll();
    });

    fileMenu->addSeparator();
    fileMenu->addAction("&Quit", this, &QWidget::close, QKeySequence::Quit);

    // ── About ──
    auto *aboutMenu = menuBar()->addMenu("&About");

    auto *actHelp = aboutMenu->addAction("&Help");
    connect(actHelp, &QAction::triggered, this, &MainWindow::showHelp);

    auto *actOptions = aboutMenu->addAction("Options");
    actOptions->setEnabled(false); // vorerst deaktiviert

    aboutMenu->addSeparator();

    // Sprach-Toggle (checkable, mutually exclusive)
    auto *langGroup = new QActionGroup(this);
    langGroup->setExclusive(true);

    m_actEnglish = aboutMenu->addAction("English");
    m_actEnglish->setCheckable(true);
    m_actEnglish->setChecked(true);
    langGroup->addAction(m_actEnglish);

    m_actDeutsch = aboutMenu->addAction("Deutsch");
    m_actDeutsch->setCheckable(true);
    langGroup->addAction(m_actDeutsch);

    connect(m_actEnglish, &QAction::triggered, this, [this]{ setLanguage(true);  });
    connect(m_actDeutsch, &QAction::triggered, this, [this]{ setLanguage(false); });
}

// ---------------------------------------------------------------------------
void MainWindow::setLanguage(bool english)
{
    m_english = english;
    setWindowTitle(english ? "Chord Diagram Generator" : "Grifftabellen-Generator");

    // Alle Tabs updaten
    for (int i = 0; i < m_tabs->count(); ++i)
        if (auto *cw = qobject_cast<ChordWidget*>(m_tabs->widget(i)))
            cw->retranslate(english);

    // Menü-Texte
    auto *fileMenu = menuBar()->actions()[0]->menu();
    if (fileMenu) {
        auto acts = fileMenu->actions();
        if (acts.size() >= 6) {
            acts[0]->setText(english ? "Save .tex\tCtrl+S"     : ".tex speichern\tStrg+S");
            acts[1]->setText(english ? "Compile PDF\tCtrl+P"   : "PDF kompilieren\tStrg+P");
            acts[2]->setText(english ? "Copy TikZ\tCtrl+K"     : "TikZ kopieren\tStrg+K");
            acts[4]->setText(english ? "Reset chord\tCtrl+Z"   : "Akkord zurücksetzen\tStrg+Z");
            acts[6]->setText(english ? "&Quit"                  : "&Beenden");
        }
    }
    auto *aboutMenu = menuBar()->actions()[1]->menu();
    if (aboutMenu) {
        auto acts = aboutMenu->actions();
        if (!acts.isEmpty()) acts[0]->setText(english ? "&Help" : "&Hilfe");
    }
}

// ---------------------------------------------------------------------------
void MainWindow::showHelp()
{
    auto *dlg = new QDialog(this);
    dlg->setWindowTitle(m_english ? "Help" : "Hilfe");
    dlg->setMinimumWidth(420);
    auto *layout = new QVBoxLayout(dlg);

    auto *lbl = new QLabel(m_english ? R"(
<h3>Chord Diagram Generator</h3>
<b>Fretboard</b><br>
Click any cell to cycle through states:<br>
&nbsp;&nbsp;○ &nbsp;<b>Root</b> – tonic / root note<br>
&nbsp;&nbsp;● &nbsp;<b>Tone</b> – any other fretted note<br>
&nbsp;&nbsp;× &nbsp;<b>Mute</b> – string is damped / not played<br>
&nbsp;&nbsp;(empty) – not set<br>
<br>
Clicking a cell clears all other cells on the same string.<br>
Column 0 is the open string / nut position.<br>
<br>
<b>Barré</b><br>
Set manually, or auto-detected when 3+ adjacent strings<br>
share the same fret.<br>
<br>
<b>Downtune (½♭)</b><br>
Lower individual strings by up to 3 semitones.<br>
String labels update automatically.<br>
<br>
<b>Position</b><br>
"Start fret" shows a Roman numeral above the first fret column,<br>
indicating the position on the neck.<br>
<br>
<b>Export</b><br>
<i>Save .tex</i> &nbsp;– standalone LaTeX file, compilable with pdflatex<br>
<i>Compile PDF</i> – calls pdflatex and opens the result<br>
<i>Copy TikZ</i> &nbsp;– copies the tikzpicture block to clipboard;<br>
&nbsp;&nbsp;paste directly into your LaTeX document<br>
<br>
<b>Keyboard shortcuts</b><br>
Ctrl+S &nbsp; Save .tex &nbsp;&nbsp; Ctrl+P &nbsp; Compile PDF<br>
Ctrl+K &nbsp; Copy TikZ &nbsp; Ctrl+Z &nbsp; Reset
)" : R"(
<h3>Grifftabellen-Generator</h3>
<b>Griffbrett</b><br>
Klick auf eine Zelle schaltet den Zustand durch:<br>
&nbsp;&nbsp;○ &nbsp;<b>Grundton</b> – Tonika / Grundton des Akkords<br>
&nbsp;&nbsp;● &nbsp;<b>Ton</b> – sonstiger gegriffener Ton<br>
&nbsp;&nbsp;× &nbsp;<b>Mute</b> – Saite wird abgedämpft / nicht gespielt<br>
&nbsp;&nbsp;(leer) – nicht gesetzt<br>
<br>
Ein Klick löscht alle anderen Bünde derselben Saite.<br>
Spalte 0 ist der Leerstring / Sattelposition.<br>
<br>
<b>Barré</b><br>
Manuell einstellbar; wird automatisch erkannt wenn<br>
3+ benachbarte Saiten denselben Bund teilen.<br>
<br>
<b>Downtune (½♭)</b><br>
Einzelne Saiten um bis zu 3 Halbtöne absenken.<br>
Saiten-Labels passen sich automatisch an.<br>
<br>
<b>Lage</b><br>
"Startbund" zeigt eine römische Ziffer oberhalb der<br>
ersten Bundspalte und gibt die Lage am Hals an.<br>
<br>
<b>Export</b><br>
<i>Speichern .tex</i> – Standalone-LaTeX, kompilierbar mit pdflatex<br>
<i>PDF kompilieren</i> – ruft pdflatex auf und öffnet das Ergebnis<br>
<i>TikZ kopieren</i> &nbsp;– kopiert den tikzpicture-Block;<br>
&nbsp;&nbsp;direkt ins LaTeX-Dokument einfügen<br>
<br>
<b>Tastaturkürzel</b><br>
Strg+S &nbsp;.tex speichern &nbsp;&nbsp; Strg+P &nbsp;PDF kompilieren<br>
Strg+K &nbsp;TikZ kopieren &nbsp; Strg+Z &nbsp;Zurücksetzen
)");
    lbl->setTextFormat(Qt::RichText);
    lbl->setWordWrap(true);
    layout->addWidget(lbl);

    auto *bbox = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(bbox, &QDialogButtonBox::accepted, dlg, &QDialog::accept);
    layout->addWidget(bbox);

    dlg->exec();
}
