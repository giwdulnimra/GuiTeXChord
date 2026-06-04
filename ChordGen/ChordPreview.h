#pragma once
#include <QWidget>
#include "ChordData.h"

// Rendert eine Grifftabelle direkt per QPainter, ohne LaTeX-Roundtrip.
// Spiegelt die Koordinatenlogik aus LatexGenerator, damit Vorschau
// und generiertes PDF identisch aussehen.
class ChordPreview : public QWidget {
    Q_OBJECT
public:
    explicit ChordPreview(QWidget *parent = nullptr);
    void setChord(const ChordData &chord);

protected:
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override { return {160, 230}; }

private:
    ChordData m_chord;

    // Mapping logische Koordinaten → Widget-Pixel
    QPointF toPixel(double lx, double ly) const;

    double m_marginX = 28.0;
    double m_marginY = 28.0;
    double m_scaleX  = 30.0;  // px per unit in x
    double m_scaleY  = 18.0;  // px per unit in y
};
