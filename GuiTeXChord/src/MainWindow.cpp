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
#include <QHBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>

MainWindow::MainWindow(QWidget *parent):QMainWindow(parent)
{
    resize(980,660);
    m_tabs=new QTabWidget(this);

    struct T{ int n; QVector<QString> t; QString name; };
    const QVector<T> tunings={
        {6,{"E","A","d","g","h","e'"},"6-Str. Standard"},
        {6,{"D","A","d","g","h","e'"},"6-Str. Drop-D"},
        {7,{"B","E","A","d","g","h","e'"},"7-Str. Standard"},
        {7,{"A","E","A","d","g","h","e'"},"7-Str. Drop-A"},
        {8,{"F#","B","E","A","d","g","h","e'"},"8-Str. Standard"},
    };
    for(const auto &td:tunings)
        m_tabs->addTab(new ChordWidget(td.n,td.t),td.name);

    setCentralWidget(m_tabs);
    setupMenu();
    setLanguage(true);
    checkLatexOnStartup();
}

void MainWindow::checkLatexOnStartup()
{
    QProcess p; p.start("pdflatex","--version"); p.waitForFinished(3000);
    if(p.exitCode()!=0){
        QMessageBox msg(this);
        msg.setWindowTitle("LaTeX not found");
        msg.setIcon(QMessageBox::Warning);
        msg.setText(
            "<b>pdflatex was not found on this system.</b><br><br>"
            "PDF compilation will be disabled.<br>"
            "You can still generate and save .tex files and copy TikZ code.<br><br>"
            "To enable PDF compilation, install a LaTeX distribution:<br>"
            "• <b>Windows:</b> MiKTeX – <a href='https://miktex.org'>miktex.org</a><br>"
            "• <b>Linux:</b> <code>sudo apt install texlive-pictures</code><br>"
            "• <b>macOS:</b> MacTeX – <a href='https://tug.org/mactex'>tug.org/mactex</a>");
        msg.setTextFormat(Qt::RichText);
        msg.setStandardButtons(QMessageBox::Ok);
        auto *openBtn=msg.addButton("Open MiKTeX website",QMessageBox::ActionRole);
        msg.exec();
        if(msg.clickedButton()==openBtn)
            QDesktopServices::openUrl(QUrl("https://miktex.org/download"));
    }
}

void MainWindow::setupMenu()
{
    // ── File ──
    auto *fileMenu=menuBar()->addMenu("&File");
    auto *actTex  =fileMenu->addAction("Save .tex\tCtrl+S");
    auto *actPdf  =fileMenu->addAction("Compile PDF\tCtrl+P");
    auto *actCopy =fileMenu->addAction("Copy TikZ\tCtrl+C");
    fileMenu->addSeparator();
    auto *actReset=fileMenu->addAction("Reset chord\tCtrl+Z");
    fileMenu->addSeparator();
    fileMenu->addAction("&Quit",this,&QWidget::close,QKeySequence::Quit);

    auto cw=[this]()->ChordWidget*{
        return qobject_cast<ChordWidget*>(m_tabs->currentWidget());};
    connect(actTex,  &QAction::triggered,this,[=]{ if(auto*w=cw()) w->onExportTex();  });
    connect(actPdf,  &QAction::triggered,this,[=]{ if(auto*w=cw()) w->onCompilePdf(); });
    connect(actCopy, &QAction::triggered,this,[=]{ if(auto*w=cw()) w->onCopyTikz();   });
    connect(actReset,&QAction::triggered,this,[=]{ if(auto*w=cw()) w->clearAll();      });

    // ── About ──
    auto *aboutMenu=menuBar()->addMenu("&About");
    auto *actHelp   =aboutMenu->addAction("&Help");
    auto *actOptions=aboutMenu->addAction("Options");
    actOptions->setEnabled(false);
    aboutMenu->addSeparator();

    auto *lg=new QActionGroup(this); lg->setExclusive(true);
    m_actEn=aboutMenu->addAction("English"); m_actEn->setCheckable(true); m_actEn->setChecked(true); lg->addAction(m_actEn);
    m_actDe=aboutMenu->addAction("Deutsch"); m_actDe->setCheckable(true); lg->addAction(m_actDe);

    connect(actHelp, &QAction::triggered,this,&MainWindow::showHelpDialog);
    connect(m_actEn, &QAction::triggered,this,[this]{ setLanguage(true);  });
    connect(m_actDe, &QAction::triggered,this,[this]{ setLanguage(false); });
}

void MainWindow::setLanguage(bool english)
{
    m_english=english;
    setWindowTitle(english?"Chord Diagram Generator":"Grifftabellen-Generator");
    for(int i=0;i<m_tabs->count();++i)
        if(auto*cw=qobject_cast<ChordWidget*>(m_tabs->widget(i)))
            cw->retranslate(english);
}

void MainWindow::showHelpDialog()
{
    auto *dlg=new QDialog(this);
    dlg->setWindowTitle(m_english?"Help":"Hilfe");
    dlg->setMinimumSize(480,360);
    auto *layout=new QVBoxLayout(dlg);

    // Two-column layout matching the spec
    auto *cols=new QHBoxLayout();

    // Left: symbols
    auto *symGroup=new QWidget();
    auto *symLayout=new QVBoxLayout(symGroup);
    symLayout->addWidget(new QLabel("<b>Symbols:</b>"));
    auto addSym=[&](const QString &sym,const QString &desc){
        auto *row=new QHBoxLayout();
        auto *s=new QLabel(sym); s->setFixedWidth(22); s->setAlignment(Qt::AlignCenter);
        s->setStyleSheet("font-weight:bold;font-size:14px;");
        row->addWidget(s); row->addWidget(new QLabel(desc)); row->addStretch();
        symLayout->addLayout(row);
    };
    addSym("○", m_english?"Root note":"Grundton");
    addSym("●", m_english?"Other note":"Ton");
    addSym("×", m_english?"Muted string":"Gedämpfte Saite");
    symLayout->addStretch();
    cols->addWidget(symGroup,1);

    // Right: functions
    auto *fnGroup=new QWidget();
    auto *fnLayout=new QVBoxLayout(fnGroup);
    fnLayout->addWidget(new QLabel("<b>Functions:</b>"));
    auto addFn=[&](const QString &name,const QString &desc){
        fnLayout->addWidget(new QLabel("<b>"+name+"</b>"));
        auto *d=new QLabel("  "+desc); d->setWordWrap(true);
        fnLayout->addWidget(d);
        fnLayout->addSpacing(4);
    };
    if(m_english){
        addFn("Compile PDF","Generates a PDF file using pdflatex.");
        addFn("Save .tex",  "Saves the LaTeX source file.");
        addFn("Copy TikZ",  "Copies the tikzpicture block to clipboard.\nPaste into your LaTeX document.");
    } else {
        addFn("PDF kompilieren","Erzeugt eine PDF-Datei mit pdflatex.");
        addFn(".tex speichern", "Speichert den LaTeX-Quellcode in eine Datei.");
        addFn("TikZ kopieren",  "Kopiert den tikzpicture-Block in die Zwischenablage.\nIn LaTeX-Dokument einfügen.");
    }
    fnLayout->addStretch();
    cols->addWidget(fnGroup,2);

    layout->addLayout(cols);
    auto *bbox=new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(bbox,&QDialogButtonBox::accepted,dlg,&QDialog::accept);
    layout->addWidget(bbox);
    dlg->exec();
}
