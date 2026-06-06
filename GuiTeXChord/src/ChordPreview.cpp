#include "ChordPreview.h"
#include <QPainter>
#include <QPainterPath>

// Shared with LatexGenerator (same numbers)
// --- Vertical ---
static constexpr double V_XDIST = 0.4;
static const double V_FRET_Y[] = {3.10,2.25,0.75,-0.75,-2.25};
static const double V_GRID_Y[] = {3.00,1.50,0.00,-1.50};
static constexpr double V_FRAME_TOP    =  3.18;
static constexpr double V_FRAME_BOTTOM = -3.00;
static constexpr double V_LABEL_Y      =  3.60;
static double vSX(int idx,int N){ return (2*idx-(N-1))*V_XDIST; }

// --- Horizontal ---
static constexpr double H_YDIST      = 0.7;
static constexpr double H_FRET_STEP  = 1.5;
static const double H_FRET_X[] = {-0.55, 0.75, 0.75+H_FRET_STEP, 0.75+2*H_FRET_STEP, 0.75+3*H_FRET_STEP};
static const double H_GRID_X[] = {(H_FRET_X[1]+H_FRET_X[2])/2.0,(H_FRET_X[2]+H_FRET_X[3])/2.0,(H_FRET_X[3]+H_FRET_X[4])/2.0};
static constexpr double H_FRAME_LEFT  = -1.0;
static constexpr double H_FRAME_RIGHT =  6.1;
static constexpr double H_LABEL_X     = -1.55;
static constexpr double H_SATTEL_X1   = -0.15;
static constexpr double H_SATTEL_X2   =  0.10;
static double hSY(int idx,int N){ return (idx-(N-1)/2.0)*H_YDIST; }
static double hFX(int f){ return (f>=0&&f<=ChordData::NUM_FRETS)?H_FRET_X[f]:H_FRET_X[ChordData::NUM_FRETS]; }

// ---------------------------------------------------------------------------
static void drawNote(QPainter &p, QPointF pt, NoteState st, double r=5.5)
{
    switch(st){
    case NoteState::Tone:
        p.setPen(QPen(Qt::black,1)); p.setBrush(Qt::black);
        p.drawEllipse(pt,r,r); break;
    case NoteState::Root:
        p.setPen(QPen(Qt::black,2)); p.setBrush(Qt::white);
        p.drawEllipse(pt,r,r);
        p.setPen(Qt::NoPen); p.setBrush(Qt::black);
        p.drawEllipse(pt,r*0.38,r*0.38);
        p.setPen(QPen(Qt::black,1.5)); p.setBrush(Qt::NoBrush); break;
    case NoteState::Mute:{
        p.setPen(QPen(Qt::black,1.5));
        double d=r*0.75;
        p.drawLine(pt+QPointF(-d,-d),pt+QPointF(d,d));
        p.drawLine(pt+QPointF(-d,d), pt+QPointF(d,-d)); break;}
    default: break;
    }
}

// ---------------------------------------------------------------------------
ChordPreview::ChordPreview(QWidget *parent):QWidget(parent){
    setMinimumSize(120,80);
    setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
}
void ChordPreview::setChord(const ChordData &chord){ m_chord=chord; update(); }

QPointF ChordPreview::toPixel(double lx,double ly) const {
    return {m_ox+lx*m_sx, m_oy-ly*m_sy};
}

void ChordPreview::paintEvent(QPaintEvent*)
{
    const ChordData &c=m_chord;
    if(c.numStrings<2) return;
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    if(c.orientation==Orientation::Vertical)  drawVertical(p);
    else                                       drawHorizontal(p);
}

// ---------------------------------------------------------------------------
void ChordPreview::drawVertical(QPainter &p)
{
    const ChordData &c=m_chord;
    const int N=c.numStrings;
    double xL=vSX(0,N), xR=vSX(N-1,N);

    // logical extent (constant bbox)
    double logLeft  = xL - 0.90;   // sattel + label space
    double logRight = xR + 0.20;
    double logTop   = V_LABEL_Y + 0.30;  // name reserve
    double logBot   = V_FRAME_BOTTOM;

    double mL=36,mR=8,mT=32,mB=8;
    double usW=width()-mL-mR, usH=height()-mT-mB;
    if(usW<1||usH<1) return;

    // Aspect-correct fit
    double scaleX=(usW)/(logRight-logLeft);
    double scaleY=(usH)/(logTop-logBot);
    double sc=qMin(scaleX,scaleY);
    m_sx=m_sy=sc;

    double drawW=(logRight-logLeft)*sc, drawH=(logTop-logBot)*sc;
    m_ox = mL + (usW-drawW)/2.0 - logLeft*sc;
    m_oy = mT + (usH-drawH)/2.0 + logTop*sc;

    p.setPen(QPen(Qt::black,1.2));

    // Name
    if(c.showName&&!c.fullName().isEmpty()){
        QFont f=p.font(); f.setBold(true); f.setPointSize(9); p.setFont(f);
        auto pt=toPixel((xL+xR)/2.0, V_LABEL_Y+0.20);
        p.drawText(QRectF(pt.x()-50,pt.y()-10,100,16),Qt::AlignCenter,c.fullName());
    }

    // String labels
    { QFont f=p.font(); f.setBold(true); f.setPointSize(7); f.setItalic(false); p.setFont(f);
      for(int s=0;s<N;++s){
        auto pt=toPixel(vSX(s,N),V_LABEL_Y);
        QString lbl=s<c.tuning.size()?c.tuning[s]:"";
        p.drawText(QRectF(pt.x()-12,pt.y()-8,24,14),Qt::AlignCenter,lbl);
      }
    }

    // Start fret
    if(c.showStartFret&&c.startFret>=1){
        static const char* rom[]={"","I","II","III","IV","V","VI","VII","VIII","IX","X",
            "XI","XII","XIII","XIV","XV","XVI","XVII","XVIII","XIX","XX","XXI","XXII","XXIII","XXIV"};
        double sfY=(V_FRET_Y[1]+V_FRET_Y[2])/2.0;
        double sfX=xL-0.60;
        auto pt=toPixel(sfX,sfY);
        QFont f=p.font(); f.setPointSize(7); f.setBold(false); p.setFont(f);
        p.save(); p.translate(pt); p.rotate(-90);
        p.drawText(QRectF(-20,-7,40,14),Qt::AlignCenter,rom[qBound(1,c.startFret,24)]);
        p.restore();
    }

    // Frame
    auto ul=toPixel(xL,V_FRAME_TOP), ur=toPixel(xR,V_FRAME_TOP);
    auto bl=toPixel(xL,V_FRAME_BOTTOM),br=toPixel(xR,V_FRAME_BOTTOM);
    p.drawRect(QRectF(ul,br));

    // Inner string lines
    for(int s=1;s<N-1;++s)
        p.drawLine(toPixel(vSX(s,N),V_FRAME_TOP),toPixel(vSX(s,N),V_FRAME_BOTTOM));

    // Fret dividers
    for(int g=0;g<4;++g)
        p.drawLine(toPixel(xL,V_GRID_Y[g]),toPixel(xR,V_GRID_Y[g]));

    // Sattel
    double sx1=xL-0.22, sx2=xL-0.08;
    p.setPen(QPen(Qt::black,1.0)); p.drawLine(toPixel(sx1,V_FRAME_TOP),toPixel(sx1,V_FRAME_BOTTOM));
    p.setPen(QPen(Qt::black,2.5)); p.drawLine(toPixel(sx2,V_FRAME_TOP),toPixel(sx2,V_FRAME_BOTTOM));
    p.setPen(QPen(Qt::black,1.2));

    // Barré
    if(c.barre.active&&c.barre.fret>=1&&c.barre.fret<=ChordData::NUM_FRETS){
        int fr=c.barre.fret;
        int sL2=qMin(c.barre.firstString,c.barre.lastString);
        int sR2=qMax(c.barre.firstString,c.barre.lastString);
        auto ptL=toPixel(vSX(sL2,N),V_FRET_Y[fr]);
        auto ptR=toPixel(vSX(sR2,N),V_FRET_Y[fr]);
        double r=5.0;
        QPainterPath bp;
        bp.moveTo(ptL.x(),ptL.y()-r);
        bp.lineTo(ptR.x(),ptR.y()-r);
        bp.arcTo(ptR.x()-r,ptR.y()-r,2*r,2*r,90,-180);
        bp.lineTo(ptL.x(),ptL.y()+r);
        bp.arcTo(ptL.x()-r,ptL.y()-r,2*r,2*r,-90,-180);
        bp.closeSubpath();
        p.setBrush(QColor(30,30,30,210)); p.setPen(Qt::NoPen); p.drawPath(bp);
        p.setPen(QPen(Qt::black,1.2)); p.setBrush(Qt::NoBrush);
    }

    // Notes
    for(int s=0;s<N;++s)
        for(int f=0;f<=ChordData::NUM_FRETS;++f){
            NoteState st=c.notes[s][f];
            if(st==NoteState::None) continue;
            drawNote(p,toPixel(vSX(s,N),V_FRET_Y[f]),st);
        }
}

// ---------------------------------------------------------------------------
void ChordPreview::drawHorizontal(QPainter &p)
{
    const ChordData &c=m_chord;
    const int N=c.numStrings;
    double yTop=hSY(N-1,N), yBot=hSY(0,N);

    double logLeft  = H_LABEL_X - 0.20;
    double logRight = H_FRAME_RIGHT + 0.20;
    double logTop   = yTop + 0.85;   // name + startfret reserve
    double logBot   = yBot;

    double mL=8,mR=8,mT=8,mB=8;
    double usW=width()-mL-mR, usH=height()-mT-mB;
    if(usW<1||usH<1) return;

    double scaleX=(usW)/(logRight-logLeft);
    double scaleY=(usH)/(logTop-logBot);
    double sc=qMin(scaleX,scaleY);
    m_sx=m_sy=sc;

    double drawW=(logRight-logLeft)*sc, drawH=(logTop-logBot)*sc;
    m_ox = mL + (usW-drawW)/2.0 - logLeft*sc;
    m_oy = mT + (usH-drawH)/2.0 + logTop*sc;

    p.setPen(QPen(Qt::black,1.2));

    // Name
    if(c.showName&&!c.fullName().isEmpty()){
        QFont f=p.font(); f.setBold(true); f.setPointSize(9); p.setFont(f);
        auto pt=toPixel((H_FRAME_LEFT+H_FRAME_RIGHT)/2.0, yTop+0.72);
        p.drawText(QRectF(pt.x()-50,pt.y()-10,100,16),Qt::AlignCenter,c.fullName());
    }

    // Start fret above fret-1
    if(c.showStartFret&&c.startFret>=1){
        static const char* rom[]={"","I","II","III","IV","V","VI","VII","VIII","IX","X",
            "XI","XII","XIII","XIV","XV","XVI","XVII","XVIII","XIX","XX","XXI","XXII","XXIII","XXIV"};
        auto pt=toPixel(H_FRET_X[1], yTop+0.35);
        QFont f=p.font(); f.setPointSize(7); f.setBold(false); p.setFont(f);
        p.drawText(QRectF(pt.x()-2,pt.y()-8,40,14),Qt::AlignLeft|Qt::AlignVCenter,
                   rom[qBound(1,c.startFret,24)]);
    }

    // String labels
    { QFont f=p.font(); f.setBold(true); f.setPointSize(7); p.setFont(f);
      for(int s=0;s<N;++s){
        auto pt=toPixel(H_LABEL_X, hSY(s,N));
        QString lbl=s<c.tuning.size()?c.tuning[s]:"";
        p.drawText(QRectF(pt.x()-22,pt.y()-8,22,16),Qt::AlignRight|Qt::AlignVCenter,lbl);
      }
    }

    // Frame
    p.drawRect(QRectF(toPixel(H_FRAME_LEFT,yTop),toPixel(H_FRAME_RIGHT,yBot)));

    // Inner string lines
    for(int s=1;s<N-1;++s)
        p.drawLine(toPixel(H_FRAME_LEFT,hSY(s,N)),toPixel(H_FRAME_RIGHT,hSY(s,N)));

    // Fret dividers
    for(int g=0;g<3;++g)
        p.drawLine(toPixel(H_GRID_X[g],yTop),toPixel(H_GRID_X[g],yBot));

    // Sattel
    p.setPen(QPen(Qt::black,1.0)); p.drawLine(toPixel(H_SATTEL_X1,yTop),toPixel(H_SATTEL_X1,yBot));
    p.setPen(QPen(Qt::black,2.5)); p.drawLine(toPixel(H_SATTEL_X2,yTop),toPixel(H_SATTEL_X2,yBot));
    p.setPen(QPen(Qt::black,1.2));

    // Barré (vertical thick line)
    if(c.barre.active&&c.barre.fret>=1&&c.barre.fret<=ChordData::NUM_FRETS){
        int fr=c.barre.fret;
        int sL2=qMin(c.barre.firstString,c.barre.lastString);
        int sR2=qMax(c.barre.firstString,c.barre.lastString);
        double bx=hFX(fr);
        double by1=hSY(sL2,N), by2=hSY(sR2,N);
        double r=5.0;
        QPainterPath bp;
        bp.moveTo(toPixel(bx,by2).x()-r, toPixel(bx,by2).y());
        bp.arcTo(toPixel(bx,by2).x()-r, toPixel(bx,by2).y()-r, 2*r,2*r, 180,-180);
        bp.lineTo(toPixel(bx,by1).x()+r, toPixel(bx,by1).y());
        bp.arcTo(toPixel(bx,by1).x()-r,  toPixel(bx,by1).y()-r, 2*r,2*r, 0,-180);
        bp.closeSubpath();
        p.setBrush(QColor(30,30,30,210)); p.setPen(Qt::NoPen); p.drawPath(bp);
        p.setPen(QPen(Qt::black,1.2)); p.setBrush(Qt::NoBrush);
    }

    // Notes
    for(int s=0;s<N;++s)
        for(int f=0;f<=ChordData::NUM_FRETS;++f){
            NoteState st=c.notes[s][f];
            if(st==NoteState::None) continue;
            drawNote(p,toPixel(hFX(f),hSY(s,N)),st);
        }
}
