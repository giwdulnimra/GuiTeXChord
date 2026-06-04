#include "LatexGenerator.h"
#include <QStringList>

// ---------------------------------------------------------------------------
// Koordinatensystem aus grifftabelle.tex:
//   \xdist = 0.4  →  x-Positionen: (-5,-3,-1,+1,+3,+5)*xdist
//   fretRow 0  = Leerstring-Zeile   y = 3.10
//   fretRow 1  = 1. Bund (oben)     y = 2.25
//   fretRow 2  = 2. Bund            y = 0.75
//   fretRow 3  = 3. Bund            y = -0.75
//   fretRow 4  = 4. Bund (unten)    y = -2.25
// ---------------------------------------------------------------------------
static constexpr double XDIST = 0.4;
static const double FRET_Y[] = { 3.10, 2.25, 0.75, -0.75, -2.25 };
// Gitternetz-Linien (horizontale Saiten-Trennlinien)
static const double GRID_Y[] = { 3.0, 1.5, 0.0, -1.5 };
// Unterkante des Rahmens
static constexpr double BOTTOM_Y = -3.0;
// Label-Zeile über dem Rahmen
static constexpr double LABEL_Y = 3.6;
// Startbund-Label x-Position
static constexpr double STARTFRET_X_OFFSET = -6; // * xdist

double LatexGenerator::stringX(int stringIdx, int numStrings)
{
    // Bei 6 Saiten: Indizes 0..5 → x-Faktoren -5,-3,-1,+1,+3,+5
    int half = numStrings - 1;
    return (2 * stringIdx - half) * XDIST;
}

double LatexGenerator::fretY(int fretRow)
{
    if (fretRow >= 0 && fretRow < 5) return FRET_Y[fretRow];
    return -2.25;
}

QString LatexGenerator::romanNumeral(int n)
{
    // Reicht für Gitarren-Lagen (1–24)
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

    // --- Styles (werden im Fragment mitgeliefert, damit es standalone-fähig
    //     eingebettet werden kann; im Liedblatt-Kontext ggf. auskommentieren) ---
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

    // ---- nodelayer ----
    lines << R"(  \begin{pgfonlayer}{nodelayer})";

    // Akkord-Name oben
    if (chord.showName && !chord.fullName().isEmpty()) {
        double cx = 0.0; // zentriert
        lines << QString("    \\node [font=\\bfseries] (chordname) at (%1, 4.3) {%2};")
                     .arg(cx).arg(chord.fullName());
    }

    // Saiten-Labels
    for (int s = 0; s < N; ++s) {
        double x = stringX(s, N);
        QString label = (s < chord.tuning.size()) ? chord.tuning[s] : QString::number(s+1);
        lines << QString("    \\node [] (t%1) at (%2, %3) {%4};")
                     .arg(s).arg(x, 0, 'f', 3).arg(LABEL_Y).arg(label);
    }

    // Startbund-Label (nur wenn != I oder explizit gewünscht)
    if (chord.showStartFret && chord.startFret > 1) {
        double sx = STARTFRET_X_OFFSET * XDIST;
        double sy = (FRET_Y[0] + FRET_Y[1]) / 2.0;
        lines << QString("    \\node [rotate=90] (startfret) at (%1, %2) {%3};")
                     .arg(sx, 0, 'f', 3).arg(sy, 0, 'f', 3)
                     .arg(romanNumeral(chord.startFret));
    }

    // Rahmen-Ecken (für \draw ... -- cycle)
    double xLeft  = stringX(0, N);
    double xRight = stringX(N-1, N);
    lines << QString("    \\node [] (frameUL) at (%1, 3.18) {};").arg(xLeft,  0,'f',3);
    lines << QString("    \\node [] (frameUR) at (%1, 3.18) {};").arg(xRight, 0,'f',3);
    lines << QString("    \\node [] (frameBL) at (%1, %2) {};")  .arg(xLeft,  0,'f',3).arg(BOTTOM_Y);
    lines << QString("    \\node [] (frameBR) at (%1, %2) {};")  .arg(xRight, 0,'f',3).arg(BOTTOM_Y);

    // Gitterlinienpunkte (links/rechts pro horizontaler Linie)
    for (int g = 0; g < 4; ++g) {
        lines << QString("    \\node [] (gl%1) at (%2, %3) {};")
                     .arg(g).arg(xLeft, 0,'f',3).arg(GRID_Y[g]);
        lines << QString("    \\node [] (gr%1) at (%2, %3) {};")
                     .arg(g).arg(xRight, 0,'f',3).arg(GRID_Y[g]);
    }

    // Saiten-Ankerpunkte (oben/unten, für Barré benötigt)
    for (int s = 0; s < N; ++s) {
        double x = stringX(s, N);
        lines << QString("    \\node [] (sTop%1) at (%2, 3.18) {};").arg(s).arg(x,0,'f',3);
        lines << QString("    \\node [] (sBot%1) at (%2, %3) {};").arg(s).arg(x,0,'f',3).arg(BOTTOM_Y);
    }

    // Noten-Nodes
    for (int s = 0; s < N; ++s) {
        double x = stringX(s, N);
        for (int f = 0; f <= ChordData::NUM_FRETS; ++f) {
            NoteState st = chord.notes[s][f];
            double y = fretY(f);
            QString styleStr = stateToStyle(st);
            if (styleStr.isEmpty())
                lines << QString("    \\node [] (n%1_%2) at (%3, %4) {};")
                             .arg(s).arg(f).arg(x,0,'f',3).arg(y);
            else
                lines << QString("    \\node [%1] (n%2_%3) at (%4, %5) {};")
                             .arg(styleStr).arg(s).arg(f).arg(x,0,'f',3).arg(y);
        }
    }

    lines << R"(  \end{pgfonlayer})";
    lines << "";

    // ---- edgelayer ----
    lines << R"(  \begin{pgfonlayer}{edgelayer})";

    // Äußerer Rahmen
    lines << R"(    \draw (frameUL.center) -- (frameUR.center) -- (frameBR.center) -- (frameBL.center) -- cycle;)";

    // Innere Saitenlinien (ohne äußere zwei, die sind Teil des Rahmens)
    for (int s = 1; s < N-1; ++s) {
        lines << QString("    \\draw (sTop%1.center) to (sBot%1.center);").arg(s);
    }

    // Horizontale Bundlinien
    for (int g = 0; g < 4; ++g) {
        lines << QString("    \\draw (gl%1.center) to (gr%1.center);").arg(g);
    }

    // Barré (gebogene Linie, analog zum bend-right-Beispiel im Template)
    if (chord.barre.active) {
        int fr = chord.barre.fret; // 1-based fretRow
        int fs = chord.barre.firstString;
        int ls = chord.barre.lastString;
        if (fr >= 1 && fr <= ChordData::NUM_FRETS && fs < ls) {
            // Dicke geschwungene Linie von einer Saite zur anderen
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
