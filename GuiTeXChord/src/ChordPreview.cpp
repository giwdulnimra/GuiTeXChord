#include "ChordPreview.h"
#include <QPainter>
#include <QPainterPath>

// ---------------------------------------------------------------------------
// Same constants as LatexGenerator
// ---------------------------------------------------------------------------
static constexpr double XD = 0.4;

// Horizontal
static const double H_COL_X[]  = { -3.1, -2.25, -0.75, 0.75, 2.25, 3.75 };
static const double H_GRID_X[] = { -3.0, -1.5, 0.0, 1.5, 3.0 };
static constexpr double H_FRAME_LEFT  = -3.18;
static constexpr double H_FRAME_RIGHT =  4.50;
static constexpr double H_LABEL_X     = -3.5;
static double hSY(int idx, int N) { return (2*idx - (N-1)) * XD; }
static double hColX(int f) { return (f>=0&&f<=ChordData::NUM_FRETS)?H_COL_X[f]:H_COL_X[ChordData::NUM_FRETS]; }

// Vertical
static const double V_ROW_Y[]  = { 3.1, 2.25, 0.75, -0.75, -2.25, -3.75 };
static const double V_GRID_Y[] = { 3.0, 1.5, 0.0, -1.5, -3.0 };
static constexpr double V_FRAME_TOP    =  3.18;
static constexpr double V_FRAME_BOTTOM = -4.50;
static constexpr double V_LABEL_Y      =  3.6;
static double vSX(int idx, int N) { return (2*idx-(N-1))*XD; }
static double vRowY(int f) { return (f>=0&&f<=ChordData::NUM_FRETS)?V_ROW_Y[f]:V_ROW_Y[ChordData::NUM_FRETS]; }

// ---------------------------------------------------------------------------
static void drawNote(QPainter &p, QPointF pt, NoteState st, double r=5.0) {
    switch(st) {
    case NoteState::Tone:
        p.setPen(QPen(Qt::black,1)); p.setBrush(Qt::black);
        p.drawEllipse(pt,r,r); break;
    case NoteState::Root:
        p.setPen(QPen(Qt::black,1.5)); p.setBrush(Qt::white);
        p.drawEllipse(pt,r,r);
        p.setPen(Qt::NoPen); p.setBrush(Qt::black);
        p.drawEllipse(pt,r*0.38,r*0.38);
        p.setPen(QPen(Qt::black,1)); p.setBrush(Qt::NoBrush); break;
    case NoteState::Mute: {
        p.setPen(QPen(Qt::black,1.5)); double d=r*0.75;
        p.drawLine(pt+QPointF(-d,-d),pt+QPointF(d,d));
        p.drawLine(pt+QPointF(-d, d),pt+QPointF(d,-d)); break; }
    default: break;
    }
}

// Draw filled barré capsule between two points
static void drawBarre(QPainter &p, QPointF ptA, QPointF ptB, double r=5.5) {
    // ptA = one end, ptB = other end; capsule around the line segment
    QPointF d = ptB - ptA;
    double len = std::sqrt(d.x()*d.x() + d.y()*d.y());
    if(len < 1.0) return;
    // Perpendicular unit vector
    QPointF perp(-d.y()/len * r, d.x()/len * r);
    QPainterPath path;
    path.moveTo(ptA + perp);
    path.lineTo(ptB + perp);
    // Arc around ptB
    QRectF rcB(ptB.x()-r, ptB.y()-r, 2*r, 2*r);
    double angB = std::atan2(-perp.y(), -perp.x()) * 180.0 / M_PI;
    path.arcTo(rcB, angB, -180.0);
    path.lineTo(ptA - perp);
    // Arc around ptA
    QRectF rcA(ptA.x()-r, ptA.y()-r, 2*r, 2*r);
    double angA = std::atan2(perp.y(), perp.x()) * 180.0 / M_PI;
    path.arcTo(rcA, angA, -180.0);
    path.closeSubpath();
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::black);
    p.drawPath(path);
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(Qt::black,1));
}

// ---------------------------------------------------------------------------
ChordPreview::ChordPreview(QWidget *parent) : QWidget(parent) {
    setMinimumSize(100,80);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}
void ChordPreview::setChord(const ChordData &c) { m_chord=c; update(); }

QPointF ChordPreview::toPixel(double lx, double ly) const {
    return { m_ox + lx*m_sx,  m_oy - ly*m_sy };
}

void ChordPreview::setupTransform(double logL, double logR, double logB, double logT,
                                   double mL, double mR, double mT, double mB)
{
    double usW = width()  - mL - mR;
    double usH = height() - mT - mB;
    if(usW<1||usH<1){ m_sx=m_sy=1; m_ox=mL; m_oy=mT; return; }
    double sc = qMin(usW/(logR-logL), usH/(logT-logB));
    m_sx = m_sy = sc;
    double dW = (logR-logL)*sc, dH = (logT-logB)*sc;
    m_ox = mL + (usW-dW)/2.0 - logL*sc;
    m_oy = mT + (usH-dH)/2.0 + logT*sc;
}

void ChordPreview::paintEvent(QPaintEvent*) {
    if(m_chord.numStrings < 2) return;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    if(m_chord.orientation == Orientation::Horizontal) drawHorizontal(p);
    else                                                drawVertical(p);
}

// ---------------------------------------------------------------------------
void ChordPreview::drawHorizontal(QPainter &p)
{
    const ChordData &c = m_chord; const int N = c.numStrings;
    double yTop = hSY(N-1, N);
    double yBot = hSY(0,   N);

    // Logical bounding box (constant, matches TeX output)
    double logL = H_LABEL_X - 0.3;
    double logR = H_FRAME_RIGHT + 0.3;
    double logT = yTop + 7.5*XD;  // name + startfret reserve
    double logB = yBot - 0.3;

    setupTransform(logL, logR, logB, logT, 6, 6, 6, 6);

    p.setPen(QPen(Qt::black, 1.2));

    // Chord name
    if(c.showName && !c.fullName().isEmpty()) {
        QFont f=p.font(); f.setBold(true); f.setPointSize(9); p.setFont(f);
        auto pt = toPixel((H_FRAME_LEFT+H_FRAME_RIGHT)/2.0, yTop+6.5*XD);
        p.drawText(QRectF(pt.x()-55,pt.y()-9,110,16), Qt::AlignCenter, c.fullName());
    }
    // Start fret
    if(c.showStartFret && c.startFret >= 1) {
        static const char*rom[]={"","I","II","III","IV","V","VI","VII","VIII","IX","X",
            "XI","XII","XIII","XIV","XV","XVI","XVII","XVIII","XIX","XX","XXI","XXII","XXIII","XXIV"};
        QFont f=p.font(); f.setBold(false); f.setPointSize(8); p.setFont(f);
        auto pt = toPixel(H_COL_X[1], yTop+2.5*XD);
        p.drawText(QRectF(pt.x()-15,pt.y()-8,30,14), Qt::AlignCenter,
                   rom[qBound(1,c.startFret,24)]);
    }
    // String labels
    { QFont f=p.font(); f.setBold(true); f.setPointSize(7); p.setFont(f);
      for(int s=0;s<N;++s) {
          auto pt = toPixel(H_LABEL_X, hSY(s,N));
          p.drawText(QRectF(pt.x()-20,pt.y()-8,20,14),
                     Qt::AlignRight|Qt::AlignVCenter,
                     s<c.tuning.size()?c.tuning[s]:"");
      }
    }

    p.setPen(QPen(Qt::black,1.2)); p.setBrush(Qt::NoBrush);

    // Frame
    p.drawRect(QRectF(toPixel(H_FRAME_LEFT,yTop), toPixel(H_FRAME_RIGHT,yBot)));

    // Inner string lines
    for(int s=1;s<N-1;++s)
        p.drawLine(toPixel(H_FRAME_LEFT,hSY(s,N)), toPixel(H_FRAME_RIGHT,hSY(s,N)));

    // Fret dividers (vertical, 5 for 5 frets)
    for(int g=0;g<5;++g)
        p.drawLine(toPixel(H_GRID_X[g],yTop), toPixel(H_GRID_X[g],yBot));

    // Barré
    if(c.barre.active && c.barre.fret>=1 && c.barre.fret<=ChordData::NUM_FRETS) {
        int fr=c.barre.fret;
        int sLo=qMin(c.barre.firstString,c.barre.lastString);
        int sHi=qMax(c.barre.firstString,c.barre.lastString);
        if(sLo < sHi)
            drawBarre(p, toPixel(hColX(fr),hSY(sLo,N)),
                         toPixel(hColX(fr),hSY(sHi,N)));
    }

    // Notes
    for(int s=0;s<N;++s)
        for(int f=0;f<=ChordData::NUM_FRETS;++f) {
            if(c.notes[s][f]==NoteState::None) continue;
            drawNote(p, toPixel(hColX(f),hSY(s,N)), c.notes[s][f]);
        }
}

// ---------------------------------------------------------------------------
void ChordPreview::drawVertical(QPainter &p)
{
    const ChordData &c = m_chord; const int N = c.numStrings;
    double xL = vSX(0,   N);
    double xR = vSX(N-1, N);

    double logL = xL - 7.0*XD;   // startfret label space
    double logR = xR + 0.5*XD;
    double logT = V_LABEL_Y + 0.9; // name reserve
    double logB = V_FRAME_BOTTOM - 0.3;

    setupTransform(logL, logR, logB, logT, 6, 6, 6, 6);

    p.setPen(QPen(Qt::black,1.2));

    // Chord name
    if(c.showName && !c.fullName().isEmpty()) {
        QFont f=p.font(); f.setBold(true); f.setPointSize(9); p.setFont(f);
        auto pt = toPixel((xL+xR)/2.0, V_LABEL_Y+0.65);
        p.drawText(QRectF(pt.x()-55,pt.y()-9,110,16), Qt::AlignCenter, c.fullName());
    }
    // String labels
    { QFont f=p.font(); f.setBold(true); f.setPointSize(7); p.setFont(f);
      for(int s=0;s<N;++s) {
          auto pt = toPixel(vSX(s,N), V_LABEL_Y);
          p.drawText(QRectF(pt.x()-12,pt.y()-8,24,14), Qt::AlignCenter,
                     s<c.tuning.size()?c.tuning[s]:"");
      }
    }
    // Start fret (rotated)
    if(c.showStartFret && c.startFret >= 1) {
        static const char*rom[]={"","I","II","III","IV","V","VI","VII","VIII","IX","X",
            "XI","XII","XIII","XIV","XV","XVI","XVII","XVIII","XIX","XX","XXI","XXII","XXIII","XXIV"};
        auto pt = toPixel(xL - 6.0*XD, V_ROW_Y[1]);
        QFont f=p.font(); f.setPointSize(7); f.setBold(false); p.setFont(f);
        p.save(); p.translate(pt); p.rotate(-90);
        p.drawText(QRectF(-20,-7,40,14), Qt::AlignCenter, rom[qBound(1,c.startFret,24)]);
        p.restore();
    }

    p.setPen(QPen(Qt::black,1.2)); p.setBrush(Qt::NoBrush);

    // Frame
    p.drawRect(QRectF(toPixel(xL,V_FRAME_TOP), toPixel(xR,V_FRAME_BOTTOM)));

    // Inner string lines (vertical)
    for(int s=1;s<N-1;++s)
        p.drawLine(toPixel(vSX(s,N),V_FRAME_TOP), toPixel(vSX(s,N),V_FRAME_BOTTOM));

    // Fret dividers (horizontal, 5 for 5 frets)
    for(int g=0;g<5;++g)
        p.drawLine(toPixel(xL,V_GRID_Y[g]), toPixel(xR,V_GRID_Y[g]));

    // Barré
    if(c.barre.active && c.barre.fret>=1 && c.barre.fret<=ChordData::NUM_FRETS) {
        int fr=c.barre.fret;
        int sLo=qMin(c.barre.firstString,c.barre.lastString);
        int sHi=qMax(c.barre.firstString,c.barre.lastString);
        if(sLo < sHi)
            drawBarre(p, toPixel(vSX(sLo,N),vRowY(fr)),
                         toPixel(vSX(sHi,N),vRowY(fr)));
    }

    // Notes
    for(int s=0;s<N;++s)
        for(int f=0;f<=ChordData::NUM_FRETS;++f) {
            if(c.notes[s][f]==NoteState::None) continue;
            drawNote(p, toPixel(vSX(s,N),vRowY(f)), c.notes[s][f]);
        }
}
