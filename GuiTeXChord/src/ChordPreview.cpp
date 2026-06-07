#include "ChordPreview.h"
#include <QPainter>
#include <QPainterPath>

// Mirror of LatexGenerator constants
static constexpr double V_XDIST = 0.4;
static const double V_FRET_Y[] = {3.10,2.25,0.75,-0.75,-2.25,-3.75};
static const double V_GRID_Y[] = {3.0,1.5,0.0,-1.5,-3.0};
static constexpr double V_FRAME_TOP    =  3.18;
static constexpr double V_FRAME_BOTTOM = -4.50;
static constexpr double V_LABEL_Y      =  3.65;
static double vSX(int i,int N){ return (2*i-(N-1))*V_XDIST; }

static constexpr double H_YDIST   = 0.7;
static constexpr double H_FSTEP   = 1.5;
static constexpr double H_SAT1    = -0.12;
static constexpr double H_SAT2    =  0.08;
static constexpr double H_OPEN_X  = (H_SAT1+H_SAT2)/2.0;
static constexpr double H_FRET1_X =  0.75;
static constexpr double H_FRAME_LEFT  = -0.70;
static constexpr double H_FRAME_RIGHT =  6.10;
static constexpr double H_LABEL_X     = -1.20;
static double hFX(int f){ return f==0 ? H_OPEN_X : H_FRET1_X+(f-1)*H_FSTEP; }
static double hGX(int g){ return (hFX(g+1)+hFX(g+2))/2.0; }
static double hSY(int i,int N){ return (i-(N-1)/2.0)*H_YDIST; }

static void drawNote(QPainter &p, QPointF pt, NoteState st, double r=5.5){
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
        p.setPen(QPen(Qt::black,1.5)); double d=r*0.75;
        p.drawLine(pt+QPointF(-d,-d),pt+QPointF(d,d));
        p.drawLine(pt+QPointF(-d,d), pt+QPointF(d,-d)); break;}
    default: break;
    }
}

ChordPreview::ChordPreview(QWidget *p):QWidget(p){
    setMinimumSize(100,80);
    setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
}
void ChordPreview::setChord(const ChordData &c){ m_chord=c; update(); }
QPointF ChordPreview::toPixel(double lx,double ly) const {
    return {m_ox+lx*m_sx, m_oy-ly*m_sy};
}

void ChordPreview::paintEvent(QPaintEvent*){
    if(m_chord.numStrings<2) return;
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    if(m_chord.orientation==Orientation::Vertical) drawVertical(p);
    else                                            drawHorizontal(p);
}

// ---------------------------------------------------------------------------
void ChordPreview::drawVertical(QPainter &p)
{
    const ChordData &c=m_chord; const int N=c.numStrings;
    double xL=vSX(0,N), xR=vSX(N-1,N);

    // Logical bbox (constant regardless of name/startfret visibility)
    double logL=xL-0.90, logR=xR+0.20;
    double logT=V_LABEL_Y+0.35, logB=V_FRAME_BOTTOM;

    double mL=8,mR=8,mT=8,mB=8;
    double usW=width()-mL-mR, usH=height()-mT-mB;
    if(usW<1||usH<1) return;
    double sc=qMin(usW/(logR-logL), usH/(logT-logB));
    m_sx=m_sy=sc;
    double dW=(logR-logL)*sc, dH=(logT-logB)*sc;
    m_ox=mL+(usW-dW)/2.0-logL*sc;
    m_oy=mT+(usH-dH)/2.0+logT*sc;

    p.setPen(QPen(Qt::black,1.2));

    // Name
    if(c.showName&&!c.fullName().isEmpty()){
        QFont f=p.font();f.setBold(true);f.setPointSize(9);p.setFont(f);
        auto pt=toPixel((xL+xR)/2.0,V_LABEL_Y+0.28);
        p.drawText(QRectF(pt.x()-55,pt.y()-10,110,18),Qt::AlignCenter,c.fullName());
    }
    // String labels
    {QFont f=p.font();f.setBold(true);f.setPointSize(7);f.setItalic(false);p.setFont(f);
     for(int s=0;s<N;++s){
       auto pt=toPixel(vSX(s,N),V_LABEL_Y);
       p.drawText(QRectF(pt.x()-12,pt.y()-8,24,14),Qt::AlignCenter,
                  s<c.tuning.size()?c.tuning[s]:"");
     }}
    // Startbund
    if(c.showStartFret&&c.startFret>=1){
        static const char*rom[]={"","I","II","III","IV","V","VI","VII","VIII","IX","X",
            "XI","XII","XIII","XIV","XV","XVI","XVII","XVIII","XIX","XX","XXI","XXII","XXIII","XXIV"};
        auto pt=toPixel(xL-0.60,(V_FRET_Y[1]+V_FRET_Y[2])/2.0);
        QFont f=p.font();f.setPointSize(7);f.setBold(false);p.setFont(f);
        p.save();p.translate(pt);p.rotate(-90);
        p.drawText(QRectF(-20,-7,40,14),Qt::AlignCenter,rom[qBound(1,c.startFret,24)]);
        p.restore();
    }
    // Frame
    p.setPen(QPen(Qt::black,1.2));
    p.drawRect(QRectF(toPixel(xL,V_FRAME_TOP),toPixel(xR,V_FRAME_BOTTOM)));
    // Inner string lines
    for(int s=1;s<N-1;++s)
        p.drawLine(toPixel(vSX(s,N),V_FRAME_TOP),toPixel(vSX(s,N),V_FRAME_BOTTOM));
    // Fret dividers
    for(int g=0;g<5;++g)
        p.drawLine(toPixel(xL,V_GRID_Y[g]),toPixel(xR,V_GRID_Y[g]));
    // Sattel: zwei horizontale Querlinien oberhalb Frame-Top
    p.setPen(QPen(Qt::black,1.0));
    p.drawLine(toPixel(xL,3.30),toPixel(xR,3.30));
    p.setPen(QPen(Qt::black,2.5));
    p.drawLine(toPixel(xL,3.50),toPixel(xR,3.50));
    p.setPen(QPen(Qt::black,1.2));
    // Barré
    if(c.barre.active&&c.barre.fret>=1&&c.barre.fret<=ChordData::NUM_FRETS){
        int fr=c.barre.fret;
        int s1=qMin(c.barre.firstString,c.barre.lastString);
        int s2=qMax(c.barre.firstString,c.barre.lastString);
        auto pL=toPixel(vSX(s1,N),V_FRET_Y[fr]);
        auto pR=toPixel(vSX(s2,N),V_FRET_Y[fr]);
        double r=5.0;
        QPainterPath bp;
        bp.moveTo(pL.x(),pL.y()-r); bp.lineTo(pR.x(),pR.y()-r);
        bp.arcTo(pR.x()-r,pR.y()-r,2*r,2*r,90,-180);
        bp.lineTo(pL.x(),pL.y()+r);
        bp.arcTo(pL.x()-r,pL.y()-r,2*r,2*r,-90,-180);
        bp.closeSubpath();
        p.setBrush(QColor(30,30,30,210));p.setPen(Qt::NoPen);p.drawPath(bp);
        p.setPen(QPen(Qt::black,1.2));p.setBrush(Qt::NoBrush);
    }
    // Notes
    for(int s=0;s<N;++s)
        for(int f=0;f<=ChordData::NUM_FRETS;++f){
            if(c.notes[s][f]==NoteState::None) continue;
            drawNote(p,toPixel(vSX(s,N),V_FRET_Y[f]),c.notes[s][f]);
        }
}

// ---------------------------------------------------------------------------
void ChordPreview::drawHorizontal(QPainter &p)
{
    const ChordData &c=m_chord; const int N=c.numStrings;
    double yTop=hSY(N-1,N), yBot=hSY(0,N);

    double logL=H_LABEL_X-0.15, logR=H_FRAME_RIGHT+0.15;
    double logT=yTop+0.85,       logB=yBot;

    double mL=8,mR=8,mT=8,mB=8;
    double usW=width()-mL-mR, usH=height()-mT-mB;
    if(usW<1||usH<1) return;
    double sc=qMin(usW/(logR-logL), usH/(logT-logB));
    m_sx=m_sy=sc;
    double dW=(logR-logL)*sc, dH=(logT-logB)*sc;
    m_ox=mL+(usW-dW)/2.0-logL*sc;
    m_oy=mT+(usH-dH)/2.0+logT*sc;

    p.setPen(QPen(Qt::black,1.2));
    // Name
    if(c.showName&&!c.fullName().isEmpty()){
        QFont f=p.font();f.setBold(true);f.setPointSize(9);p.setFont(f);
        auto pt=toPixel((H_FRAME_LEFT+H_FRAME_RIGHT)/2.0,yTop+0.72);
        p.drawText(QRectF(pt.x()-55,pt.y()-10,110,18),Qt::AlignCenter,c.fullName());
    }
    // Startbund
    if(c.showStartFret&&c.startFret>=1){
        static const char*rom[]={"","I","II","III","IV","V","VI","VII","VIII","IX","X",
            "XI","XII","XIII","XIV","XV","XVI","XVII","XVIII","XIX","XX","XXI","XXII","XXIII","XXIV"};
        auto pt=toPixel(hFX(1),yTop+0.35);
        QFont f=p.font();f.setPointSize(7);f.setBold(false);p.setFont(f);
        p.drawText(QRectF(pt.x()-2,pt.y()-8,40,14),Qt::AlignLeft|Qt::AlignVCenter,
                   rom[qBound(1,c.startFret,24)]);
    }
    // String labels
    {QFont f=p.font();f.setBold(true);f.setPointSize(7);p.setFont(f);
     for(int s=0;s<N;++s){
       auto pt=toPixel(H_LABEL_X,hSY(s,N));
       p.drawText(QRectF(pt.x()-22,pt.y()-8,22,16),Qt::AlignRight|Qt::AlignVCenter,
                  s<c.tuning.size()?c.tuning[s]:"");
     }}
    // Frame
    p.setPen(QPen(Qt::black,1.2));
    p.drawRect(QRectF(toPixel(H_FRAME_LEFT,yTop),toPixel(H_FRAME_RIGHT,yBot)));
    // Inner string lines
    for(int s=1;s<N-1;++s)
        p.drawLine(toPixel(H_FRAME_LEFT,hSY(s,N)),toPixel(H_FRAME_RIGHT,hSY(s,N)));
    // Fret dividers (4 lines for 5 frets)
    for(int g=0;g<4;++g)
        p.drawLine(toPixel(hGX(g),yTop),toPixel(hGX(g),yBot));
    // Sattel: nur zwei dichte vertikale Linien
    p.setPen(QPen(Qt::black,1.0));
    p.drawLine(toPixel(H_SAT1,yTop),toPixel(H_SAT1,yBot));
    p.setPen(QPen(Qt::black,2.5));
    p.drawLine(toPixel(H_SAT2,yTop),toPixel(H_SAT2,yBot));
    p.setPen(QPen(Qt::black,1.2));
    // Barré
    if(c.barre.active&&c.barre.fret>=1&&c.barre.fret<=ChordData::NUM_FRETS){
        int fr=c.barre.fret;
        int s1=qMin(c.barre.firstString,c.barre.lastString);
        int s2=qMax(c.barre.firstString,c.barre.lastString);
        double bx=hFX(fr);
        auto pB=toPixel(bx,hSY(s1,N)), pT=toPixel(bx,hSY(s2,N));
        double r=5.0;
        QPainterPath bp;
        bp.moveTo(pT.x()-r,pT.y());
        bp.arcTo(pT.x()-r,pT.y()-r,2*r,2*r,180,-180);
        bp.lineTo(pB.x()+r,pB.y());
        bp.arcTo(pB.x()-r,pB.y()-r,2*r,2*r,0,-180);
        bp.closeSubpath();
        p.setBrush(QColor(30,30,30,210));p.setPen(Qt::NoPen);p.drawPath(bp);
        p.setPen(QPen(Qt::black,1.2));p.setBrush(Qt::NoBrush);
    }
    // Notes
    for(int s=0;s<N;++s)
        for(int f=0;f<=ChordData::NUM_FRETS;++f){
            if(c.notes[s][f]==NoteState::None) continue;
            drawNote(p,toPixel(hFX(f),hSY(s,N)),c.notes[s][f]);
        }
}
