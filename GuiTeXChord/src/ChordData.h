#pragma once
#include <QString>
#include <QVector>

enum class NoteState { None, Tone, Root, Mute };

struct FretNote {
    int stringIdx;
    int fret;
    NoteState state;
};

struct BarreInfo {
    bool active = false;
    int fret = 1;
    int firstString = 0;
    int lastString  = 5;
};

struct ChordData {
    QString rootNote;
    QString suffix;
    bool    showName     = true;
    int     startFret    = 1;
    bool    showStartFret = false;
    int     numStrings   = 6;
    QVector<QString> tuning; // tief→hoch, Index 0 = tiefste Saite

    static constexpr int NUM_FRETS = 4;

    QVector<QVector<NoteState>> notes; // [stringIdx][fretRow 0..NUM_FRETS]
    BarreInfo barre;

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
