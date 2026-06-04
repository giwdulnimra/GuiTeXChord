#include "ChordPreview.h"
#include <QPainter>
#include <QPainterPath>
#include <QFontMetricsF>

// Koordinatensystem identisch zu LatexGenerator
static constexpr double XDIST = 0.4;
static const double FRET_Y[]  = { 3.10, 2.25, 0.75, -0.75, -2.25 };
static const double GRID_Y[]  = { 3.0,  1.5,  0.0,  -1.5 };
static constexpr double BOTTOM_Y = -3.0;
static constexpr double LABEL_Y  =  3.6;

static double stringX(int idx, int n) {
    return (2*idx - (n-1)) * XDIST;
}

ChordPreview::ChordPreview(QWidget *parent) : QWidget(parent) {
    setMinimumSize(120, 180);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
}

void ChordPreview::setChord(const ChordData &chord) {
    m_chord = chord;
    update();
}

QPointF ChordPreview::toPixel(double lx, double ly) const {
    // Logisch: x wächst nach rechts, y wächst nach oben
    // Widget: x nach rechts, y nach unten
    // Referenzpunkt: Mitte oben des Rasters (y=3.18)
    double px = m_marginX + (lx / XDIST + (m_chord.numStrings-1)) * m_scaleX * XDIST / (2*XDIST) * 2;
    // Einfacher: direkte Skalierung
    double originX = width()  / 2.0;
    double originY = m_marginY + (3.18 - LABEL_Y) * (-m_scaleY); // Label-Zeile oben

    px = originX + lx * m_scaleX;
    double py = originY + (LABEL_Y - ly) * m_scaleY;
    return { px, py };
}

void ChordPreview::paintEvent(QPaintEvent *)
{
    const ChordData &c = m_chord;
    if (c.numStrings < 2) return;

    // Dynamisch skalieren auf Widget-Größe
    double usableW = width()  - 2*m_marginX;
    double usableH = height() - 2*m_marginY - 20; // 20px oben für Akkordname
    double totalLogW = (c.numStrings - 1) * (2 * XDIST); // logische Breite
    double totalLogH = LABEL_Y - BOTTOM_Y;
    m_scaleX = usableW / totalLogW;
    m_scaleY = usableH / totalLogH;
    // Gleichmäßige Skalierung wäre m_scaleX = m_scaleY = min(...),
    // aber Grifftabellen sind bewusst gestreckt – daher getrennt.

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // --- Akkord-Name ---
    if (c.showName && !c.fullName().isEmpty()) {
        QFont f = p.font();
        f.setBold(true);
        f.setPointSize(10);
        p.setFont(f);
        p.drawText(rect().adjusted(0,2,0,-height()+20),
                   Qt::AlignHCenter | Qt::AlignTop, c.fullName());
    }

    p.setPen(QPen(Qt::black, 1.5));
    QFont labelFont = p.font();
    labelFont.setBold(false);
    labelFont.setPointSize(7);
    p.setFont(labelFont);

    // --- Saiten-Labels ---
    for (int s = 0; s < c.numStrings; ++s) {
        auto pt = toPixel(stringX(s, c.numStrings), LABEL_Y);
        QString lbl = (s < c.tuning.size()) ? c.tuning[s] : "";
        QRectF r(pt.x()-12, pt.y()-8, 24, 14);
        p.drawText(r, Qt::AlignCenter, lbl);
    }

    // --- Startbund ---
    if (c.showStartFret && c.startFret > 1) {
        static const char *roman[] = {"","I","II","III","IV","V","VI","VII","VIII","IX","X",
                                       "XI","XII","XIII","XIV","XV","XVI","XVII","XVIII","XIX","XX",
                                       "XXI","XXII","XXIII","XXIV"};
        int n = qBound(1, c.startFret, 24);
        auto mid = toPixel(stringX(0, c.numStrings) - 2*XDIST,
                           (FRET_Y[0] + FRET_Y[1]) / 2.0);
        p.save();
        p.translate(mid);
        p.rotate(-90);
        p.drawText(QRectF(-20,-8,40,14), Qt::AlignCenter, roman[n]);
        p.restore();
    }

    // --- Rahmen ---
    auto ul = toPixel(stringX(0,       c.numStrings), 3.18);
    auto ur = toPixel(stringX(c.numStrings-1, c.numStrings), 3.18);
    auto bl = toPixel(stringX(0,       c.numStrings), BOTTOM_Y);
    auto br = toPixel(stringX(c.numStrings-1, c.numStrings), BOTTOM_Y);
    p.setPen(QPen(Qt::black, 1.5));
    p.drawRect(QRectF(ul, br));

    // --- Innere Saitenlinien ---
    for (int s = 1; s < c.numStrings - 1; ++s) {
        auto top = toPixel(stringX(s, c.numStrings), 3.18);
        auto bot = toPixel(stringX(s, c.numStrings), BOTTOM_Y);
        p.drawLine(top, bot);
    }

    // --- Horizontale Bundlinien ---
    for (int g = 0; g < 4; ++g) {
        auto left  = toPixel(stringX(0, c.numStrings), GRID_Y[g]);
        auto right = toPixel(stringX(c.numStrings-1, c.numStrings), GRID_Y[g]);
        p.drawLine(left, right);
    }

    // --- Barré ---
    if (c.barre.active && c.barre.fret >= 1 && c.barre.fret <= ChordData::NUM_FRETS) {
        int fr = c.barre.fret;
        auto ptL = toPixel(stringX(c.barre.firstString, c.numStrings), FRET_Y[fr]);
        auto ptR = toPixel(stringX(c.barre.lastString,  c.numStrings), FRET_Y[fr]);
        double r = 5.0;
        QPainterPath barPath;
        barPath.moveTo(ptL.x(), ptL.y() - r);
        barPath.lineTo(ptR.x(), ptR.y() - r);
        barPath.arcTo(ptR.x()-r, ptR.y()-r, 2*r, 2*r, 90, -180);
        barPath.lineTo(ptL.x(), ptL.y() + r);
        barPath.arcTo(ptL.x()-r, ptL.y()-r, 2*r, 2*r, -90, -180);
        barPath.closeSubpath();
        p.setBrush(QColor(30,30,30,210));
        p.setPen(Qt::NoPen);
        p.drawPath(barPath);
        p.setPen(QPen(Qt::black, 1.5));
        p.setBrush(Qt::NoBrush);
    }

    // --- Noten ---
    for (int s = 0; s < c.numStrings; ++s) {
        for (int f = 0; f <= ChordData::NUM_FRETS; ++f) {
            NoteState st = c.notes[s][f];
            if (st == NoteState::None) continue;
            auto pt = toPixel(stringX(s, c.numStrings), FRET_Y[f]);
            double r = 5.5;

            switch (st) {
            case NoteState::Root:
                p.setPen(QPen(Qt::black, 1.5));
                p.setBrush(Qt::white);
                p.drawEllipse(pt, r, r);
                break;
            case NoteState::Tone:
                p.setPen(QPen(Qt::black, 1.0));
                p.setBrush(Qt::black);
                p.drawEllipse(pt, r, r);
                break;
            case NoteState::Mute: {
                // X zeichnen
                p.setPen(QPen(Qt::black, 1.5));
                p.drawLine(pt + QPointF(-r, -r), pt + QPointF( r,  r));
                p.drawLine(pt + QPointF(-r,  r), pt + QPointF( r, -r));
                break;
            }
            default: break;
            }
        }
    }
}
