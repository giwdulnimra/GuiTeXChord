#pragma once
#include <QMainWindow>

class QTabWidget;
class QListWidget;
class ChordWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void setupMenu();

    QTabWidget *m_tabs;
};
