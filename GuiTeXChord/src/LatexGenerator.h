#pragma once
#include "ChordData.h"
#include <QString>

class LatexGenerator {
public:
    static QString tikzFragment(const ChordData &chord);
    static QString standaloneDocument(const ChordData &chord);

private:
    static double stringY(int stringIdx, int numStrings);
    static double fretX(int fretRow);
    static QString romanNumeral(int n);
    static QString stateToStyle(NoteState s);
};
