#pragma once
#include <QWidget>
#include <QVector>
#include "ChordData.h"

class QLineEdit; class QComboBox; class QCheckBox;
class QSpinBox;  class QLabel;    class QPushButton;
class QButtonGroup; class QRadioButton; class QGroupBox;
class ChordPreview;

// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
class ChordWidget : public QWidget {
    Q_OBJECT
public:
    explicit ChordWidget(int numStrings, const QVector<QString> &tuning,
                         QWidget *parent=nullptr);

    ChordData currentChord() const;
    void      loadChord(const ChordData &chord);
    void      clearAll();
    void      retranslate(bool english);
    bool      hasLatex() const { return m_latexOk; }

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
    void onGlobalDowntuneChanged(int val);
    void updatePreview();

private:
    void rebuildGrid();
    void updateBarreRadios();
    static QString applyDowntune(const QString &note, int semitones);
    static bool checkLatex();

    int              m_numStrings;
    QVector<QString> m_tuning;
    QVector<QString> m_currentTuning;
    int              m_globalDowntune = 0;
    Orientation      m_orientation    = Orientation::Vertical;
    bool             m_latexOk        = false;

    // Grid cells [stringIdx][fretRow]
    QVector<QVector<FretCell*>> m_cells;

    // Barré radio buttons [fretRow 1..NUM_FRETS]
    QCheckBox          *m_barreCheck   = nullptr;
    QButtonGroup       *m_barreGroup   = nullptr;
    QVector<QRadioButton*> m_barreRadios;
    QWidget            *m_barreRow     = nullptr; // container for radios

    // Controls – left column
    QComboBox  *m_orientCombo  = nullptr;
    QGroupBox  *m_gridGroup    = nullptr;
    QWidget    *m_gridContainer= nullptr;

    // Controls – right column
    ChordPreview *m_preview    = nullptr;

    // Chord name
    QComboBox  *m_rootCombo    = nullptr;
    QComboBox  *m_accCombo     = nullptr;
    QComboBox  *m_suffixCombo  = nullptr;
    QCheckBox  *m_showNameCheck= nullptr;

    // Position & tuning
    QSpinBox   *m_startFretSpin= nullptr;
    QCheckBox  *m_showPosCheck = nullptr;
    QComboBox  *m_downtuneCombo= nullptr;  // Std, -1..-8

    // Export
    QPushButton *m_pdfBtn  = nullptr;
    QPushButton *m_texBtn  = nullptr;
    QPushButton *m_copyBtn = nullptr;
    QLabel      *m_outDirLabel = nullptr;
    QLabel      *m_statusLabel = nullptr;

    // Reset
    QPushButton *m_resetBtn= nullptr;

    QString m_outputDir;
    bool    m_english = true;
};
