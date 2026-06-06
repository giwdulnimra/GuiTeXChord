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
    QSize sizeHint() const override { return {300, 180}; }

private:
    void drawVertical  (QPainter &p);
    void drawHorizontal(QPainter &p);
    QPointF toPixel(double lx, double ly) const;

    ChordData m_chord;
    mutable double m_sx=1, m_sy=1, m_ox=0, m_oy=0;
};
