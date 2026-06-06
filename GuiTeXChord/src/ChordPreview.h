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
    QSize sizeHint() const override { return {280, 130}; }

private:
    QPointF toPixel(double lx, double ly) const;

    ChordData m_chord;
    mutable double m_scaleX = 30.0, m_scaleY = 30.0;
    mutable double m_originX = 0.0, m_originY = 0.0;
};
