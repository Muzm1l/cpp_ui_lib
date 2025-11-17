#include "rtwsymbols.h"
#include <QFont>
#include <QPainterPath>
#include <QPen>
#include <QBrush>
#include <QColor>

RTWSymbols::RTWSymbols(int baseSize)
    : size(baseSize)
{
    generateAll();
}

void RTWSymbols::draw(QPainter* p, QPointF pos, SymbolType type)
{
    const QPixmap& pix = cache[type];
    p->drawPixmap(pos.x() - pix.width()/2,
                  pos.y() - pix.height()/2,
                  pix);
}

const QPixmap& RTWSymbols::get(SymbolType type) const
{
    return cache[type];
}

void RTWSymbols::generateAll()
{
    cache[SymbolType::TM]        = makeTM();
    cache[SymbolType::DP]        = makeDP();
    cache[SymbolType::LY]        = makeLY();
    cache[SymbolType::CircleI]   = makeCircleI();
    cache[SymbolType::Triangle]  = makeTriangle();
    cache[SymbolType::RectR]     = makeRectR();
    cache[SymbolType::EllipsePP] = makeEllipsePP();
    cache[SymbolType::RectX]     = makeRectX();
    cache[SymbolType::RectA]     = makeRectA();
    cache[SymbolType::RectAPurple] = makeRectAPurple();
    cache[SymbolType::RectK]     = makeRectK();
    cache[SymbolType::CircleRYellow] = makeCircleRYellow();
    cache[SymbolType::DoubleBarYellow] = makeDoubleBarYellow();
    cache[SymbolType::R]          = R();
    cache[SymbolType::L]          = L();
    cache[SymbolType::BOT]        = BOT();
    cache[SymbolType::BOTC]      = BOTC();
    cache[SymbolType::BOTF]       = BOTF();
    cache[SymbolType::BOTD]       = BOTD();
}

/* ----------------- Helpers ----------------- */

QPixmap RTWSymbols::blank()
{
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);
    return pix;
}

static QFont makeFont()
{
    QFont f("Calisto MT");
    f.setBold(true);
    f.setPixelSize(18);    // adjust
    return f;
}

/* ----------------- Generators ----------------- */

// rectangle with letter TM in centre, font calisto MT
// Name : TTM Range
QPixmap RTWSymbols::makeTM()
{
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF box(4, 4, size-8, size-8);
    p.setPen(Qt::white);
    p.drawRect(box);

    p.setFont(makeFont());
    p.drawText(box, Qt::AlignCenter, "TM");

    return pix;
}

// rectangle with letter DP in centre, font calisto MT
// Name : DOPPLER Range
QPixmap RTWSymbols::makeDP()
{
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF box(4, 4, size-8, size-8);
    p.setPen(Qt::white);
    p.drawRect(box);

    p.setFont(makeFont());
    p.drawText(box, Qt::AlignCenter, "DP");

    return pix;
}

// rectangle with letter LY in centre, font calisto MT
// Name : LLOYD Range
QPixmap RTWSymbols::makeLY()
{
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF box(4, 4, size-8, size-8);
    p.setPen(Qt::white);
    p.drawRect(box);

    p.setFont(makeFont());
    p.drawText(box, Qt::AlignCenter, "LY");

    return pix;
}

// circle with letter I in centre, font calisto MT
// Name : SONAR Range (DEPENDING N THE LEVEL)
QPixmap RTWSymbols::makeCircleI()
{
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    p.setPen(QPen(Qt::cyan, 2));
    p.drawEllipse(QRectF(4, 4, size-8, size-8));

    p.setFont(makeFont());
    p.drawText(QRectF(4, 4, size-8, size-8), Qt::AlignCenter, "I");

    return pix;
}



// solid triangle of white color
// Name : INTECEPTION SONAR LEVEL MEASURE
QPixmap RTWSymbols::makeTriangle()
{
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QPolygonF tri;
    tri << QPointF(size/2, 4)
        << QPointF(4, size-4)
        << QPointF(size-4, size-4);

    p.setPen(Qt::white);        // Border color
    p.setBrush(Qt::white);      // Solid fill

    p.drawPolygon(tri);         // Will draw a filled triangle

    return pix;
}


// rectangle with letter R in centre, font calisto MT
// Name : RADAR Range
QPixmap RTWSymbols::makeRectR()
{
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF box(4, 4, size-8, size-8);
    p.setPen(Qt::cyan);
    p.drawRect(box);

    p.setFont(makeFont());
    p.drawText(box, Qt::AlignCenter, "R");

    return pix;
}

// ellipse with letter PP in centre, font calisto MT
// Name : RULER PIVOT Range
QPixmap RTWSymbols::makeEllipsePP()
{
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF e(4, 10, size-8, size-20);
    
    // Create a textured brush for the border
    QPixmap texture(4, 4);
    texture.fill(Qt::transparent);
    QPainter texturePainter(&texture);
    texturePainter.setPen(Qt::white);
    // Create a simple dot pattern texture
    texturePainter.drawPoint(0, 0);
    texturePainter.drawPoint(2, 2);
    texturePainter.drawPoint(1, 3);
    texturePainter.drawPoint(3, 1);
    
    QBrush texturedBrush(texture);
    QPen texturedPen(texturedBrush, 2.0);
    texturedPen.setStyle(Qt::SolidLine);
    
    p.setPen(texturedPen);
    p.drawEllipse(e);

    p.setFont(makeFont());
    p.drawText(e, Qt::AlignCenter, "PP");

    return pix;
}

//make a rectangle with a X in centre
// Name : EXTERNAL Range
QPixmap RTWSymbols::makeRectX()
{
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF box(4, 4, size-8, size-8);
    p.setPen(Qt::cyan);
    p.drawRect(box);

    p.setFont(makeFont());
    p.drawText(box, Qt::AlignCenter, "X");

    return pix;
}

//  symbols: rectangle with letter A in centre, color red
// Name : REAL TIME ADPTION
QPixmap RTWSymbols::makeRectA()
{
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF box(4, 4, size-8, size-8);
    p.setPen(Qt::red);
    p.drawRect(box);

    p.setFont(makeFont());
    p.drawText(box, Qt::AlignCenter, "A");

    return pix;
}


//  symbols: rectangle with letter A in centre color purple
// Name : PAST TIME ADPTION
QPixmap RTWSymbols::makeRectAPurple()
{
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF box(4, 4, size-8, size-8);
    p.setPen(QColor(128, 0, 128)); // Purple color
    p.drawRect(box);
    
    p.setFont(makeFont());
    p.drawText(box, Qt::AlignCenter, "A");

    return pix;
}

//------RTW MANUAL LOCATION RANGE SYMBOLS------

//  symbols: rectangle with letter K in centre color CYAN
// Name : EKELUND Range
QPixmap RTWSymbols::makeRectK()
{
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF box(4, 4, size-8, size-8);
    p.setPen(Qt::cyan);
    p.drawRect(box);
    
    p.setFont(makeFont());
    p.drawText(box, Qt::AlignCenter, "K");

    return pix;
}

//  symbols: circle with letter R in centre color yellow
// Name : LATERAL Range
QPixmap RTWSymbols::makeCircleRYellow()
{
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF box(4, 4, size-8, size-8);
    p.setPen(Qt::yellow);
    p.drawEllipse(box);
    
    p.setFont(makeFont());
    p.drawText(box, Qt::AlignCenter, "R");

    return pix;
}

//  symbols: || in color yellow
// Name : MIN/MAX Range
QPixmap RTWSymbols::makeDoubleBarYellow()
{
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF box(4, 4, size-8, size-8);
    p.setPen(QPen(Qt::yellow, 2));
    // Draw two vertical parallel lines
    qreal centerX = box.center().x();
    qreal spacing = 4.0;
    p.drawLine(QPointF(centerX - spacing, box.top()), QPointF(centerX - spacing, box.bottom()));
    p.drawLine(QPointF(centerX + spacing, box.top()), QPointF(centerX + spacing, box.bottom()));

    return pix;
}


//---RTW AUTOMATIC GLOBAL METHODS RANGES METHODOLOGY---

// symbol: letter R in orange color, no circle
// Name: ATMA-ATMAF
QPixmap RTWSymbols::R(){
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    
    QRectF box(4, 4, size-8, size-8);
    p.setPen(QColor(255, 165, 0)); // Orange color
    
    p.setFont(makeFont());
    p.drawText(box, Qt::AlignCenter, "R");

    return pix;
}

//----RTWGLOBAL METHODS RANGES METHODOLOGY----

// Letter L in a circle , color green
// Name: BOPT
QPixmap RTWSymbols::L(){
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF box(4, 4, size-8, size-8);
    p.setPen(Qt::green);
    p.drawEllipse(box);

    p.setFont(makeFont());
    p.drawText(box, Qt::AlignCenter, "L");

    return pix;
}

// Letter L in a RECTANGLE , color green
// Name: BOT
QPixmap RTWSymbols::BOT(){
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF box(4, 4, size-8, size-8);
    p.setPen(Qt::green);
    p.drawRect(box);
    
    p.setFont(makeFont());
    p.drawText(box, Qt::AlignCenter, "L");

    return pix;
}

// Letter C , color green
// Name: BOTC
QPixmap RTWSymbols::BOTC(){
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF box(4, 4, size-8, size-8);
    p.setPen(Qt::green);
    
    p.setFont(makeFont());
    p.drawText(box, Qt::AlignCenter, "C");

    return pix;
}

// Letter F, color green
// Name: BFT
QPixmap RTWSymbols::BOTF(){
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF box(4, 4, size-8, size-8);
    p.setPen(Qt::green);
    
    p.setFont(makeFont());
    p.drawText(box, Qt::AlignCenter, "F");

    return pix;
}

//-----RTW GLOBAL METHODS RANGES BRAT METHODOLOGY----

// Letter D, color green
// Name: BRAT
QPixmap RTWSymbols::BOTD(){
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF box(4, 4, size-8, size-8);
    p.setPen(Qt::green);
    
    p.setFont(makeFont());
    p.drawText(box, Qt::AlignCenter, "D");

    return pix;
}