#pragma once
#include "ChordData.h"
#include <QString>

class LatexGenerator {
public:
    // Erzeugt nur das tikzpicture-Fragment (für Einbettung ins Liedblatt)
    static QString tikzFragment(const ChordData &chord);

    // Vollständiges standalone-Dokument (kompilierbar mit pdflatex)
    static QString standaloneDocument(const ChordData &chord);

private:
    // Koordinaten-Hilfsfunktionen (spiegeln grifftabelle.tex)
    static double stringX(int stringIdx, int numStrings);
    static double fretY(int fretRow);   // 0 = Leerstring-Zeile

    static QString romanNumeral(int n);
    static QString stateToStyle(NoteState s);
};
