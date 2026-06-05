#include "LatexGenerator.h"
#include <QStringList>

// ---------------------------------------------------------------------------
// Koordinatensystem – GEDREHT gegenüber grifftabelle.tex:
//
//   x-Achse = Bünde (links→rechts):
//     Bund 0 (Leerstring)  x = FRET_X[0] = -0.5
//     Bund 1               x = FRET_X[1] =  0.75
//     Bund 2               x = FRET_X[2] =  2.25
//     Bund 3               x = FRET_X[3] =  3.75
//     Bund 4               x = FRET_X[4] =  5.25
//
//   y-Achse = Saiten (oben→unten, TikZ y wächst nach oben → tiefste Saite
//   bekommt den kleinsten y-Wert):
//     stringIdx 0  = e1  (höchste)  →  y = +YDIST*(N-1)/2
//     stringIdx N-1= E6  (tiefste)  →  y = -YDIST*(N-1)/2
//
//   Gitternetzlinien (vertikale Bundlinien) bei x = GRID_X[0..3]
//   zwischen den Bund-Mittelpunkten.
//
//   Rahmen: von x=FRAME_LEFT bis x=FRAME_RIGHT,
//           von y=stringY(N-1) bis y=stringY(0)
//
//   Saiten-Label: links bei x = LABEL_X
//   Akkordname:   oben zentriert
//   Startbund:    unter dem Rahmen, zentriert zwischen Bund 1 und Bund 4
// ---------------------------------------------------------------------------

static constexpr double YDIST = 0.7;   // Abstand zwischen Saiten

// x-Positionen der Bund-Mittelpunkte (Noten sitzen hier)
// Bund 0 etwas abgesetzt, dann gleichmäßige Abstände
static const double FRET_X[] = { -0.5, 0.75, 2.25, 3.75, 5.25 };

// Vertikale Gitternetzlinien (zwischen Bünden, = Bundstäbe)
// zwischen FRET_X[f] und FRET_X[f+1], also bei Mitte:
static const double GRID_X[] = { 0.0, 1.5, 3.0, 4.5 };

static constexpr double FRAME_LEFT  = -1.0;  // linke Rahmenkante (vor Bund 0)
static constexpr double FRAME_RIGHT =  6.0;  // rechte Rahmenkante (nach Bund 4)
static constexpr double LABEL_X     = -1.6;  // Saiten-Labels links außen
static constexpr double NAME_Y_OFFSET = 0.7; // Akkordname oberhalb Rahmen

// ---------------------------------------------------------------------------
double LatexGenerator::stringY(int stringIdx, int numStrings)
{
    // stringIdx 0 = e1 = oben = größtes y
    // stringIdx N-1 = E6 = unten = kleinstes y
    return ((numStrings - 1) / 2.0 - stringIdx) * YDIST;
}

double LatexGenerator::fretX(int fretRow)
{
    if (fretRow >= 0 && fretRow <= ChordData::NUM_FRETS) return FRET_X[fretRow];
    return FRET_X[ChordData::NUM_FRETS];
}

QString LatexGenerator::romanNumeral(int n)
{
    static const char *thousands[] = {"","M","MM","MMM"};
    static const char *hundreds[]  = {"","C","CC","CCC","CD","D","DC","DCC","DCCC","CM"};
    static const char *tens[]      = {"","X","XX","XXX","XL","L","LX","LXX","LXXX","XC"};
    static const char *ones[]      = {"","I","II","III","IV","V","VI","VII","VIII","IX"};
    if (n <= 0 || n > 3999) return QString::number(n);
    return QString(thousands[n/1000]) + hundreds[(n%1000)/100]
         + tens[(n%100)/10] + ones[n%10];
}

QString LatexGenerator::stateToStyle(NoteState s)
{
    switch (s) {
    case NoteState::Root: return "root";
    case NoteState::Tone: return "tone";
    case NoteState::Mute: return "mute";
    default:              return "";
    }
}

// ---------------------------------------------------------------------------
QString LatexGenerator::tikzFragment(const ChordData &chord)
{
    QStringList lines;
    const int N = chord.numStrings;

    double yTop    = stringY(0,   N);  // e1, oben
    double yBottom = stringY(N-1, N);  // E6, unten

    // --- Styles ---
    lines << R"(\tikzset{)";
    lines << R"(  root/.style={draw=black,shape=circle,fill=white,minimum size=8pt,inner sep=0pt},)";
    lines << R"(  tone/.style={draw=black,shape=circle,fill=black,minimum size=7pt,inner sep=0pt},)";
    lines << R"(  mute/.style={draw=none,shape=rectangle,rounded corners=.7pt,minimum size=8pt,)";
    lines << R"(    path picture={\draw[line width=1pt,color=black])";
    lines << R"(      (path picture bounding box.south west)--(path picture bounding box.north east))";
    lines << R"(      (path picture bounding box.north west)--(path picture bounding box.south east);}},)";
    lines << R"(})";
    lines << "";

    lines << R"(\begin{tikzpicture})";
    lines << R"(  \begin{pgfonlayer}{nodelayer})";

    // Akkord-Name zentriert über dem Diagramm
    if (chord.showName && !chord.fullName().isEmpty()) {
        double cx = (FRAME_LEFT + FRAME_RIGHT) / 2.0;
        double cy = yTop + NAME_Y_OFFSET;
        lines << QString("    \\node [font=\\bfseries] (chordname) at (%1, %2) {%3};")
                     .arg(cx, 0,'f',3).arg(cy, 0,'f',3).arg(chord.fullName());
    }

    // Saiten-Labels links außen (e1 oben, E6 unten)
    for (int s = 0; s < N; ++s) {
        double y = stringY(s, N);
        QString label = (s < chord.tuning.size()) ? chord.tuning[s] : QString::number(s+1);
        lines << QString("    \\node [anchor=east] (t%1) at (%2, %3) {%4};")
                     .arg(s).arg(LABEL_X, 0,'f',3).arg(y, 0,'f',3).arg(label);
    }

    // Startbund-Label unter dem Rahmen, zentriert über Bünde 1–4
    if (chord.showStartFret && chord.startFret >= 1) {
        double cx = (FRET_X[1] + FRET_X[ChordData::NUM_FRETS]) / 2.0;
        double cy = yBottom - 0.5;
        lines << QString("    \\node [] (startfret) at (%1, %2) {%3};")
                     .arg(cx, 0,'f',3).arg(cy, 0,'f',3)
                     .arg(romanNumeral(chord.startFret));
    }

    // Rahmen-Ecken
    lines << QString("    \\node [] (frameUL) at (%1, %2) {};").arg(FRAME_LEFT, 0,'f',3).arg(yTop,    0,'f',3);
    lines << QString("    \\node [] (frameUR) at (%1, %2) {};").arg(FRAME_RIGHT,0,'f',3).arg(yTop,    0,'f',3);
    lines << QString("    \\node [] (frameBL) at (%1, %2) {};").arg(FRAME_LEFT, 0,'f',3).arg(yBottom, 0,'f',3);
    lines << QString("    \\node [] (frameBR) at (%1, %2) {};").arg(FRAME_RIGHT,0,'f',3).arg(yBottom, 0,'f',3);

    // Gitterlinienpunkte (vertikale Bundstäbe, oben/unten)
    for (int g = 0; g < 4; ++g) {
        lines << QString("    \\node [] (gt%1) at (%2, %3) {};").arg(g).arg(GRID_X[g],0,'f',3).arg(yTop,    0,'f',3);
        lines << QString("    \\node [] (gb%1) at (%2, %3) {};").arg(g).arg(GRID_X[g],0,'f',3).arg(yBottom, 0,'f',3);
    }

    // Saiten-Ankerpunkte (links/rechts, für horizontale Saitenlinien)
    for (int s = 0; s < N; ++s) {
        double y = stringY(s, N);
        lines << QString("    \\node [] (sL%1) at (%2, %3) {};").arg(s).arg(FRAME_LEFT, 0,'f',3).arg(y, 0,'f',3);
        lines << QString("    \\node [] (sR%1) at (%2, %3) {};").arg(s).arg(FRAME_RIGHT,0,'f',3).arg(y, 0,'f',3);
    }

    // Noten-Nodes: x = fretX(f), y = stringY(s)
    for (int s = 0; s < N; ++s) {
        double y = stringY(s, N);
        for (int f = 0; f <= ChordData::NUM_FRETS; ++f) {
            NoteState st = chord.notes[s][f];
            double x = fretX(f);
            QString styleStr = stateToStyle(st);
            if (styleStr.isEmpty())
                lines << QString("    \\node [] (n%1_%2) at (%3, %4) {};")
                             .arg(s).arg(f).arg(x,0,'f',3).arg(y,0,'f',3);
            else
                lines << QString("    \\node [%1] (n%2_%3) at (%4, %5) {};")
                             .arg(styleStr).arg(s).arg(f).arg(x,0,'f',3).arg(y,0,'f',3);
        }
    }

    lines << R"(  \end{pgfonlayer})";
    lines << "";
    lines << R"(  \begin{pgfonlayer}{edgelayer})";

    // Äußerer Rahmen
    lines << R"(    \draw (frameUL.center) -- (frameUR.center) -- (frameBR.center) -- (frameBL.center) -- cycle;)";

    // Horizontale Saitenlinien (innen, ohne äußere zwei die zum Rahmen gehören)
    for (int s = 1; s < N - 1; ++s)
        lines << QString("    \\draw (sL%1.center) to (sR%1.center);").arg(s);

    // Vertikale Bundstäbe (Gitternetzlinien)
    for (int g = 0; g < 4; ++g)
        lines << QString("    \\draw (gt%1.center) to (gb%1.center);").arg(g);

    // Barré: horizontale dicke Linie entlang x=fretX(fr), von stringY(fs) bis stringY(ls)
    if (chord.barre.active) {
        int fr = chord.barre.fret;
        int fs = chord.barre.firstString;
        int ls = chord.barre.lastString;
        if (fr >= 1 && fr <= ChordData::NUM_FRETS && fs < ls) {
            lines << QString("    \\draw [line width=5pt, line cap=round, opacity=0.85]"
                             " (n%1_%2.center) to (n%3_%4.center);")
                         .arg(fs).arg(fr).arg(ls).arg(fr);
        }
    }

    lines << R"(  \end{pgfonlayer})";
    lines << R"(\end{tikzpicture})";

    return lines.join('\n');
}

// ---------------------------------------------------------------------------
QString LatexGenerator::standaloneDocument(const ChordData &chord)
{
    QStringList doc;
    doc << R"(\documentclass[tikz, border=6pt]{standalone})";
    doc << R"(\usepackage{tikz})";
    doc << R"(\usetikzlibrary{shapes.geometric})";
    doc << R"(\pgfdeclarelayer{nodelayer})";
    doc << R"(\pgfdeclarelayer{edgelayer})";
    doc << R"(\pgfdeclarelayer{backgroundlayer})";
    doc << R"(\pgfsetlayers{backgroundlayer,edgelayer,nodelayer,main})";
    doc << R"(\begin{document})";
    doc << tikzFragment(chord);
    doc << R"(\end{document})";
    return doc.join('\n');
}
