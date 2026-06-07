#pragma once
#include <QMainWindow>
class QTabWidget; class QAction; class ChordWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent=nullptr);
private:
    void setupMenu();
    void setLanguage(bool english);
    void showHelpDialog();
    void checkLatexOnStartup();
    QTabWidget *m_tabs;
    QAction    *m_actEn, *m_actDe;
    bool        m_english = true;
};
