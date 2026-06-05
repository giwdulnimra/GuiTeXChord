#pragma once
#include <QWidget>
#include "ChordData.h"

class ChordPreview : public QWidget {
    Q_OBJECT
public:
    explicit ChordPreview(QWidget *parent = nullptr);
    void setChord(const ChordData &chord);

protected:
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override { return {260, 140}; }

private:
    QPointF toPixel(double lx, double ly) const;

    ChordData m_chord;

    // werden in paintEvent() gesetzt
    mutable double m_scaleX  = 30.0;
    mutable double m_scaleY  = 30.0;
    mutable double m_originX = 0.0;
    mutable double m_originY = 0.0;
};
