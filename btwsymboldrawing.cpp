#include "btwsymboldrawing.h"
#include <QFont>
#include <QPainterPath>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QDebug>
#include <cmath>

BTWSymbolDrawing::BTWSymbolDrawing(int baseSize)
    : size(baseSize)
{
    generateAll();
}

void BTWSymbolDrawing::draw(QPainter* p, QPointF pos, SymbolType type)
{
    const QPixmap& pix = cache[type];
    p->drawPixmap(pos.x() - pix.width()/2,
                  pos.y() - pix.height()/2,
                  pix);
}

const QPixmap& BTWSymbolDrawing::get(SymbolType type) const
{
    // Use constFind to safely access the cache without creating default entries
    auto it = cache.constFind(type);
    if (it != cache.constEnd())
    {
        return it.value();
    }
    
    // If not found, return a reference to a static empty pixmap
    static QPixmap emptyPixmap;
    qDebug() << "BTWSymbolDrawing::get - Symbol type" << static_cast<int>(type) << "not found in cache!";
    return emptyPixmap;
}

void BTWSymbolDrawing::generateAll()
{
    cache[SymbolType::MagentaCircle] = makeMagentaCircle();
}

/* ----------------- Helpers ----------------- */

QPixmap BTWSymbolDrawing::blank()
{
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);
    return pix;
}

QFont BTWSymbolDrawing::makeFont()
{
    QFont font("Calisto MT", size/3);
    font.setBold(true);
    return font;
}

/* ----------------- Symbol Generators ----------------- */

// Small magenta circle for BTW automatic marker synchronization
QPixmap BTWSymbolDrawing::makeMagentaCircle()
{
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    // Draw a small hollow magenta circle
    QColor magentaColor(255, 0, 255); // Magenta color

    // Use a cosmetic pen so the outline stays 1–2 px regardless of zoom
    QPen pen(magentaColor);
    pen.setWidthF(1.5);      // Thin outline
    pen.setCosmetic(true);   // Width in screen pixels, not scene units
    p.setPen(pen);
    p.setBrush(Qt::NoBrush); // Hollow circle - no fill
    
    // Keep it smaller than RTW symbols, but large enough to see the hole
    qreal circleSize = 10.0; // Slightly larger than 8px so the ring is visible
    qreal offset = (size - circleSize) / 2.0;
    p.drawEllipse(QRectF(offset, offset, circleSize, circleSize));

    return pix;
}

