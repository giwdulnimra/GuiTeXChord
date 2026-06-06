#include "LatexGenerator.h"
#include <QStringList>
#include <QtMath>

// ===========================================================================
// Common helpers
// ===========================================================================
QString LatexGenerator::romanNumeral(int n)
{
    static const char *th[] = {"","M","MM","MMM"};
    static const char *hu[] = {"","C","CC","CCC","CD","D","DC","DCC","DCCC","CM"};
    static const char *te[] = {"","X","XX","XXX","XL","L","LX","LXX","LXXX","XC"};
    static const char *on[] = {"","I","II","III","IV","V","VI","VII","VIII","IX"};
    if (n<=0||n>3999) return QString::number(n);
    return QString(th[n/1000])+hu[(n%1000)/100]+te[(n%100)/10]+on[n%10];
}

QString LatexGenerator::stateToStyle(NoteState s)
{
    switch(s){
    case NoteState::Root: return "root";
    case NoteState::Tone: return "tone";
    case NoteState::Mute: return "mute";
    default:              return "";
    }
}

QString LatexGenerator::commonStyles()
{
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
// VERTICAL layout  (klassische Grifftabelle, wie grifftabelle.tex)
//
//   Saiten  = Spalten  x-Achse: stringIdx 0 (E) links, N-1 (e') rechts
//   Bünde   = Zeilen   y-Achse: open-Zeile oben (y=3.1), Bund4 unten
//   Startbund-Label: links außen, rotiert, neben Bund-1-Zeile
// ===========================================================================
static constexpr double V_XDIST      = 0.4;   // horizontal string spacing factor
// y-positions from grifftabelle.tex:
static const double V_FRET_Y[]  = { 3.10, 2.25, 0.75, -0.75, -2.25 }; // fretRow 0..4
static const double V_GRID_Y[]  = { 3.00, 1.50,  0.00, -1.50 };        // fret dividers
static constexpr double V_FRAME_TOP    =  3.18;
static constexpr double V_FRAME_BOTTOM = -3.00;
static constexpr double V_LABEL_Y      =  3.60;  // string name labels
// Sattel: two close vertical lines between open col and fret-1 col
static constexpr double V_SATTEL_X1   = -0.15 * 1.0; // will be scaled per numStrings
static constexpr double V_SATTEL_X2   =  0.00;

static double vStringX(int idx, int N){
    return (2*idx - (N-1)) * V_XDIST;
}

QString LatexGenerator::tikzVertical(const ChordData &chord)
{
    QStringList L;
    const int N = chord.numStrings;
    double xLeft  = vStringX(0,   N);
    double xRight = vStringX(N-1, N);
    // Sattel x: midpoint between open-col and fret-1 col, squeezed together
    // We place them just left of the frame left edge
    double satX1 = xLeft - 0.25;
    double satX2 = xLeft - 0.10;

    // Name (always present, empty string if disabled → constant bbox)
    double nameCX = (xLeft + xRight) / 2.0;
    L << QString("  \\node[font=\\bfseries] at (%1,4.3) {%2};")
             .arg(nameCX,0,'f',3)
             .arg(chord.showName ? chord.fullName() : "");

    // String labels (E left, e' right)
    for(int s=0;s<N;++s){
        double x = vStringX(s,N);
        QString lbl = s<chord.tuning.size() ? chord.tuning[s] : "";
        L << QString("  \\node[] at (%1,%2) {%3};")
                 .arg(x,0,'f',3).arg(V_LABEL_Y).arg(lbl);
    }

    // Start fret label: rotated, left of frame, next to fret-1 row
    // Always emit (empty if not shown) for constant bbox
    double sfY = (V_FRET_Y[0] + V_FRET_Y[1]) / 2.0;  // between open and fret1
    // Actually spec: left of fret-1 row, not between open/fret1
    sfY = (V_FRET_Y[1] + V_FRET_Y[2]) / 2.0;
    double sfX = xLeft - 0.65;
    QString sfStr = (chord.showStartFret && chord.startFret>=1)
                    ? romanNumeral(chord.startFret) : "";
    L << QString("  \\node[rotate=90] at (%1,%2) {%3};")
             .arg(sfX,0,'f',3).arg(sfY,0,'f',3).arg(sfStr);

    // Frame
    L << QString("  \\draw (%1,%2) -- (%3,%4) -- (%3,%5) -- (%1,%5) -- cycle;")
             .arg(xLeft,0,'f',3).arg(V_FRAME_TOP)
             .arg(xRight,0,'f',3).arg(V_FRAME_TOP)
             .arg(V_FRAME_BOTTOM);

    // Inner vertical string lines
    for(int s=1;s<N-1;++s){
        double x=vStringX(s,N);
        L << QString("  \\draw (%1,%2) -- (%1,%3);")
                 .arg(x,0,'f',3).arg(V_FRAME_TOP).arg(V_FRAME_BOTTOM);
    }

    // Horizontal fret dividers (grid lines, not the open-string line)
    for(int g=0;g<4;++g)
        L << QString("  \\draw (%1,%3) -- (%2,%3);")
                 .arg(xLeft,0,'f',3).arg(xRight,0,'f',3).arg(V_GRID_Y[g]);

    // Sattel: two close vertical lines (left of leftmost string)
    L << QString("  \\draw[line width=0.8pt] (%1,%2) -- (%1,%3);")
             .arg(satX1,0,'f',3).arg(V_FRAME_TOP).arg(V_FRAME_BOTTOM);
    L << QString("  \\draw[line width=2.0pt] (%1,%2) -- (%1,%3);")
             .arg(satX2,0,'f',3).arg(V_FRAME_TOP).arg(V_FRAME_BOTTOM);

    // Barré
    if(chord.barre.active && chord.barre.fret>=1
       && chord.barre.fret<=ChordData::NUM_FRETS){
        int fr = chord.barre.fret;
        int sL = qMin(chord.barre.firstString, chord.barre.lastString);
        int sR = qMax(chord.barre.firstString, chord.barre.lastString);
        double bx1 = vStringX(sL,N), bx2 = vStringX(sR,N);
        double by  = V_FRET_Y[fr];
        L << QString("  \\draw[line width=5pt,line cap=round,opacity=0.85]"
                     " (%1,%3) -- (%2,%3);")
                 .arg(bx1,0,'f',3).arg(bx2,0,'f',3).arg(by,0,'f',3);
    }

    // Notes
    for(int s=0;s<N;++s){
        double x=vStringX(s,N);
        for(int f=0;f<=ChordData::NUM_FRETS;++f){
            NoteState st=chord.notes[s][f];
            if(st==NoteState::None) continue;
            QString sty=stateToStyle(st);
            double y=V_FRET_Y[f];
            L << QString("  \\node[%1] at (%2,%3) {};")
                     .arg(sty).arg(x,0,'f',3).arg(y,0,'f',3);
        }
    }

    return L.join('\n');
}

// ===========================================================================
// HORIZONTAL layout  (waagerechte Grifftabelle, horizontal-PDF)
//
//   Saiten  = Zeilen   y-Achse: e' (N-1) oben, E (0) unten
//   Bünde   = Spalten  x-Achse: open links, fret4 right
//   Startbund-Label: above frame, above fret-1 column
// ===========================================================================
static constexpr double H_YDIST     = 0.7;   // vertical string spacing
static constexpr double H_FRET_STEP = 1.5;   // horizontal fret spacing

// x positions of note columns
static const double H_FRET_X[] = {
    -0.55,                            // open / fret 0
     0.75,
     0.75 + 1*H_FRET_STEP,
     0.75 + 2*H_FRET_STEP,
     0.75 + 3*H_FRET_STEP
};
static const double H_GRID_X[] = {   // vertical fret dividers
    (H_FRET_X[1]+H_FRET_X[2])/2.0,
    (H_FRET_X[2]+H_FRET_X[3])/2.0,
    (H_FRET_X[3]+H_FRET_X[4])/2.0
};
static constexpr double H_FRAME_LEFT  = -1.0;
static constexpr double H_FRAME_RIGHT =  6.1;
static constexpr double H_LABEL_X     = -1.55; // string name labels
static constexpr double H_SATTEL_X1   = -0.15;
static constexpr double H_SATTEL_X2   =  0.10;

static double hStringY(int idx, int N){
    // idx 0=E (lowest) = bottom, idx N-1=e' = top
    return (idx - (N-1)/2.0) * H_YDIST;
}
static double hFretX(int f){
    return (f>=0&&f<=ChordData::NUM_FRETS) ? H_FRET_X[f] : H_FRET_X[ChordData::NUM_FRETS];
}

QString LatexGenerator::tikzHorizontal(const ChordData &chord)
{
    QStringList L;
    const int N = chord.numStrings;
    double yTop    = hStringY(N-1, N);
    double yBottom = hStringY(0,   N);

    // Constant bbox reserves
    double nameY    = yTop + 0.75;
    double startFretY = yTop + 0.38;

    // Chord name
    double cx = (H_FRAME_LEFT + H_FRAME_RIGHT) / 2.0;
    L << QString("  \\node[font=\\bfseries] at (%1,%2) {%3};")
             .arg(cx,0,'f',3).arg(nameY,0,'f',3)
             .arg(chord.showName ? chord.fullName() : "");

    // Start fret label above fret-1 column
    QString sfStr = (chord.showStartFret && chord.startFret>=1)
                    ? romanNumeral(chord.startFret) : "";
    L << QString("  \\node[anchor=south] at (%1,%2) {%3};")
             .arg(H_FRET_X[1],0,'f',3).arg(startFretY,0,'f',3).arg(sfStr);

    // String labels (e' top, E bottom)
    for(int s=0;s<N;++s){
        double y = hStringY(s,N);
        QString lbl = s<chord.tuning.size() ? chord.tuning[s] : "";
        L << QString("  \\node[anchor=east] at (%1,%2) {%3};")
                 .arg(H_LABEL_X,0,'f',3).arg(y,0,'f',3).arg(lbl);
    }

    // Frame
    L << QString("  \\draw (%1,%2) -- (%3,%2) -- (%3,%4) -- (%1,%4) -- cycle;")
             .arg(H_FRAME_LEFT,0,'f',3).arg(yTop,0,'f',3)
             .arg(H_FRAME_RIGHT,0,'f',3).arg(yBottom,0,'f',3);

    // Inner horizontal string lines
    for(int s=1;s<N-1;++s){
        double y=hStringY(s,N);
        L << QString("  \\draw (%1,%3) -- (%2,%3);")
                 .arg(H_FRAME_LEFT,0,'f',3).arg(H_FRAME_RIGHT,0,'f',3)
                 .arg(y,0,'f',3);
    }

    // Vertical fret dividers (between frets 1-4)
    for(int g=0;g<3;++g)
        L << QString("  \\draw (%1,%2) -- (%1,%3);")
                 .arg(H_GRID_X[g],0,'f',3).arg(yTop,0,'f',3)
                 .arg(yBottom,0,'f',3);

    // Sattel: two close vertical lines
    L << QString("  \\draw[line width=0.8pt] (%1,%2) -- (%1,%3);")
             .arg(H_SATTEL_X1,0,'f',3).arg(yTop,0,'f',3).arg(yBottom,0,'f',3);
    L << QString("  \\draw[line width=2.0pt] (%1,%2) -- (%1,%3);")
             .arg(H_SATTEL_X2,0,'f',3).arg(yTop,0,'f',3).arg(yBottom,0,'f',3);

    // Barré: vertical thick line on one fret column
    if(chord.barre.active && chord.barre.fret>=1
       && chord.barre.fret<=ChordData::NUM_FRETS){
        int fr = chord.barre.fret;
        int sL = qMin(chord.barre.firstString, chord.barre.lastString);
        int sR = qMax(chord.barre.firstString, chord.barre.lastString);
        double by1 = hStringY(sL,N), by2 = hStringY(sR,N);
        double bx  = hFretX(fr);
        L << QString("  \\draw[line width=5pt,line cap=round,opacity=0.85]"
                     " (%1,%2) -- (%1,%3);")
                 .arg(bx,0,'f',3).arg(by2,0,'f',3).arg(by1,0,'f',3);
    }

    // Notes
    for(int s=0;s<N;++s){
        double y=hStringY(s,N);
        for(int f=0;f<=ChordData::NUM_FRETS;++f){
            NoteState st=chord.notes[s][f];
            if(st==NoteState::None) continue;
            double x=hFretX(f);
            L << QString("  \\node[%1] at (%2,%3) {};")
                     .arg(stateToStyle(st)).arg(x,0,'f',3).arg(y,0,'f',3);
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
    doc << R"(\begin{tikzpicture})";
    if(chord.orientation == Orientation::Vertical)
        doc << tikzVertical(chord);
    else
        doc << tikzHorizontal(chord);
    doc << R"(\end{tikzpicture})";
    return doc.join('\n');
}

QString LatexGenerator::standaloneDocument(const ChordData &chord)
{
    QStringList doc;
    doc << R"(\documentclass[tikz,border=6pt]{standalone})";
    doc << R"(\usepackage{tikz})";
    doc << R"(\usetikzlibrary{shapes.geometric})";
    doc << R"(\pgfdeclarelayer{nodelayer})";
    doc << R"(\pgfdeclarelayer{edgelayer})";
    doc << R"(\pgfsetlayers{edgelayer,nodelayer,main})";
    doc << R"(\begin{document})";
    doc << tikzFragment(chord);
    doc << R"(\end{document})";
    return doc.join('\n');
}
