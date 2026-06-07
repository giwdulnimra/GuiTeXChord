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
QString LatexGenerator::stateToStyle(NoteState s){
    switch(s){
    case NoteState::Root: return "root";
    case NoteState::Tone: return "tone";
    case NoteState::Mute: return "mute";
    default: return "";
    }
}
QString LatexGenerator::commonStyles(){
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
// VERTICAL  (klassische Grifftabelle, senkrecht)
//
// Koordinaten aus grifftabelle.tex:
//   Saiten = Spalten: stringIdx 0 (E) links, N-1 (e') rechts
//   x = (2*s - (N-1)) * 0.4
//   Bünde = Zeilen:
//     fretRow 0 (open): y = 3.10   (auf der Sattellinie)
//     fretRow 1..5:     y = 2.25, 0.75, -0.75, -2.25, -3.75
//   Sattel = zwei dichte horizontale Linien links des Rahmens, oben zwischen
//            open-Zeile und fret-1-Zeile
//   Startbund-Label: rotiert, links außen, neben Mitte von fret-1 und fret-2
// ===========================================================================
static constexpr double V_XDIST = 0.4;
// y-Positionen: open-Zeile auf Sattellinie, dann 5 Bünde
static const double V_FRET_Y[] = { 3.10, 2.25, 0.75, -0.75, -2.25, -3.75 };
// Gitternetz-Trennlinien (zwischen Bünden)
static const double V_GRID_Y[] = { 3.0, 1.5, 0.0, -1.5, -3.0 };
static constexpr double V_FRAME_TOP    =  3.18;
static constexpr double V_FRAME_BOTTOM = -4.50;
static constexpr double V_LABEL_Y      =  3.65;

static double vSX(int idx, int N){ return (2*idx-(N-1))*V_XDIST; }

QString LatexGenerator::tikzVertical(const ChordData &chord)
{
    QStringList L;
    const int N = chord.numStrings;
    double xL = vSX(0,N), xR = vSX(N-1,N);

    // Chord name (constant bbox: always emit, empty if disabled)
    L << QString("  \\node[font=\\bfseries] at (%1,4.40) {%2};")
             .arg((xL+xR)/2.0,0,'f',3)
             .arg(chord.showName ? chord.fullName() : "");

    // Startbund: rotiert, links außen, neben fret-1/fret-2-Mitte
    // Immer emittieren (leer wenn inaktiv) → konstante BBox
    double sfY = (V_FRET_Y[1]+V_FRET_Y[2])/2.0;
    double sfX = xL - 0.70;
    L << QString("  \\node[rotate=90] at (%1,%2) {%3};")
             .arg(sfX,0,'f',3).arg(sfY,0,'f',3)
             .arg((chord.showStartFret&&chord.startFret>=1)
                  ? romanNumeral(chord.startFret) : "");

    // String labels
    for(int s=0;s<N;++s)
        L << QString("  \\node[] at (%1,%2) {%3};")
                 .arg(vSX(s,N),0,'f',3).arg(V_LABEL_Y)
                 .arg(s<chord.tuning.size()?chord.tuning[s]:"");

    // Frame
    L << QString("  \\draw (%1,%2) -- (%3,%2) -- (%3,%4) -- (%1,%4) -- cycle;")
             .arg(xL,0,'f',3).arg(V_FRAME_TOP)
             .arg(xR,0,'f',3).arg(V_FRAME_BOTTOM);

    // Inner vertical string lines
    for(int s=1;s<N-1;++s){
        double x=vSX(s,N);
        L << QString("  \\draw (%1,%2) -- (%1,%3);")
                 .arg(x,0,'f',3).arg(V_FRAME_TOP).arg(V_FRAME_BOTTOM);
    }

    // Horizontal fret dividers (5 lines for 5 frets + open section)
    for(int g=0;g<5;++g)
        L << QString("  \\draw (%1,%3) -- (%2,%3);")
                 .arg(xL,0,'f',3).arg(xR,0,'f',3).arg(V_GRID_Y[g]);

    // Sattel: zwei dichte horizontale Linien über dem Frame-Top (Querbalken oben)
    // Die open-Zeile (y=3.10) sitzt direkt an der oberen Rahmenlinie (y=3.18)
    // → Sattel als zwei enge horizontale Linien oberhalb des Rahmens
    L << QString("  \\draw[line width=0.8pt] (%1,%3) -- (%2,%3);")
             .arg(xL,0,'f',3).arg(xR,0,'f',3).arg(3.30);
    L << QString("  \\draw[line width=2.2pt] (%1,%3) -- (%2,%3);")
             .arg(xL,0,'f',3).arg(xR,0,'f',3).arg(3.50);

    // Barré: horizontale dicke Linie
    if(chord.barre.active&&chord.barre.fret>=1
       &&chord.barre.fret<=ChordData::NUM_FRETS){
        int fr=chord.barre.fret;
        int s1=qMin(chord.barre.firstString,chord.barre.lastString);
        int s2=qMax(chord.barre.firstString,chord.barre.lastString);
        L << QString("  \\draw[line width=5pt,line cap=round,opacity=0.85]"
                     " (%1,%3) -- (%2,%3);")
                 .arg(vSX(s1,N),0,'f',3).arg(vSX(s2,N),0,'f',3)
                 .arg(V_FRET_Y[fr],0,'f',3);
    }

    // Notes
    for(int s=0;s<N;++s)
        for(int f=0;f<=ChordData::NUM_FRETS;++f){
            NoteState st=chord.notes[s][f];
            if(st==NoteState::None) continue;
            QString sty=stateToStyle(st);
            L << QString("  \\node[%1] at (%2,%3) {};")
                     .arg(sty).arg(vSX(s,N),0,'f',3).arg(V_FRET_Y[f],0,'f',3);
        }
    return L.join('\n');
}

// ===========================================================================
// HORIZONTAL  (waagerechte Grifftabelle)
//
//   Saiten = Zeilen: e'(N-1) oben, E(0) unten
//   Bünde  = Spalten: open links (auf Sattellinie), fret 1..5 rechts
//
//   Sattel: zwei dichte VERTIKALE Linien zwischen open-Spalte und fret-1-Spalte
//   Noten auf fretRow 0 (open) sitzen AUF der Sattellinie (x = Sattel-Mitte)
//   Startbund-Label: oberhalb fret-1-Spalte
// ===========================================================================
static constexpr double H_YDIST    = 0.7;
static constexpr double H_FSTEP    = 1.5;

// Sattel-x-Positionen (zwei enge Linien)
static constexpr double H_SAT1 = -0.12;
static constexpr double H_SAT2 =  0.08;
// open-Noten sitzen auf der Sattelmitte
static constexpr double H_OPEN_X = (H_SAT1+H_SAT2)/2.0;

// Fret-1..5 x-Positionen gleichmäßig rechts des Sattels
static constexpr double H_FRET1_X = 0.75;
static double hFretX(int f){
    if(f==0) return H_OPEN_X;
    return H_FRET1_X + (f-1)*H_FSTEP;
}
// Gitternetz-Trennlinien zwischen fret 1..5 (4 Linien)
static double hGridX(int g){ return (hFretX(g+1)+hFretX(g+2))/2.0; }

static constexpr double H_FRAME_LEFT  = -0.70;
static constexpr double H_FRAME_RIGHT =  6.10;
static constexpr double H_LABEL_X     = -1.20;

static double hSY(int idx,int N){ return (idx-(N-1)/2.0)*H_YDIST; }

QString LatexGenerator::tikzHorizontal(const ChordData &chord)
{
    QStringList L;
    const int N = chord.numStrings;
    double yTop=hSY(N-1,N), yBot=hSY(0,N);

    // Constant bbox reserves
    double nameY = yTop+0.75;
    double sfY   = yTop+0.38;

    // Chord name
    L << QString("  \\node[font=\\bfseries] at (%1,%2) {%3};")
             .arg((H_FRAME_LEFT+H_FRAME_RIGHT)/2.0,0,'f',3).arg(nameY,0,'f',3)
             .arg(chord.showName ? chord.fullName() : "");

    // Startbund above fret-1
    L << QString("  \\node[anchor=south] at (%1,%2) {%3};")
             .arg(hFretX(1),0,'f',3).arg(sfY,0,'f',3)
             .arg((chord.showStartFret&&chord.startFret>=1)
                  ? romanNumeral(chord.startFret) : "");

    // String labels
    for(int s=0;s<N;++s)
        L << QString("  \\node[anchor=east] at (%1,%2) {%3};")
                 .arg(H_LABEL_X,0,'f',3).arg(hSY(s,N),0,'f',3)
                 .arg(s<chord.tuning.size()?chord.tuning[s]:"");

    // Frame
    L << QString("  \\draw (%1,%2) -- (%3,%2) -- (%3,%4) -- (%1,%4) -- cycle;")
             .arg(H_FRAME_LEFT,0,'f',3).arg(yTop,0,'f',3)
             .arg(H_FRAME_RIGHT,0,'f',3).arg(yBot,0,'f',3);

    // Inner horizontal string lines
    for(int s=1;s<N-1;++s)
        L << QString("  \\draw (%1,%3) -- (%2,%3);")
                 .arg(H_FRAME_LEFT,0,'f',3).arg(H_FRAME_RIGHT,0,'f',3)
                 .arg(hSY(s,N),0,'f',3);

    // Vertical fret dividers between frets 1-5 (4 lines)
    for(int g=0;g<4;++g)
        L << QString("  \\draw (%1,%2) -- (%1,%3);")
                 .arg(hGridX(g),0,'f',3).arg(yTop,0,'f',3).arg(yBot,0,'f',3);

    // Sattel: zwei dichte vertikale Linien (kein dritter Strich!)
    L << QString("  \\draw[line width=0.8pt] (%1,%2) -- (%1,%3);")
             .arg(H_SAT1,0,'f',3).arg(yTop,0,'f',3).arg(yBot,0,'f',3);
    L << QString("  \\draw[line width=2.2pt] (%1,%2) -- (%1,%3);")
             .arg(H_SAT2,0,'f',3).arg(yTop,0,'f',3).arg(yBot,0,'f',3);

    // Barré: vertikale dicke Linie
    if(chord.barre.active&&chord.barre.fret>=1
       &&chord.barre.fret<=ChordData::NUM_FRETS){
        int fr=chord.barre.fret;
        int s1=qMin(chord.barre.firstString,chord.barre.lastString);
        int s2=qMax(chord.barre.firstString,chord.barre.lastString);
        L << QString("  \\draw[line width=5pt,line cap=round,opacity=0.85]"
                     " (%1,%2) -- (%1,%3);")
                 .arg(hFretX(fr),0,'f',3)
                 .arg(hSY(s2,N),0,'f',3).arg(hSY(s1,N),0,'f',3);
    }

    // Notes: fretRow 0 auf Sattelmitte (H_OPEN_X), fretRow 1..5 auf hFretX
    for(int s=0;s<N;++s)
        for(int f=0;f<=ChordData::NUM_FRETS;++f){
            NoteState st=chord.notes[s][f];
            if(st==NoteState::None) continue;
            L << QString("  \\node[%1] at (%2,%3) {};")
                     .arg(stateToStyle(st))
                     .arg(hFretX(f),0,'f',3).arg(hSY(s,N),0,'f',3);
        }
    return L.join('\n');
}

// ===========================================================================
QString LatexGenerator::tikzFragment(const ChordData &chord)
{
    QStringList doc;
    doc << commonStyles();
    doc << R"(\begin{tikzpicture})";
    doc << (chord.orientation==Orientation::Vertical
            ? tikzVertical(chord) : tikzHorizontal(chord));
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
