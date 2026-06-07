#include "LatexGenerator.h"
#include <QStringList>

// ===========================================================================
// Helpers
// ===========================================================================
QString LatexGenerator::romanNumeral(int n)
{
    static const char *th[]={"","M","MM","MMM"};
    static const char *hu[]={"","C","CC","CCC","CD","D","DC","DCC","DCCC","CM"};
    static const char *te[]={"","X","XX","XXX","XL","L","LX","LXX","LXXX","XC"};
    static const char *on[]={"","I","II","III","IV","V","VI","VII","VIII","IX"};
    if(n<=0||n>3999) return QString::number(n);
    return QString(th[n/1000])+hu[(n%1000)/100]+te[(n%100)/10]+on[n%10];
}

QString LatexGenerator::stateToStyle(NoteState s) {
    switch(s){
    case NoteState::Root: return "root";
    case NoteState::Tone: return "tone";
    case NoteState::Mute: return "mute";
    default: return "";
    }
}

QString LatexGenerator::commonStyles() {
    return
R"(\tikzset{
  root/.style={draw=black,shape=circle,fill=white,minimum size=8pt,inner sep=0pt},
  tone/.style={draw=black,shape=circle,fill=black,minimum size=7pt,inner sep=0pt},
  mute/.style={draw=none,shape=rectangle,rounded corners=.7pt,minimum size=8pt,
    path picture={\draw[line width=1pt,color=black]
      (path picture bounding box.south west)--(path picture bounding box.north east)
      (path picture bounding box.north west)--(path picture bounding box.south east);}},
})";
}

// ===========================================================================
// HORIZONTAL  —  chord_h.tex
//
// xdist = 0.4
// Saiten = Zeilen, y = s_idx * 2 * xdist - (N-1)*xdist
//   idx 0 (E)  → y = -(N-1)*xdist   (unten)
//   idx N-1(e')→ y = +(N-1)*xdist   (oben)
// Spalten:
//   open:   x = -3.1          (auf dem Sattel, zwischen Rahmenrand -3.18 und Gitter -3.0)
//   fret 1: x = -2.25
//   fret 2: x = -0.75
//   fret 3: x = +0.75
//   fret 4: x = +2.25
// Gitternetz (vertikale Trennlinien): x = -3.0, -1.5, 0.0, +1.5
// Rahmen: x von -3.18 bis +3.0,  y von -(N-1)*xdist bis +(N-1)*xdist
// Saiten-Labels: x = -3.5
// Startbund-Label: x = fret1_x = -2.25, y = (N-1)*xdist + 6*xdist/N  (oberhalb)
// Barré: filled bend-right-45 path zwischen zwei Saiten am selben Bund
// ===========================================================================

static constexpr double XD = 0.4; // xdist

// x-Positionen der Bund-Spalten — fretRow 0 (open) bis 5
static const double H_COL_X[] = { -3.1, -2.25, -0.75, 0.75, 2.25, 3.75 };
// Gitternetz-Trennlinien (5 Linien für 5 Bünde)
static const double H_GRID_X[] = { -3.0, -1.5, 0.0, 1.5, 3.0 };
static constexpr double H_FRAME_LEFT  = -3.18;
static constexpr double H_FRAME_RIGHT =  4.50;  // erweitert für 5. Bund
static constexpr double H_LABEL_X     = -3.5;

static double hSY(int idx, int N) {
    // idx 0 = E (lowest) = bottom = most negative y
    return (2*idx - (N-1)) * XD;
}

static double hColX(int f) {
    if(f >= 0 && f <= ChordData::NUM_FRETS) return H_COL_X[f];
    return H_COL_X[ChordData::NUM_FRETS];
}

QString LatexGenerator::tikzHorizontal(const ChordData &chord)
{
    QStringList L;
    const int N = chord.numStrings;
    double yTop = hSY(N-1, N);   // e', oben
    double yBot = hSY(0,   N);   // E,  unten

    // Constant-bbox nodes (always emit, empty text if not shown)
    // Chord name: centred above frame
    double nameCX = (H_FRAME_LEFT + H_FRAME_RIGHT) / 2.0;
    double nameY  = yTop + 6.5*XD;
    L << QString("  \\node[font=\\bfseries] at (%1,%2) {%3};")
             .arg(nameCX,0,'f',3).arg(nameY,0,'f',3)
             .arg(chord.showName ? chord.fullName() : "");

    // Startfret label above fret-1 column
    double sfY = yTop + 2.5*XD;
    L << QString("  \\node[] at (%1,%2) {%3};")
             .arg(H_COL_X[1],0,'f',3).arg(sfY,0,'f',3)
             .arg((chord.showStartFret && chord.startFret >= 1)
                  ? romanNumeral(chord.startFret) : "");

    // String labels
    for(int s = 0; s < N; ++s) {
        QString lbl = s < chord.tuning.size() ? chord.tuning[s] : "";
        L << QString("  \\node[] at (%1,%2) {%3};")
                 .arg(H_LABEL_X).arg(hSY(s,N),0,'f',3).arg(lbl);
    }

    // Frame
    L << QString("  \\draw (%1,%2) -- (%1,%3) -- (%4,%3) -- (%4,%2) -- cycle;")
             .arg(H_FRAME_LEFT).arg(yTop,0,'f',3)
             .arg(yBot,0,'f',3).arg(H_FRAME_RIGHT);

    // Inner horizontal string lines (not the outer two, they are the frame)
    for(int s = 1; s < N-1; ++s)
        L << QString("  \\draw (%1,%3) -- (%2,%3);")
                 .arg(H_FRAME_LEFT).arg(H_FRAME_RIGHT).arg(hSY(s,N),0,'f',3);

    // Vertical fret dividers (5 lines for 5 frets)
    for(int g = 0; g < 5; ++g)
        L << QString("  \\draw (%1,%2) -- (%1,%3);")
                 .arg(H_GRID_X[g]).arg(yTop,0,'f',3).arg(yBot,0,'f',3);

    // Barré: filled bend-right-45 path (horizontal, from low string to high string)
    // Mirrors: \draw[fill=black,bend right=45] (hi4.east) to (hi4.north) to (hi4.west)
    //          -- (lo4.west) to (lo4.south) to (lo4.east) -- cycle;
    if(chord.barre.active && chord.barre.fret >= 1
       && chord.barre.fret <= ChordData::NUM_FRETS) {
        int fr = chord.barre.fret;
        int sLo = qMin(chord.barre.firstString, chord.barre.lastString);
        int sHi = qMax(chord.barre.firstString, chord.barre.lastString);
        if(sLo < sHi) {
            double bx  = hColX(fr);
            double yLo = hSY(sLo, N);
            double yHi = hSY(sHi, N);
            // Named helper nodes for the barré path
            L << QString("  \\node (barLo) at (%1,%2) {};").arg(bx,0,'f',3).arg(yLo,0,'f',3);
            L << QString("  \\node (barHi) at (%1,%2) {};").arg(bx,0,'f',3).arg(yHi,0,'f',3);
            L << "  \\draw[fill=black,bend right=45]"
                 " (barHi.east) to (barHi.north) to (barHi.west)"
                 " -- (barLo.west) to (barLo.south) to (barLo.east) -- cycle;";
        }
    }

    // Notes
    for(int s = 0; s < N; ++s) {
        double y = hSY(s, N);
        for(int f = 0; f <= ChordData::NUM_FRETS; ++f) {
            NoteState st = chord.notes[s][f];
            if(st == NoteState::None) continue;
            L << QString("  \\node[%1] at (%2,%3) {};")
                     .arg(stateToStyle(st)).arg(hColX(f),0,'f',3).arg(y,0,'f',3);
        }
    }

    return L.join('\n');
}

// ===========================================================================
// VERTICAL  —  chord_v.tex
//
// xdist = 0.4
// Saiten = Spalten, x = (2*s - (N-1)) * xdist
//   idx 0 (E)  → x = -(N-1)*xdist   (links)
//   idx N-1(e')→ x = +(N-1)*xdist   (rechts)
// Zeilen:
//   open:   y = 3.1           (an der oberen Rahmenlinie)
//   fret 1: y = 2.25
//   fret 2: y = 0.75
//   fret 3: y = -0.75
//   fret 4: y = -2.25
// Gitternetz (horizontale Trennlinien): y = 3.0, 1.5, 0.0, -1.5
// Rahmen: y von 3.18 bis -3.0,  x von -(N-1)*xdist bis +(N-1)*xdist
// Saiten-Labels: y = 3.6
// Startbund-Label: x = -(N-1)*xdist - 6*xdist, y = 2.25  (links, ggf. rotate=90)
// Barré: filled bend-right-45 path zwischen zwei Saiten am selben Bund
// ===========================================================================

static const double V_ROW_Y[] = { 3.1, 2.25, 0.75, -0.75, -2.25, -3.75 }; // fretRow 0..5
static const double V_GRID_Y[] = { 3.0, 1.5, 0.0, -1.5, -3.0 };
static constexpr double V_FRAME_TOP    =  3.18;
static constexpr double V_FRAME_BOTTOM = -4.50;  // erweitert für 5. Bund
static constexpr double V_LABEL_Y      =  3.6;

static double vSX(int idx, int N) {
    return (2*idx - (N-1)) * XD;
}
static double vRowY(int f) {
    if(f >= 0 && f <= ChordData::NUM_FRETS) return V_ROW_Y[f];
    return V_ROW_Y[ChordData::NUM_FRETS];
}

QString LatexGenerator::tikzVertical(const ChordData &chord)
{
    QStringList L;
    const int N = chord.numStrings;
    double xL = vSX(0,   N);  // E, links
    double xR = vSX(N-1, N);  // e', rechts

    // Chord name centred above
    double nameCX = (xL + xR) / 2.0;
    double nameY  = V_LABEL_Y + 0.6;
    L << QString("  \\node[font=\\bfseries] at (%1,%2) {%3};")
             .arg(nameCX,0,'f',3).arg(nameY,0,'f',3)
             .arg(chord.showName ? chord.fullName() : "");

    // Startfret: left of frame, at fret-1 y, rotate=90 (or as in TeX: no rotate, just left)
    // chord_v.tex: \node[](startfret) at (-6*xdist, 2.25) {IV}; %rotate=90
    double sfX = xL - 6.0*XD;
    L << QString("  \\node[rotate=90] at (%1,%2) {%3};")
             .arg(sfX,0,'f',3).arg(V_ROW_Y[1],0,'f',3)
             .arg((chord.showStartFret && chord.startFret >= 1)
                  ? romanNumeral(chord.startFret) : "");

    // String labels
    for(int s = 0; s < N; ++s) {
        QString lbl = s < chord.tuning.size() ? chord.tuning[s] : "";
        L << QString("  \\node[] at (%1,%2) {%3};")
                 .arg(vSX(s,N),0,'f',3).arg(V_LABEL_Y).arg(lbl);
    }

    // Frame
    L << QString("  \\draw (%1,%2) -- (%3,%2) -- (%3,%4) -- (%1,%4) -- cycle;")
             .arg(xL,0,'f',3).arg(V_FRAME_TOP)
             .arg(xR,0,'f',3).arg(V_FRAME_BOTTOM);

    // Inner vertical string lines (not outer two = frame edges)
    for(int s = 1; s < N-1; ++s) {
        double x = vSX(s, N);
        L << QString("  \\draw (%1,%2) -- (%1,%3);")
                 .arg(x,0,'f',3).arg(V_FRAME_TOP).arg(V_FRAME_BOTTOM);
    }

    // Horizontal fret dividers (5 lines for 5 frets)
    for(int g = 0; g < 5; ++g)
        L << QString("  \\draw (%1,%3) -- (%2,%3);")
                 .arg(xL,0,'f',3).arg(xR,0,'f',3).arg(V_GRID_Y[g]);

    // Barré: filled bend-right-45 path
    // chord_v.tex: (e4.south) to (e4.east) to (e4.north) -- (g4.north) to (g4.west) to (g4.south)
    // = right-side cap at high-string (e'), left-side cap at low-string
    if(chord.barre.active && chord.barre.fret >= 1
       && chord.barre.fret <= ChordData::NUM_FRETS) {
        int fr = chord.barre.fret;
        int sLo = qMin(chord.barre.firstString, chord.barre.lastString);
        int sHi = qMax(chord.barre.firstString, chord.barre.lastString);
        if(sLo < sHi) {
            double by  = vRowY(fr);
            double xLo = vSX(sLo, N);
            double xHi = vSX(sHi, N);
            L << QString("  \\node (barLo) at (%1,%2) {};").arg(xLo,0,'f',3).arg(by,0,'f',3);
            L << QString("  \\node (barHi) at (%1,%2) {};").arg(xHi,0,'f',3).arg(by,0,'f',3);
            L << "  \\draw[fill=black,bend right=45]"
                 " (barHi.south) to (barHi.east) to (barHi.north)"
                 " -- (barLo.north) to (barLo.west) to (barLo.south) -- cycle;";
        }
    }

    // Notes
    for(int s = 0; s < N; ++s) {
        double x = vSX(s, N);
        for(int f = 0; f <= ChordData::NUM_FRETS; ++f) {
            NoteState st = chord.notes[s][f];
            if(st == NoteState::None) continue;
            L << QString("  \\node[%1] at (%2,%3) {};")
                     .arg(stateToStyle(st)).arg(x,0,'f',3).arg(vRowY(f),0,'f',3);
        }
    }

    return L.join('\n');
}

// ===========================================================================
// Public API
// ===========================================================================
QString LatexGenerator::tikzFragment(const ChordData &chord)
{
    QStringList doc;
    doc << commonStyles();
    doc << R"(\begin{pgfonlayer}{nodelayer})";
    doc << (chord.orientation == Orientation::Horizontal
            ? tikzHorizontal(chord) : tikzVertical(chord));
    doc << R"(\end{pgfonlayer})";
    // edgelayer is handled inline via \draw commands above (pgfonlayer not strictly needed
    // for simple draws, but we wrap for compatibility with the user's template)
    return doc.join('\n');
}

QString LatexGenerator::standaloneDocument(const ChordData &chord)
{
    QStringList doc;
    doc << R"(\documentclass[tikz,border=2pt]{standalone})";
    doc << R"(\usepackage{pgfplots})";
    doc << R"(\usepgfplotslibrary{groupplots})";
    doc << R"(\usepackage{pgfplotstable})";
    doc << R"(\pgfdeclarelayer{nodelayer})";
    doc << R"(\pgfdeclarelayer{edgelayer})";
    doc << R"(\pgfdeclarelayer{backgroundlayer})";
    doc << R"(\pgfsetlayers{backgroundlayer,edgelayer,nodelayer,main})";
    doc << commonStyles();
    doc << R"(\begin{document})";
    doc << R"(\newcommand{\xdist}{0.4})";
    doc << R"(\begin{tikzpicture})";
    doc << R"(\begin{pgfonlayer}{nodelayer})";
    doc << (chord.orientation == Orientation::Horizontal
            ? tikzHorizontal(chord) : tikzVertical(chord));
    doc << R"(\end{pgfonlayer})";
    doc << R"(\end{tikzpicture})";
    doc << R"(\end{document})";
    return doc.join('\n');
}
