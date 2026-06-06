#pragma once
#include <QMainWindow>

class QTabWidget;
class QAction;
class ChordWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void setupMenu();
    void setLanguage(bool english);
    void showHelp();

    QTabWidget *m_tabs;
    QAction    *m_actEnglish;
    QAction    *m_actDeutsch;
    bool        m_english = true;
};
