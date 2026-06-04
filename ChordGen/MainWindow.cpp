#include "MainWindow.h"
#include "ChordWidget.h"

#include <QTabWidget>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QVBoxLayout>
#include <QWidget>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("Grifftabellen-Generator");
    resize(860, 620);

    m_tabs = new QTabWidget(this);
    m_tabs->setTabPosition(QTabWidget::North);

    // Stimmungen definieren: {Anzahl Saiten, Labels tief→hoch, Tab-Name}
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
}

void MainWindow::setupMenu()
{
    auto *fileMenu = menuBar()->addMenu("&Datei");

    auto *actTex = fileMenu->addAction("Aktiven Akkord als &TeX speichern …");
    connect(actTex, &QAction::triggered, this, [this]{
        if (auto *cw = qobject_cast<ChordWidget*>(m_tabs->currentWidget()))
            cw->onExportTex();
    });

    auto *actPdf = fileMenu->addAction("Aktiven Akkord als &PDF kompilieren …");
    connect(actPdf, &QAction::triggered, this, [this]{
        if (auto *cw = qobject_cast<ChordWidget*>(m_tabs->currentWidget()))
            cw->onCompilePdf();
    });

    auto *actCopy = fileMenu->addAction("TikZ in &Zwischenablage");
    connect(actCopy, &QAction::triggered, this, [this]{
        if (auto *cw = qobject_cast<ChordWidget*>(m_tabs->currentWidget()))
            cw->onCopyTikz();
    });

    fileMenu->addSeparator();
    auto *actClear = fileMenu->addAction("Aktuellen Akkord &zurücksetzen");
    connect(actClear, &QAction::triggered, this, [this]{
        if (auto *cw = qobject_cast<ChordWidget*>(m_tabs->currentWidget()))
            cw->clearAll();
    });

    fileMenu->addSeparator();
    fileMenu->addAction("&Beenden", this, &QWidget::close, QKeySequence::Quit);

    auto *helpMenu = menuBar()->addMenu("&Hilfe");
    helpMenu->addAction("Über", this, []{
        QMessageBox::about(nullptr, "Grifftabellen-Generator",
            "<b>Grifftabellen-Generator</b><br>"
            "Erzeugt TikZ/LaTeX-Grifftabellen.<br><br>"
            "Klicken auf eine Zelle:<br>"
            "• Bund 0: leer → offen (○) → abgedämpft (×)<br>"
            "• Bund 1–4: leer → Ton (●) → Grundton (◎)");
    });
}
