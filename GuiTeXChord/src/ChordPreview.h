#pragma once
#include <QWidget>
#include "ChordData.h"

class ChordPreview : public QWidget {
    Q_OBJECT
public:
    explicit ChordPreview(QWidget *parent = nullptr);
    void setChord(const ChordData &chord);

protected:
    void paintEvent(QPaintEvent *) override;
    QSize sizeHint() const override { return {260, 180}; }

private:
    void drawHorizontal(QPainter &p);
    void drawVertical  (QPainter &p);
    void setupTransform(double logL, double logR, double logB, double logT,
                        double mL,   double mR,   double mT,   double mB);
    QPointF toPixel(double lx, double ly) const;

    ChordData m_chord;
    mutable double m_sx=1, m_sy=1, m_ox=0, m_oy=0;
};
