#include "LatexGenerator.h"
#include <QStringList>

// ---------------------------------------------------------------------------
// Koordinatensystem (gedreht, v0.01.20):
//
//   x-Achse = Bünde (links→rechts):
//     Sattel  (zwei dichte Linien): SATTEL_X1=-0.15, SATTEL_X2=0.0
//     Bund 0 (Leerstring-Noten):   FRET_X[0] = -0.55  (links des Sattels)
//     Bund 1–4:                    FRET_X[1..4] gleichmäßig rechts des Sattels
//     Gitternetzlinien (Bundstäbe): GRID_X[0..3] zwischen den Bünden
//
//   y-Achse = Saiten:
//     stringIdx 0  = tiefste Saite (E6)  → y = -(N-1)/2 * YDIST   (unten)
//     stringIdx N-1= höchste Saite (e1)  → y = +(N-1)/2 * YDIST   (oben)
//     Rendering: tuning[0]=E unten, tuning[N-1]=e' oben
//
//   Startbund-Label: oberhalb des Rahmens, linksbündig ab FRET_X[1]
//   Akkordname:      zentriert oberhalb, mit fester Reserve auch wenn leer
//   Bounding-Box:    IMMER gleich groß (feste Reserven für Name + Startbund)
// ---------------------------------------------------------------------------

static constexpr double YDIST      = 0.7;
static constexpr double FRET_STEP  = 1.5;   // Abstand zwischen Bünden 1-4
static constexpr double SATTEL_X1  = -0.15; // linke Sattellinie
static constexpr double SATTEL_X2  =  0.10; // rechte Sattellinie (Sattel = zwei Linien)
static constexpr double FRAME_LEFT = -0.85; // linke Rahmenkante (vor Sattel)
static constexpr double FRAME_RIGHT=  6.10; // rechte Rahmenkante
static constexpr double LABEL_X    = -1.30; // Saiten-Labels links außen

// Noten-x-Positionen:
// Bund 0 links des Sattels, Bünde 1-4 rechts gleichmäßig
static const double FRET_X[] = {
    -0.55,          // Bund 0 (Leerstring)
     0.75,          // Bund 1
     0.75 + 1*FRET_STEP,  // Bund 2
     0.75 + 2*FRET_STEP,  // Bund 3
     0.75 + 3*FRET_STEP   // Bund 4
};

// Bundstab-Positionen (vertikale Gitternetzlinien zwischen Bünden 1-4)
static const double GRID_X[] = {
    (FRET_X[1]+FRET_X[2])/2.0,
    (FRET_X[2]+FRET_X[3])/2.0,
    (FRET_X[3]+FRET_X[4])/2.0
};

// Feste vertikale Reserven für konstante Bounding-Box
static constexpr double NAME_RESERVE     = 0.7;  // immer reserviert oben
static constexpr double STARTFRET_RESERVE= 0.5;  // immer reserviert oben (unter Name)

// ---------------------------------------------------------------------------
double LatexGenerator::stringY(int stringIdx, int numStrings)
{
    // stringIdx 0 = tiefste Saite (E) = unten = kleinstes y
    return (stringIdx - (numStrings - 1) / 2.0) * YDIST;
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

    double yBottom = stringY(0,   N);  // E6, unten
    double yTop    = stringY(N-1, N);  // e1, oben

    // Konstante Oberkante: immer Name + Startbund reserviert
    double yNameCenter    = yTop + NAME_RESERVE;
    double yStartFretTop  = yTop + STARTFRET_RESERVE * 0.55;

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

    // Akkordname – immer ein Node damit Bounding-Box konstant bleibt
    {
        double cx = (FRAME_LEFT + FRAME_RIGHT) / 2.0;
        QString nameStr = (chord.showName && !chord.fullName().isEmpty())
                          ? chord.fullName() : "";
        lines << QString("    \\node [font=\\bfseries] (chordname) at (%1, %2) {%3};")
                     .arg(cx, 0,'f',3).arg(yNameCenter, 0,'f',3).arg(nameStr);
    }

    // Startbund – immer ein Node (leer wenn nicht angezeigt)
    {
        QString sfStr = (chord.showStartFret && chord.startFret > 1)
                        ? romanNumeral(chord.startFret) : "";
        lines << QString("    \\node [anchor=south west, font=\\small] (startfret) at (%1, %2) {%3};")
                     .arg(FRET_X[1], 0,'f',3).arg(yStartFretTop, 0,'f',3).arg(sfStr);
    }

    // Saiten-Labels (E unten, e' oben)
    for (int s = 0; s < N; ++s) {
        double y = stringY(s, N);
        // tuning[0]=tiefste(E), tuning[N-1]=höchste(e') → direkt 1:1
        QString label = (s < chord.tuning.size()) ? chord.tuning[s] : QString::number(s+1);
        lines << QString("    \\node [anchor=east] (t%1) at (%2, %3) {%4};")
                     .arg(s).arg(LABEL_X, 0,'f',3).arg(y, 0,'f',3).arg(label);
    }

    // Rahmen-Ecken
    lines << QString("    \\node [] (frameUL) at (%1, %2) {};").arg(FRAME_LEFT,  0,'f',3).arg(yTop,    0,'f',3);
    lines << QString("    \\node [] (frameUR) at (%1, %2) {};").arg(FRAME_RIGHT, 0,'f',3).arg(yTop,    0,'f',3);
    lines << QString("    \\node [] (frameBL) at (%1, %2) {};").arg(FRAME_LEFT,  0,'f',3).arg(yBottom, 0,'f',3);
    lines << QString("    \\node [] (frameBR) at (%1, %2) {};").arg(FRAME_RIGHT, 0,'f',3).arg(yBottom, 0,'f',3);

    // Bundstab-Ankerpunkte
    for (int g = 0; g < 3; ++g) {
        lines << QString("    \\node [] (gt%1) at (%2, %3) {};").arg(g).arg(GRID_X[g],0,'f',3).arg(yTop,    0,'f',3);
        lines << QString("    \\node [] (gb%1) at (%2, %3) {};").arg(g).arg(GRID_X[g],0,'f',3).arg(yBottom, 0,'f',3);
    }

    // Sattel-Anker (zwei Linien)
    lines << QString("    \\node [] (sattUL) at (%1, %2) {};").arg(SATTEL_X1,0,'f',3).arg(yTop,    0,'f',3);
    lines << QString("    \\node [] (sattUR) at (%1, %2) {};").arg(SATTEL_X2,0,'f',3).arg(yTop,    0,'f',3);
    lines << QString("    \\node [] (sattBL) at (%1, %2) {};").arg(SATTEL_X1,0,'f',3).arg(yBottom, 0,'f',3);
    lines << QString("    \\node [] (sattBR) at (%1, %2) {};").arg(SATTEL_X2,0,'f',3).arg(yBottom, 0,'f',3);

    // Saiten-Anker (links/rechts)
    for (int s = 0; s < N; ++s) {
        double y = stringY(s, N);
        lines << QString("    \\node [] (sL%1) at (%2, %3) {};").arg(s).arg(FRAME_LEFT,  0,'f',3).arg(y,0,'f',3);
        lines << QString("    \\node [] (sR%1) at (%2, %3) {};").arg(s).arg(FRAME_RIGHT, 0,'f',3).arg(y,0,'f',3);
    }

    // Noten
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

    // Rahmen
    lines << R"(    \draw (frameUL.center) -- (frameUR.center) -- (frameBR.center) -- (frameBL.center) -- cycle;)";

    // Saitenlinien innen
    for (int s = 1; s < N - 1; ++s)
        lines << QString("    \\draw (sL%1.center) to (sR%1.center);").arg(s);

    // Bundstäbe (nur zwischen Bünden 1-4, 3 Linien)
    for (int g = 0; g < 3; ++g)
        lines << QString("    \\draw (gt%1.center) to (gb%1.center);").arg(g);

    // Sattel: zwei dichte vertikale Linien
    lines << R"(    \draw [line width=1.2pt] (sattUL.center) to (sattBL.center);)";
    lines << R"(    \draw [line width=2.2pt] (sattUR.center) to (sattBR.center);)";

    // Barré
    if (chord.barre.active) {
        int fr = chord.barre.fret;
        int fs = chord.barre.firstString;
        int ls = chord.barre.lastString;
        if (fr >= 1 && fr <= ChordData::NUM_FRETS && fs != ls) {
            // fs/ls sind stringIdx: kleinerer = weiter unten (tiefere Saite)
            int sLow  = qMin(fs, ls);
            int sHigh = qMax(fs, ls);
            lines << QString("    \\draw [line width=5pt, line cap=round, opacity=0.85]"
                             " (n%1_%2.center) to (n%3_%4.center);")
                         .arg(sLow).arg(fr).arg(sHigh).arg(fr);
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
