#ifndef RTWSYMBOLS_H
#define RTWSYMBOLS_H

#include <QPainter>
#include <QPixmap>
#include <QMap>

class RTWSymbols
{
public:
    enum class SymbolType {
        TM, DP, LY,
        CircleI,
        Triangle,
        RectR,
        EllipsePP,
        RectX,
        RectA,
        RectAPurple,
        RectK,
        CircleRYellow,
        DoubleBarYellow,
        R,
        L,
        BOT,
        BOTC,
        BOTF,
        BOTD
    };

    RTWSymbols(int baseSize = 40);  // size in pixels

    void draw(QPainter* p, QPointF pos, SymbolType type);
    const QPixmap& get(SymbolType type) const;

private:
    int size;
    QMap<SymbolType, QPixmap> cache;

private:
    void generateAll();

    // functions to generate each symbol
    QPixmap makeTM();
    QPixmap makeDP();
    QPixmap makeLY();
    QPixmap makeCircleI();
    QPixmap makeTriangle();
    QPixmap makeRectR();
    QPixmap makeEllipsePP();
    QPixmap makeRectX();
    QPixmap makeRectA();
    QPixmap makeRectAPurple();
    QPixmap makeRectK();
    QPixmap makeCircleRYellow();
    QPixmap makeDoubleBarYellow();
    QPixmap R();
    QPixmap L();
    QPixmap BOT();
    QPixmap BOTC();
    QPixmap BOTF();
    QPixmap BOTD();

    // helpers
    QPixmap blank();
};

#endif
