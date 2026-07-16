#include "chevroncombobox.h"

#include <QPainter>
#include <QPainterPath>
#include <QStyle>
#include <QStyleOptionComboBox>
#include <QtMath>

ChevronComboBox::ChevronComboBox(QWidget *parent)
    : QComboBox(parent)
    , m_arrowColor(QColor(0x00, 0x4C, 0x99)) // dark blue (border + chevron)
{
    // Suppress the native arrow glyph only; the rest of the combo box keeps its
    // native look. We paint our own circular double-chevron on top in paintEvent.
    setStyleSheet(QStringLiteral("QComboBox::down-arrow { image: none; width: 0px; height: 0px; }"));
}

void ChevronComboBox::setArrowColor(const QColor &color)
{
    if (m_arrowColor == color)
        return;
    m_arrowColor = color;
    update();
}

void ChevronComboBox::setShowCircle(bool show)
{
    if (m_showCircle == show)
        return;
    m_showCircle = show;
    update();
}

void ChevronComboBox::paintEvent(QPaintEvent *event)
{
    // Let the base class draw the frame, current text and the (now blank) arrow area.
    QComboBox::paintEvent(event);

    QStyleOptionComboBox opt;
    initStyleOption(&opt);

    const QRect arrowRect = style()->subControlRect(
        QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxArrow, this);
    if (arrowRect.isEmpty())
        return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // Square drawing box centered in the arrow sub-control, with a small inset.
    const qreal side = qMin(arrowRect.width(), arrowRect.height()) - 4.0;
    if (side <= 2.0)
        return;

    QRectF box(0.0, 0.0, side, side);
    box.moveCenter(QRectF(arrowRect).center());

    QPen pen(m_arrowColor);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setBrush(Qt::NoBrush);

    // Enclosing circle.
    if (m_showCircle)
    {
        pen.setWidthF(qMax(1.0, side * 0.07));
        p.setPen(pen);
        // Slightly inset so the stroke stays inside the box.
        const qreal r = side * 0.5 - pen.widthF() * 0.5;
        p.drawEllipse(box.center(), r, r);
    }

    // Double chevron (two stacked "v" shapes pointing down), centered in the circle.
    pen.setWidthF(qMax(1.0, side * 0.09));
    p.setPen(pen);

    const QPointF c = box.center();
    const qreal halfW = side * 0.22;  // horizontal half-width of each chevron
    const qreal armH = side * 0.13;   // how far the chevron tip drops below its arms
    const qreal gap = side * 0.24;    // vertical spacing between the two chevron midpoints

    // Place the pair symmetrically around the circle center (was biased upward).
    for (int i = 0; i < 2; ++i)
    {
        const qreal midY = c.y() + (i == 0 ? -gap * 0.5 : gap * 0.5);
        const qreal topY = midY - armH * 0.5;
        QPainterPath path;
        path.moveTo(c.x() - halfW, topY);
        path.lineTo(c.x(), topY + armH);
        path.lineTo(c.x() + halfW, topY);
        p.drawPath(path);
    }
}
