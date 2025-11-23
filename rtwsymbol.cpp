#include "rtwsymbols.h"
#include <QFont>
#include <QPainterPath>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <cmath>

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
    QFont f("Noto Serif");
    f.setBold(true);
    f.setPointSize(14);
    // Improve font rendering
    f.setStyleStrategy(QFont::StyleStrategy(QFont::PreferAntialias | QFont::PreferQuality));
    f.setStyleHint(QFont::Serif);
    
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

    QRectF ellipseRect(4, 10, size-8, size-20); // Ellipse: wider than tall
    qreal centerX = ellipseRect.center().x();
    qreal centerY = ellipseRect.center().y();
    qreal radiusX = ellipseRect.width() / 2.0;  // Horizontal radius
    qreal radiusY = ellipseRect.height() / 1.5; // Vertical radius
    
    p.setPen(QPen(Qt::green, 2));
    
    // Create a wavy/scalloped ellipse border
    int numScallops = 14; // Number of up/down intervals
    QPainterPath wavyPath;
    
    for (int i = 0; i <= numScallops; ++i) {
        double angle1 = (i * 2.0 * M_PI) / numScallops;
        double angle2 = ((i + 1) * 2.0 * M_PI) / numScallops;
        
        // Calculate outer point (normal ellipse)
        qreal x1 = centerX + radiusX * cos(angle1);
        qreal y1 = centerY + radiusY * sin(angle1);
        qreal x2 = centerX + radiusX * cos(angle2);
        qreal y2 = centerY + radiusY * sin(angle2);
        
        // Calculate inner point (indented for scallop)
        double midAngle = (angle1 + angle2) / 2.0;
        qreal indentRadiusX = radiusX * 0.85; // 15% indent
        qreal indentRadiusY = radiusY * 0.85; // 15% indent
        qreal xMid = centerX + indentRadiusX * cos(midAngle);
        qreal yMid = centerY + indentRadiusY * sin(midAngle);
        
        if (i == 0) {
            wavyPath.moveTo(x1, y1);
        }
        
        // Draw curve from outer point, through indent, to next outer point
        QPointF p1(x1, y1);
        QPointF pMid(xMid, yMid);
        QPointF p2(x2, y2);
        
        // Create a quadratic curve for smooth scallop
        QPointF controlPoint = pMid;
        wavyPath.quadTo(controlPoint, p2);
    }
    
    wavyPath.closeSubpath();
    p.drawPath(wavyPath);

     // add a text in the center of the circle
     QRectF textRect(4, 4, size-8, size-8);
     p.setFont(makeFont());
     p.drawText(textRect, Qt::AlignCenter, "PP");
 

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

    QRectF circleRect(4, 4, size-8, size-8);
    qreal centerX = circleRect.center().x();
    qreal centerY = circleRect.center().y();
    qreal radius = circleRect.width() / 2.0;
    
    p.setPen(QPen(Qt::yellow, 2));
    
    // Create a wavy/scalloped circle border
    // We'll draw the circle with regular indentations (scallops)
    int numScallops = 14; // Number of up/down intervals
    QPainterPath wavyPath;
    
    for (int i = 0; i <= numScallops; ++i) {
        double angle1 = (i * 2.0 * M_PI) / numScallops;
        double angle2 = ((i + 1) * 2.0 * M_PI) / numScallops;
        
        // Calculate outer point (normal circle)
        qreal x1 = centerX + radius * cos(angle1);
        qreal y1 = centerY + radius * sin(angle1);
        qreal x2 = centerX + radius * cos(angle2);
        qreal y2 = centerY + radius * sin(angle2);
        
        // Calculate inner point (indented for scallop)
        double midAngle = (angle1 + angle2) / 2.0;
        qreal indentRadius = radius * 0.85; // 15% indent
        qreal xMid = centerX + indentRadius * cos(midAngle);
        qreal yMid = centerY + indentRadius * sin(midAngle);
        
        if (i == 0) {
            wavyPath.moveTo(x1, y1);
        }
        
        // Draw curve from outer point, through indent, to next outer point
        QPointF p1(x1, y1);
        QPointF pMid(xMid, yMid);
        QPointF p2(x2, y2);
        
        // Create a quadratic curve for smooth scallop
        QPointF controlPoint = pMid;
        wavyPath.quadTo(controlPoint, p2);
    }
    
    wavyPath.closeSubpath();
    p.drawPath(wavyPath);

    // p.setFont(makeFont());
    // p.drawText(, Qt::AlignCenter, "PP");

    // add a text in the center of the circle
    QRectF textRect(4, 4, size-8, size-8);
    p.setFont(makeFont());
    p.drawText(textRect, Qt::AlignCenter, "R");

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