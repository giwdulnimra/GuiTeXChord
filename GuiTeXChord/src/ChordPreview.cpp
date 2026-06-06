#include "ChordPreview.h"
#include <QPainter>
#include <QPainterPath>

// ---------------------------------------------------------------------------
// Koordinaten spiegeln LatexGenerator exakt:
//   x = Bünde links→rechts, Sattel als zwei Linien
//   y = Saiten: stringIdx 0 (E) unten, stringIdx N-1 (e') oben
// ---------------------------------------------------------------------------

static constexpr double YDIST      = 0.7;
static constexpr double FRET_STEP  = 1.5;
static constexpr double SATTEL_X1  = -0.15;
static constexpr double SATTEL_X2  =  0.10;
static constexpr double FRAME_LEFT = -0.85;
static constexpr double FRAME_RIGHT=  6.10;
static constexpr double LABEL_X    = -1.30;
static constexpr double NAME_RESERVE      = 0.7;
static constexpr double STARTFRET_RESERVE = 0.5;

static const double FRET_X[] = {
    -0.55,
     0.75,
     0.75 + 1*FRET_STEP,
     0.75 + 2*FRET_STEP,
     0.75 + 3*FRET_STEP
};
static const double GRID_X[] = {
    (FRET_X[1]+FRET_X[2])/2.0,
    (FRET_X[2]+FRET_X[3])/2.0,
    (FRET_X[3]+FRET_X[4])/2.0
};

static double logStringY(int idx, int n) {
    // idx 0 = tiefste (E) = unten = kleinster y
    return (idx - (n - 1) / 2.0) * YDIST;
}
static double lfretX(int f) {
    if (f >= 0 && f <= ChordData::NUM_FRETS) return FRET_X[f];
    return FRET_X[ChordData::NUM_FRETS];
}

// ---------------------------------------------------------------------------
ChordPreview::ChordPreview(QWidget *parent) : QWidget(parent) {
    setMinimumSize(220, 110);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
}

void ChordPreview::setChord(const ChordData &chord) {
    m_chord = chord;
    update();
}

QPointF ChordPreview::toPixel(double lx, double ly) const {
    // logisch y oben = großes y → Widget y oben = kleines y  →  umkehren
    return { m_originX + lx * m_scaleX,
             m_originY - ly * m_scaleY };
}

void ChordPreview::paintEvent(QPaintEvent *)
{
    const ChordData &c = m_chord;
    if (c.numStrings < 2) return;
    const int N = c.numStrings;

    double yBottom = logStringY(0,   N);   // E, unten
    double yTop    = logStringY(N-1, N);   // e', oben

    // Konstante logische Ausdehnung (Bounding-Box immer gleich)
    double logYTop    = yTop    + NAME_RESERVE;
    double logYBottom = yBottom;
    double logW = FRAME_RIGHT - FRAME_LEFT;
    double logH = logYTop - logYBottom;

    // Feste Margins: oben = Name + Startbund, links = Saiten-Labels
    double topMargin    = 38.0; // Platz für Name + Startbund, immer gleich
    double bottomMargin = 8.0;
    double leftMargin   = 30.0;
    double rightMargin  = 8.0;

    double usableW = width()  - leftMargin - rightMargin;
    double usableH = height() - topMargin  - bottomMargin;
    if (usableW < 1 || usableH < 1) return;

    m_scaleX = usableW / logW;
    m_scaleY = usableH / logH;

    // Ursprung: logisch (FRAME_LEFT, logYTop) → Pixel (leftMargin, topMargin)
    m_originX = leftMargin  - FRAME_LEFT * m_scaleX;
    m_originY = topMargin   + logYTop    * m_scaleY;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // --- Akkord-Name (immer gezeichnet, leer wenn nicht aktiv → Box konstant) ---
    {
        QFont f = p.font(); f.setBold(true); f.setPointSize(10); p.setFont(f);
        p.setPen(Qt::black);
        QString name = (c.showName && !c.fullName().isEmpty()) ? c.fullName() : "";
        double cx = (FRAME_LEFT + FRAME_RIGHT) / 2.0;
        auto pt = toPixel(cx, yTop + NAME_RESERVE);
        p.drawText(QRectF(pt.x()-60, pt.y()-12, 120, 20),
                   Qt::AlignHCenter | Qt::AlignVCenter, name);
    }

    // --- Startbund oberhalb erstem Bund, linksbündig ---
    {
        static const char *roman[] = {
            "","I","II","III","IV","V","VI","VII","VIII","IX","X",
            "XI","XII","XIII","XIV","XV","XVI","XVII","XVIII","XIX","XX",
            "XXI","XXII","XXIII","XXIV"
        };
        QFont f = p.font(); f.setBold(false); f.setPointSize(8); p.setFont(f);
        p.setPen(Qt::black);
        QString sfStr = (c.showStartFret && c.startFret > 1)
                        ? roman[qBound(1, c.startFret, 24)] : "";
        auto pt = toPixel(FRET_X[1], yTop + STARTFRET_RESERVE * 0.55);
        p.drawText(QRectF(pt.x()-2, pt.y()-8, 40, 14),
                   Qt::AlignLeft | Qt::AlignVCenter, sfStr);
    }

    // --- Saiten-Labels (E unten, e' oben) ---
    {
        QFont f = p.font(); f.setBold(true); f.setPointSize(8); p.setFont(f);
        p.setPen(Qt::black);
        for (int s = 0; s < N; ++s) {
            double y = logStringY(s, N);
            QString lbl = (s < c.tuning.size()) ? c.tuning[s] : "";
            auto pt = toPixel(LABEL_X, y);
            p.drawText(QRectF(pt.x()-22, pt.y()-8, 22, 16),
                       Qt::AlignRight | Qt::AlignVCenter, lbl);
        }
    }

    p.setPen(QPen(Qt::black, 1.2));
    p.setBrush(Qt::NoBrush);

    // --- Rahmen ---
    auto ul = toPixel(FRAME_LEFT,  yTop);
    auto br = toPixel(FRAME_RIGHT, yBottom);
    p.drawRect(QRectF(ul, br));

    // --- Saitenlinien innen ---
    for (int s = 1; s < N-1; ++s) {
        double y = logStringY(s, N);
        p.drawLine(toPixel(FRAME_LEFT, y), toPixel(FRAME_RIGHT, y));
    }

    // --- Bundstäbe (3 Linien zwischen Bünden 1-4) ---
    for (int g = 0; g < 3; ++g)
        p.drawLine(toPixel(GRID_X[g], yTop), toPixel(GRID_X[g], yBottom));

    // --- Sattel: zwei dichte vertikale Linien ---
    p.setPen(QPen(Qt::black, 1.2));
    p.drawLine(toPixel(SATTEL_X1, yTop), toPixel(SATTEL_X1, yBottom));
    p.setPen(QPen(Qt::black, 2.5));
    p.drawLine(toPixel(SATTEL_X2, yTop), toPixel(SATTEL_X2, yBottom));
    p.setPen(QPen(Qt::black, 1.2));

    // --- Barré ---
    if (c.barre.active && c.barre.fret >= 1 && c.barre.fret <= ChordData::NUM_FRETS) {
        int fr  = c.barre.fret;
        int sLo = qMin(c.barre.firstString, c.barre.lastString);
        int sHi = qMax(c.barre.firstString, c.barre.lastString);
        if (sLo < sHi) {
            double x   = lfretX(fr);
            double yLo = logStringY(sLo, N);
            double yHi = logStringY(sHi, N);
            auto ptLo = toPixel(x, yLo);
            auto ptHi = toPixel(x, yHi);
            double r = 5.0;
            QPainterPath bp;
            // Barré läuft vertikal im Widget (y-Richtung)
            bp.moveTo(ptLo.x()-r, ptLo.y());
            bp.arcTo(ptLo.x()-r, ptLo.y()-r, 2*r, 2*r, 180, -180);
            bp.lineTo(ptHi.x()+r, ptHi.y());
            bp.arcTo(ptHi.x()-r, ptHi.y()-r, 2*r, 2*r, 0, -180);
            bp.closeSubpath();
            p.setBrush(QColor(30,30,30,210));
            p.setPen(Qt::NoPen);
            p.drawPath(bp);
            p.setPen(QPen(Qt::black, 1.2));
            p.setBrush(Qt::NoBrush);
        }
    }

    // --- Noten ---
    for (int s = 0; s < N; ++s) {
        double y = logStringY(s, N);
        for (int f = 0; f <= ChordData::NUM_FRETS; ++f) {
            NoteState st = c.notes[s][f];
            if (st == NoteState::None) continue;
            double x  = lfretX(f);
            auto   pt = toPixel(x, y);
            double r  = 5.5;
            switch (st) {
            case NoteState::Tone:
                p.setPen(QPen(Qt::black, 1.0));
                p.setBrush(Qt::black);
                p.drawEllipse(pt, r, r);
                break;
            case NoteState::Root:
                p.setPen(QPen(Qt::black, 2.0));
                p.setBrush(Qt::white);
                p.drawEllipse(pt, r, r);
                p.setPen(Qt::NoPen);
                p.setBrush(QColor(0,0,0));
                p.drawEllipse(pt, r*0.38, r*0.38);
                p.setPen(QPen(Qt::black,1.2));
                p.setBrush(Qt::NoBrush);
                break;
            case NoteState::Mute: {
                p.setPen(QPen(Qt::black, 1.5));
                double d = r*0.75;
                p.drawLine(pt+QPointF(-d,-d), pt+QPointF(d,d));
                p.drawLine(pt+QPointF(-d, d), pt+QPointF(d,-d));
                break;
            }
            default: break;
            }
        }
    }
}
