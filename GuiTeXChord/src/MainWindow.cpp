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
#include <QPushButton>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>
#include <QFile>
#include <QRegularExpression>

static QString loadResource(const QString &path){
    QFile f(path); if(!f.open(QIODevice::ReadOnly|QIODevice::Text)) return {};
    return QString::fromUtf8(f.readAll());
}
static QString mdToHtml(const QString &md){
    // Minimal: headers, bold, table rows, linebreaks
    QString html = md.toHtmlEscaped();
    html.replace(QRegularExpression(R"(^### (.+)$)",QRegularExpression::MultilineOption),
                 "<h4>\\1</h4>");
    html.replace(QRegularExpression(R"(^## (.+)$)", QRegularExpression::MultilineOption),
                 "<h3>\\1</h3>");
    html.replace(QRegularExpression(R"(^# (.+)$)",  QRegularExpression::MultilineOption),
                 "<h2>\\1</h2>");
    html.replace(QRegularExpression(R"(\*\*(.+?)\*\*)"), "<b>\\1</b>");
    html.replace(QRegularExpression(R"(\*(.+?)\*)"),     "<i>\\1</i>");
    html.replace(QRegularExpression(R"(`(.+?)`)"),       "<code>\\1</code>");
    html.replace("\n","<br>");
    return html;
}

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
    QProcess p; p.start("pdflatex",{"--version"}); p.waitForFinished(3000);
    if(p.exitCode()!=0){
        QMessageBox msg(this);
        msg.setWindowTitle("LaTeX not found");
        msg.setIcon(QMessageBox::Warning);
        msg.setText(
            "<b>pdflatex was not found on this system.</b><br><br>"
            "PDF compilation will be disabled.<br>"
            "You can still save .tex files and copy TikZ code.<br><br>"
            "Install a LaTeX distribution to enable PDF compilation:<br>"
            "• <b>Windows:</b> MiKTeX – <a href='https://miktex.org'>miktex.org</a><br>"
            "• <b>Linux:</b> <code>sudo apt install texlive-pictures</code>");
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
    auto *fileMenu=menuBar()->addMenu("&File");
    auto *actTex  =fileMenu->addAction("Save .tex\tCtrl+S");
    auto *actPdf  =fileMenu->addAction("Compile PDF\tCtrl+P");
    auto *actCopy =fileMenu->addAction("Copy TikZ\tCtrl+C");
    fileMenu->addSeparator();
    auto *actReset=fileMenu->addAction("Reset chord\tCtrl+Z");
    fileMenu->addSeparator();
    fileMenu->addAction("&Quit", QKeySequence::Quit, this, &QWidget::close);

    auto cw=[this]()->ChordWidget*{
        return qobject_cast<ChordWidget*>(m_tabs->currentWidget());};
    connect(actTex,  &QAction::triggered,this,[=]{ if(auto*w=cw()) w->onExportTex();  });
    connect(actPdf,  &QAction::triggered,this,[=]{ if(auto*w=cw()) w->onCompilePdf(); });
    connect(actCopy, &QAction::triggered,this,[=]{ if(auto*w=cw()) w->onCopyTikz();   });
    connect(actReset,&QAction::triggered,this,[=]{ if(auto*w=cw()) w->clearAll();      });

    auto *aboutMenu=menuBar()->addMenu("&About");
    auto *actHelp   =aboutMenu->addAction("&Help");
    auto *actOptions=aboutMenu->addAction("Options");
    actOptions->setEnabled(false);
    aboutMenu->addSeparator();
    auto *lg=new QActionGroup(this); lg->setExclusive(true);
    m_actEn=aboutMenu->addAction("English"); m_actEn->setCheckable(true);
    m_actEn->setChecked(true); lg->addAction(m_actEn);
    m_actDe=aboutMenu->addAction("Deutsch"); m_actDe->setCheckable(true);
    lg->addAction(m_actDe);

    connect(actHelp, &QAction::triggered,this,&MainWindow::showHelpDialog);
    connect(m_actEn, &QAction::triggered,this,[this]{ setLanguage(true);  });
    connect(m_actDe, &QAction::triggered,this,[this]{ setLanguage(false); });
}

void MainWindow::setLanguage(bool english)
{
    m_english=english;
    // Title: GuiTeXChord - vX.Y.Z
    QString ver = QString(CPP_APPVERSION_DISPLAY);
    setWindowTitle(QString("GuiTeXChord - %1").arg(ver));
    for(int i=0;i<m_tabs->count();++i)
        if(auto*cw=qobject_cast<ChordWidget*>(m_tabs->widget(i)))
            cw->retranslate(english);
}

void MainWindow::showHelpDialog()
{
    auto *dlg=new QDialog(this);
    dlg->setWindowTitle(m_english?"Help – GuiTeXChord":"Hilfe – GuiTeXChord");
    dlg->setMinimumSize(500,420);
    auto *layout=new QVBoxLayout(dlg);

    auto *lbl=new QLabel();
    lbl->setTextFormat(Qt::RichText);
    lbl->setWordWrap(true);
    lbl->setAlignment(Qt::AlignTop|Qt::AlignLeft);
    lbl->setOpenExternalLinks(true);

    QString mdPath = m_english ? ":/help/help_dialog_en.md" : ":/help/help_dialog_de.md";
    lbl->setText(mdToHtml(loadResource(mdPath)));
    layout->addWidget(lbl,1);

    auto *bbox=new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(bbox,&QDialogButtonBox::accepted,dlg,&QDialog::accept);
    layout->addWidget(bbox);
    dlg->exec();
}
