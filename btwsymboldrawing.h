#ifndef BTWSYMBOLDRAWING_H
#define BTWSYMBOLDRAWING_H

#include <QPainter>
#include <QPixmap>
#include <QMap>

class BTWSymbolDrawing
{
public:
    enum class SymbolType {
        MagentaCircle,     // Small magenta circle for BTW automatic marker synchronization (hollow)
        MagentaCircleSynced  // Filled magenta circle for synced BTW symbols
    };

    BTWSymbolDrawing(int baseSize = 40);  // size in pixels

    void draw(QPainter* p, QPointF pos, SymbolType type);
    const QPixmap& get(SymbolType type) const;

private:
    int size;
    QMap<SymbolType, QPixmap> cache;

private:
    void generateAll();

    // functions to generate each symbol
    QPixmap makeMagentaCircle();
    QPixmap makeMagentaCircleSynced();

    // helpers
    QPixmap blank();
    QFont makeFont();
};

#endif

