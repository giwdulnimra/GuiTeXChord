#include "ChordPreview.h"
#include <QPainter>
#include <QPainterPath>

// ---------------------------------------------------------------------------
// Koordinatensystem identisch zu LatexGenerator (gedreht):
//   x  = Bünde  (links→rechts): Bund 0 links abgesetzt, Bünde 1-4 gleichmäßig
//   y  = Saiten (oben→unten):   stringIdx 0 (e1) oben, stringIdx N-1 (E6) unten
// ---------------------------------------------------------------------------

static constexpr double YDIST = 0.7;

static const double FRET_X[]  = { -0.5, 0.75, 2.25, 3.75, 5.25 };
static const double GRID_X[]  = {  0.0, 1.5,  3.0,  4.5 };
static constexpr double FRAME_LEFT   = -1.0;
static constexpr double FRAME_RIGHT  =  6.0;
static constexpr double LABEL_X      = -1.6;
static constexpr double NAME_Y_OFFSET =  0.7;

static double logStringY(int idx, int n) {
    return ((n - 1) / 2.0 - idx) * YDIST;
}

static double fretX(int fretRow) {
    if (fretRow >= 0 && fretRow <= ChordData::NUM_FRETS) return FRET_X[fretRow];
    return FRET_X[ChordData::NUM_FRETS];
}

// ---------------------------------------------------------------------------
ChordPreview::ChordPreview(QWidget *parent) : QWidget(parent) {
    setMinimumSize(200, 120);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
}

void ChordPreview::setChord(const ChordData &chord) {
    m_chord = chord;
    update();
}

// Logische Koordinate → Widget-Pixel
// Logisch: x wächst nach rechts, y wächst nach oben (TikZ-Konvention)
// Widget:  x nach rechts, y nach unten
QPointF ChordPreview::toPixel(double lx, double ly) const
{
    double px = m_originX + lx * m_scaleX;
    double py = m_originY - ly * m_scaleY;   // y umkehren
    return { px, py };
}

void ChordPreview::paintEvent(QPaintEvent *)
{
    const ChordData &c = m_chord;
    if (c.numStrings < 2) return;

    const int N = c.numStrings;
    double yTop    = logStringY(0,   N);
    double yBottom = logStringY(N-1, N);

    // --- Skalierung dynamisch auf Widget-Größe ---
    // Logische Ausdehnung:
    double logW = FRAME_RIGHT - FRAME_LEFT;
    double logH = yTop - yBottom;

    // Platz für Name oben und Startbund unten
    double topMargin    = c.showName && !c.fullName().isEmpty() ? 26.0 : 10.0;
    double bottomMargin = c.showStartFret ? 22.0 : 10.0;
    double leftMargin   = 28.0;  // Saiten-Labels
    double rightMargin  = 10.0;

    double usableW = width()  - leftMargin  - rightMargin;
    double usableH = height() - topMargin   - bottomMargin;

    m_scaleX = usableW / logW;
    m_scaleY = usableH / logH;

    // Ursprung: logischer Punkt (FRAME_LEFT, yTop) → Pixel (leftMargin, topMargin)
    m_originX = leftMargin  - FRAME_LEFT * m_scaleX;
    m_originY = topMargin   + yTop       * m_scaleY;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // --- Akkord-Name ---
    if (c.showName && !c.fullName().isEmpty()) {
        QFont f = p.font(); f.setBold(true); f.setPointSize(10); p.setFont(f);
        double cx = (FRAME_LEFT + FRAME_RIGHT) / 2.0;
        auto pt = toPixel(cx, yTop + NAME_Y_OFFSET);
        p.drawText(QRectF(pt.x() - 60, pt.y() - 10, 120, 18),
                   Qt::AlignHCenter | Qt::AlignVCenter, c.fullName());
    }

    // --- Saiten-Labels ---
    {
        QFont f = p.font(); f.setBold(true); f.setPointSize(8); p.setFont(f);
        p.setPen(Qt::black);
        for (int s = 0; s < N; ++s) {
            double y = logStringY(s, N);
            auto pt = toPixel(LABEL_X, y);
            QString lbl = (s < c.tuning.size()) ? c.tuning[s] : "";
            p.drawText(QRectF(pt.x() - 20, pt.y() - 8, 20, 16),
                       Qt::AlignRight | Qt::AlignVCenter, lbl);
        }
    }

    // --- Startbund unter Rahmen ---
    if (c.showStartFret && c.startFret >= 1) {
        static const char *roman[] = {
            "","I","II","III","IV","V","VI","VII","VIII","IX","X",
            "XI","XII","XIII","XIV","XV","XVI","XVII","XVIII","XIX","XX",
            "XXI","XXII","XXIII","XXIV"
        };
        int n = qBound(1, c.startFret, 24);
        double cx = (FRET_X[1] + FRET_X[ChordData::NUM_FRETS]) / 2.0;
        auto pt = toPixel(cx, yBottom - 0.5);
        QFont f = p.font(); f.setBold(false); f.setPointSize(8); p.setFont(f);
        p.setPen(Qt::black);
        p.drawText(QRectF(pt.x() - 30, pt.y() - 8, 60, 16),
                   Qt::AlignHCenter | Qt::AlignVCenter, roman[n]);
    }

    // --- Rahmen ---
    p.setPen(QPen(Qt::black, 1.5));
    p.setBrush(Qt::NoBrush);
    auto ul = toPixel(FRAME_LEFT,  yTop);
    auto br = toPixel(FRAME_RIGHT, yBottom);
    p.drawRect(QRectF(ul, br));

    // --- Horizontale Saitenlinien (innen) ---
    for (int s = 1; s < N - 1; ++s) {
        double y = logStringY(s, N);
        p.drawLine(toPixel(FRAME_LEFT, y), toPixel(FRAME_RIGHT, y));
    }

    // --- Vertikale Bundstäbe ---
    for (int g = 0; g < 4; ++g) {
        p.drawLine(toPixel(GRID_X[g], yTop), toPixel(GRID_X[g], yBottom));
    }

    // --- Trennlinie Bund 0 / Bund 1 (Sattel-ähnlich, etwas dicker) ---
    p.setPen(QPen(Qt::black, 3.0));
    p.drawLine(toPixel(GRID_X[0], yTop), toPixel(GRID_X[0], yBottom));

    p.setPen(QPen(Qt::black, 1.5));

    // --- Barré ---
    if (c.barre.active && c.barre.fret >= 1 && c.barre.fret <= ChordData::NUM_FRETS) {
        int fr = c.barre.fret;
        int fs = c.barre.firstString;
        int ls = c.barre.lastString;
        if (fs < ls) {
            double x  = fretX(fr);
            double yS = logStringY(fs, N);  // oben (höhere Saite = kleinerer Index = größeres y)
            double yE = logStringY(ls, N);  // unten
            auto ptTop = toPixel(x, yS);
            auto ptBot = toPixel(x, yE);
            double r = 5.0;
            QPainterPath barPath;
            barPath.moveTo(ptTop.x() - r, ptTop.y());
            barPath.arcTo(ptTop.x() - r, ptTop.y() - r, 2*r, 2*r, 180, 180);
            barPath.lineTo(ptBot.x() + r, ptBot.y());
            barPath.arcTo(ptBot.x() - r, ptBot.y() - r, 2*r, 2*r, 0, 180);
            barPath.closeSubpath();
            p.setBrush(QColor(30, 30, 30, 210));
            p.setPen(Qt::NoPen);
            p.drawPath(barPath);
            p.setPen(QPen(Qt::black, 1.5));
            p.setBrush(Qt::NoBrush);
        }
    }

    // --- Noten ---
    for (int s = 0; s < N; ++s) {
        double y = logStringY(s, N);
        for (int f = 0; f <= ChordData::NUM_FRETS; ++f) {
            NoteState st = c.notes[s][f];
            if (st == NoteState::None) continue;
            double x  = fretX(f);
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
                p.setBrush(QColor(180, 0, 0));
                p.drawEllipse(pt, r*0.35, r*0.35);
                p.setPen(QPen(Qt::black, 1.5));
                p.setBrush(Qt::NoBrush);
                break;
            case NoteState::Mute: {
                p.setPen(QPen(Qt::black, 1.5));
                double d = r * 0.75;
                p.drawLine(pt + QPointF(-d, -d), pt + QPointF( d,  d));
                p.drawLine(pt + QPointF(-d,  d), pt + QPointF( d, -d));
                break;
            }
            default: break;
            }
        }
    }
}