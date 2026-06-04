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
class QToolButton;
class ChordPreview;

// Ein einzelnes Feld im Griffbrett-Grid
// Zustand wird per Klick durchgeschalten
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
// Haupt-Widget: Akkord-Eingabe + Vorschau + Export-Buttons
class ChordWidget : public QWidget {
    Q_OBJECT
public:
    explicit ChordWidget(int numStrings, const QVector<QString> &tuning,
                         QWidget *parent = nullptr);

    ChordData currentChord() const;
    void      loadChord(const ChordData &chord);
    void      clearAll();

signals:
    void chordChanged();

public slots:
    void onExportTex();
    void onCompilePdf();
    void onCopyTikz();

private slots:
    void onCellChanged(int string, int fret, NoteState state);
    void onBarreToggled(bool checked);
    void updatePreview();

private:
    void buildGrid();
    void syncBarreFromCells(); // Auto-Barré-Erkennung

    int                   m_numStrings;
    QVector<QString>      m_tuning;

    // Grid: m_cells[string][fretRow 0..4]
    QVector<QVector<FretCell*>> m_cells;

    // Controls
    QLineEdit   *m_rootEdit;
    QComboBox   *m_accidentalCombo; // "", "#", "b"
    QLineEdit   *m_suffixEdit;
    QCheckBox   *m_showNameCheck;
    QSpinBox    *m_startFretSpin;
    QCheckBox   *m_showStartFretCheck;
    QCheckBox   *m_barreCheck;
    QSpinBox    *m_barreFretSpin;
    QSpinBox    *m_barreFromSpin;
    QSpinBox    *m_barreToSpin;
    QLabel      *m_statusLabel;
    ChordPreview *m_preview;

    QString     m_outputDir;
};
