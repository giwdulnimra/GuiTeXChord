#pragma once
#include "ChordData.h"
#include <QString>

class LatexGenerator {
public:
    // tikzpicture fragment only (for embedding in existing LaTeX docs)
    static QString tikzFragment(const ChordData &chord);
    // Full compilable standalone document
    static QString standaloneDocument(const ChordData &chord);

private:
    static QString tikzVertical(const ChordData &chord);
    static QString tikzHorizontal(const ChordData &chord);
    static QString commonStyles();
    static QString romanNumeral(int n);
    static QString stateToStyle(NoteState s);
};
