#pragma once
#include <QString>
#include <QVector>

// ---------------------------------------------------------------------------
enum class NoteState  { None, Tone, Root, Mute };
enum class Orientation { Horizontal, Vertical };
// Horizontal = Saiten horizontal (Zeilen), Bünde vertikal (Spalten)
//              → waagerechte Grifftabelle, wie im horizontal-PDF
// Vertical   = Saiten vertikal (Spalten), Bünde horizontal (Zeilen)
//              → senkrechte Grifftabelle, wie im vertical-PDF / grifftabelle.tex

// ---------------------------------------------------------------------------
struct BarreInfo {
    bool active      = false;
    int  fret        = 1;   // 1-based fret (not the open string)
    int  firstString = 0;   // lower string index
    int  lastString  = 5;   // higher string index
};

// ---------------------------------------------------------------------------
struct ChordData {
    // Identity
    QString rootNote;
    QString suffix;
    bool    showName      = true;

    // Position
    int     startFret     = 1;
    bool    showStartFret = false;

    // Strings
    int              numStrings = 6;
    QVector<QString> tuning;    // index 0 = lowest string (E)

    static constexpr int NUM_FRETS = 5; // fretted positions (open + 5 frets)

    // Notes: [stringIdx 0=low..N-1=high][fretRow 0=open, 1..NUM_FRETS]
    QVector<QVector<NoteState>> notes;

    // Barre
    BarreInfo barre;

    // Output orientation
    Orientation orientation = Orientation::Vertical;

    void init() {
        notes.resize(numStrings);
        for (auto &col : notes)
            col.assign(NUM_FRETS + 1, NoteState::None);
    }

    QString fullName() const {
        if (!showName) return {};
        return rootNote + suffix;
    }
};
