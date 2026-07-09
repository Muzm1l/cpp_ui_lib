#ifndef RTWSYMBOLDRAWING_H
#define RTWSYMBOLDRAWING_H

#include <QPainter>
#include <QPixmap>
#include <QMap>

class RTWSymbolDrawing
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
        BOTD,
        YellowCircle1,
        YellowCircle2,
        YellowCircle3,
        YellowCircle4,
        WhiteCircle1,   // Active (unselected) ruler 1
        WhiteCircle2,   // Active (unselected) ruler 2
        WhiteCircle3,   // Active (unselected) ruler 3
        WhiteCircle4,   // Active (unselected) ruler 4
        MaxSymbol,    // Yellow spine (left) with horizontal branches + circle tips (right)
        MinSymbol,    // Mirror of MaxSymbol: spine (right), branches + circle tips (left)
        Dummy1,       // Former MaxSymbol: yellow vertical line with 4 cyan dot lines behind
        Dummy2        // Former MinSymbol: yellow vertical line with 4 cyan dot lines in front
    };

    RTWSymbolDrawing(int baseSize = 20);  // size in pixels (reduced from 40)

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
    QPixmap makeYellowCircle1();
    QPixmap makeYellowCircle2();
    QPixmap makeYellowCircle3();
    QPixmap makeYellowCircle4();
    // Parameterized numbered-circle generator (used for ruler indicators)
    QPixmap makeNumberedCircle(int digit, const QColor &fill, const QColor &textColor);
    QPixmap makeMaxSymbol();
    QPixmap makeMinSymbol();
    QPixmap makeDummy1();
    QPixmap makeDummy2();

    // helpers
    QPixmap blank();
};

#endif
