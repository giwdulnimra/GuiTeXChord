#pragma once
#include <QWidget>
#include <QVector>
#include "ChordData.h"

class QComboBox; class QCheckBox; class QSpinBox;
class QLabel;    class QPushButton; class QGroupBox;
class QButtonGroup; class QRadioButton;
class ChordPreview;

class FretCell : public QWidget {
    Q_OBJECT
public:
    explicit FretCell(int stringIdx, int fretRow, QWidget *parent=nullptr);
    NoteState state() const { return m_state; }
    void setState(NoteState s);
    void reset(){ setState(NoteState::None); }
signals:
    void stateChanged(int stringIdx, int fretRow, NoteState newState);
protected:
    void mousePressEvent(QMouseEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    QSize sizeHint() const override { return {26,26}; }
private:
    int m_string, m_fret;
    NoteState m_state = NoteState::None;
};

class ChordWidget : public QWidget {
    Q_OBJECT
public:
    explicit ChordWidget(int numStrings, const QVector<QString> &tuning,
                         QWidget *parent=nullptr);
    ChordData currentChord() const;
    void      loadChord(const ChordData &chord);
    void      clearAll();
    void      retranslate(bool english);

signals:
    void chordChanged();

public slots:
    void onExportTex();
    void onCompilePdf();
    void onCopyTikz();

private slots:
    void onCellChanged(int str, int fret, NoteState state);
    void onBarreToggled(bool checked);
    void onOrientationChanged(int idx);
    void onGlobalDowntuneChanged(int semitones);
    void updatePreview();

private:
    void rebuildGrid();
    void updateBarreStringCombos();
    static QString applyDowntune(const QString &note, int semitones);
    static bool    checkLatex();

    int              m_numStrings;
    QVector<QString> m_tuning;
    QVector<QString> m_currentTuning;
    int              m_globalDowntune = 0;
    Orientation      m_orientation    = Orientation::Vertical;
    bool             m_latexOk        = false;
    bool             m_english        = true;

    QVector<QVector<FretCell*>> m_cells;
    QVector<QRadioButton*>      m_barreRadios;

    QCheckBox    *m_barreCheck    = nullptr;
    QButtonGroup *m_barreGroup    = nullptr;
    QComboBox    *m_barreFromCombo= nullptr;
    QComboBox    *m_barreToCombo  = nullptr;
    QGroupBox    *m_gridGroup     = nullptr;
    QWidget      *m_gridContainer = nullptr;
    QComboBox    *m_orientCombo   = nullptr;

    ChordPreview *m_preview       = nullptr;
    QComboBox    *m_rootCombo     = nullptr;
    QComboBox    *m_accCombo      = nullptr;
    QComboBox    *m_suffixCombo   = nullptr;
    QCheckBox    *m_showNameCheck = nullptr;
    QSpinBox     *m_startFretSpin = nullptr;
    QCheckBox    *m_showPosCheck  = nullptr;
    QComboBox    *m_downtuneCombo = nullptr;
    QPushButton  *m_pdfBtn        = nullptr;
    QPushButton  *m_texBtn        = nullptr;
    QPushButton  *m_copyBtn       = nullptr;
    QPushButton  *m_resetBtn      = nullptr;
    QLabel       *m_outDirLabel   = nullptr;
    QLabel       *m_statusLabel   = nullptr;
    QLabel       *m_helpLabel     = nullptr;
    QString       m_outputDir;
};
