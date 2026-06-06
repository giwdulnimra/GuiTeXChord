#pragma once
#include <QWidget>
#include <QVector>
#include "ChordData.h"

class QLineEdit;
class QComboBox;
class QCheckBox;
class QSpinBox;
class QLabel;
class QPushButton;
class ChordPreview;

// ---------------------------------------------------------------------------
class FretCell : public QWidget {
    Q_OBJECT
public:
    explicit FretCell(int stringIdx, int fretRow, QWidget *parent = nullptr);
    NoteState state() const { return m_state; }
    void setState(NoteState s);
    void reset() { setState(NoteState::None); }

signals:
    void stateChanged(int stringIdx, int fretRow, NoteState newState);

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    QSize sizeHint() const override { return {28, 28}; }

private:
    int m_string, m_fret;
    NoteState m_state = NoteState::None;
};

// ---------------------------------------------------------------------------
class ChordWidget : public QWidget {
    Q_OBJECT
public:
    explicit ChordWidget(int numStrings, const QVector<QString> &tuning,
                         QWidget *parent = nullptr);

    ChordData currentChord() const;
    void      loadChord(const ChordData &chord);
    void      clearAll();

    // Sprache umschalten (wird von MainWindow aufgerufen)
    void retranslate(bool english);

signals:
    void chordChanged();

public slots:
    void onExportTex();
    void onCompilePdf();
    void onCopyTikz();

private slots:
    void onCellChanged(int string, int fret, NoteState state);
    void onBarreToggled(bool checked);
    void onDowntuneChanged(int stringIdx, int semitones);
    void updatePreview();

private:
    void syncBarreFromCells();
    static QString applyDowntune(const QString &note, int semitones);

    int              m_numStrings;
    QVector<QString> m_tuning;        // Basis-Stimmung (unverändert)
    QVector<QString> m_currentTuning; // nach Downtune
    QVector<int>     m_downtune;      // Halbtöne pro Saite (0..-3)

    QVector<QVector<FretCell*>> m_cells; // [stringIdx][fretRow]
    QVector<QSpinBox*>          m_downtuneSpins; // [stringIdx]
    QVector<QLabel*>            m_stringLabels;  // [stringIdx]

    QLineEdit  *m_rootEdit;
    QComboBox  *m_accidentalCombo;
    QLineEdit  *m_suffixEdit;
    QCheckBox  *m_showNameCheck;
    QSpinBox   *m_startFretSpin;
    QCheckBox  *m_showStartFretCheck;
    QCheckBox  *m_barreCheck;
    QSpinBox   *m_barreFretSpin;
    QSpinBox   *m_barreFromSpin;
    QSpinBox   *m_barreToSpin;
    QLabel     *m_statusLabel;
    QLabel     *m_outputDirLabel;
    ChordPreview *m_preview;

    QPushButton *m_texBtn;
    QPushButton *m_pdfBtn;
    QPushButton *m_copyBtn;
    QPushButton *m_resetBtn;

    QString m_outputDir;
    bool    m_english = true;
};
