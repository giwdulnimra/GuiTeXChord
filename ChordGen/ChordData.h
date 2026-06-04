#pragma once
#include <QString>
#include <QVector>

// Zustand einer Saite an einem bestimmten Bund
enum class NoteState { None, Tone, Root, Mute };

// Ein gegriffener Punkt auf dem Griffbrett
struct FretNote {
    int stringIdx; // 0 = tiefste Saite (links im Diagramm)
    int fret;      // 1-based; 0 = Leerstring-Markierung (open/mute)
    NoteState state;
};

// Barré: von Saite firstString bis lastString auf Bund fret
struct BarreInfo {
    bool active = false;
    int fret = 1;
    int firstString = 0; // linke Saite (tiefste)
    int lastString = 5;  // rechte Saite (höchste)
};

struct ChordData {
    // --- Identität ---
    QString rootNote;     // z.B. "C", "F#", "Bb"
    QString suffix;       // z.B. "m", "maj7", "sus2", ""
    bool showName = true;

    // --- Griffbrett ---
    int startFret = 1;    // Anzeige-Startbund (römisch links)
    bool showStartFret = true; // nur anzeigen wenn != I

    // --- Saiten (Anzahl variabel für spätere Erweiterung) ---
    int numStrings = 6;
    QVector<QString> tuning; // z.B. {"E","A","d","g","h","e'"} tief→hoch

    static constexpr int NUM_FRETS = 4; // angezeigte Bünde

    // --- Noten ---
    // notes[stringIdx][fretRow]  fretRow 0 = Leerstring-Zeile
    // state dort: Open, Mute, oder None
    QVector<QVector<NoteState>> notes; // [string][0..NUM_FRETS]

    // --- Barré ---
    BarreInfo barre;

    // Initialisierung
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
