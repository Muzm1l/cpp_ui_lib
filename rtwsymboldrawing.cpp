#include "rtwsymboldrawing.h"
#include "debugutils.h"
#include <QFont>
#include <QPainterPath>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QDebug>
#include <cmath>

static const int RTW_SYMBOL_FONT_SIZE = 10;
// Match BTW numbered-circle glyph size (small circle centered in the pixmap canvas).
static const qreal RTW_NUMBERED_CIRCLE_SIZE = 14.0;

RTWSymbolDrawing::RTWSymbolDrawing(int baseSize)
    : size(baseSize)
{
    generateAll();
}

void RTWSymbolDrawing::draw(QPainter* p, QPointF pos, SymbolType type)
{
    const QPixmap& pix = cache[type];
    p->drawPixmap(pos.x() - pix.width()/2,
                  pos.y() - pix.height()/2,
                  pix);
}

const QPixmap& RTWSymbolDrawing::get(SymbolType type) const
{
    // Use constFind to safely access the cache without creating default entries
    auto it = cache.constFind(type);
    if (it != cache.constEnd())
    {
        return it.value();
    }
    
    // If not found, return a reference to a static empty pixmap
    // This should never happen if generateAll() was called properly
    static QPixmap emptyPixmap;
    DEBUG_OUT() << "RTWSymbolDrawing::get - Symbol type" << static_cast<int>(type) << "not found in cache!";
    return emptyPixmap;
}

void RTWSymbolDrawing::generateAll()
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
    cache[SymbolType::YellowCircle1] = makeYellowCircle1();
    cache[SymbolType::YellowCircle2] = makeYellowCircle2();
    cache[SymbolType::YellowCircle3] = makeYellowCircle3();
    cache[SymbolType::YellowCircle4] = makeYellowCircle4();
    cache[SymbolType::WhiteCircle1] = makeNumberedCircle(1, Qt::white, Qt::black);
    cache[SymbolType::WhiteCircle2] = makeNumberedCircle(2, Qt::white, Qt::black);
    cache[SymbolType::WhiteCircle3] = makeNumberedCircle(3, Qt::white, Qt::black);
    cache[SymbolType::WhiteCircle4] = makeNumberedCircle(4, Qt::white, Qt::black);
    cache[SymbolType::MaxSymbol] = makeMaxSymbol();
    cache[SymbolType::MinSymbol] = makeMinSymbol();
    cache[SymbolType::Dummy1]    = makeDummy1();
    cache[SymbolType::Dummy2]    = makeDummy2();
}

/* ----------------- Helpers ----------------- */

QPixmap RTWSymbolDrawing::blank()
{
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);
    return pix;
}

static QFont makeFont()
{
    QFont f("Noto Serif");
    f.setBold(true);
    f.setPointSize(RTW_SYMBOL_FONT_SIZE);
    // Improve font rendering
    f.setStyleStrategy(QFont::StyleStrategy(QFont::PreferAntialias | QFont::PreferQuality));
    f.setStyleHint(QFont::Serif);
    
    return f;
}

/* ----------------- Generators ----------------- */

// rectangle with letter TM in centre, font calisto MT
// Name : TTM Range
QPixmap RTWSymbolDrawing::makeTM()
{
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF box(3, 3, size-6, size-6);
    p.setPen(Qt::white);
    p.drawRect(box);

    p.setFont(makeFont());
    p.drawText(box, Qt::AlignCenter, "TM");

    return pix;
}

// rectangle with letter DP in centre, font calisto MT
// Name : DOPPLER Range
QPixmap RTWSymbolDrawing::makeDP()
{
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF box(3, 3, size-6, size-6);
    p.setPen(Qt::white);
    p.drawRect(box);

    p.setFont(makeFont());
    p.drawText(box, Qt::AlignCenter, "DP");

    return pix;
}

// rectangle with letter LY in centre, font calisto MT
// Name : LLOYD Range
QPixmap RTWSymbolDrawing::makeLY()
{
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF box(3, 3, size-6, size-6);
    p.setPen(Qt::white);
    p.drawRect(box);

    p.setFont(makeFont());
    p.drawText(box, Qt::AlignCenter, "LY");

    return pix;
}

// circle with letter I in centre, font calisto MT
// Name : SONAR Range (DEPENDING N THE LEVEL)
QPixmap RTWSymbolDrawing::makeCircleI()
{
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    p.setPen(QPen(Qt::cyan, 2));
    p.drawEllipse(QRectF(3, 3, size-6, size-6));

    p.setFont(makeFont());
    p.drawText(QRectF(3, 3, size-6, size-6), Qt::AlignCenter, "I");

    return pix;
}



// solid triangle of white color
// Name : INTECEPTION SONAR LEVEL MEASURE
QPixmap RTWSymbolDrawing::makeTriangle()
{
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QPolygonF tri;
    tri << QPointF(size/2, 3)
        << QPointF(3, size-3)
        << QPointF(size-3, size-3);

    p.setPen(Qt::white);        // Border color
    p.setBrush(Qt::white);      // Solid fill

    p.drawPolygon(tri);         // Will draw a filled triangle

    return pix;
}


// rectangle with letter R in centre, font calisto MT
// Name : RADAR Range
QPixmap RTWSymbolDrawing::makeRectR()
{
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF box(3, 3, size-6, size-6);
    p.setPen(Qt::cyan);
    p.drawRect(box);

    p.setFont(makeFont());
    p.drawText(box, Qt::AlignCenter, "R");

    return pix;
}

// ellipse with letter PP in centre, font calisto MT
// Name : RULER PIVOT Range
QPixmap RTWSymbolDrawing::makeEllipsePP()
{
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF ellipseRect(3, 9, size-6, size-18); // Ellipse: wider than tall
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
     QRectF textRect(3, 3, size-6, size-6);
     p.setFont(makeFont());
     p.drawText(textRect, Qt::AlignCenter, "PP");
 

    return pix;
}

//make a rectangle with a X in centre
// Name : EXTERNAL Range
QPixmap RTWSymbolDrawing::makeRectX()
{
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF box(3, 3, size-6, size-6);
    p.setPen(Qt::cyan);
    p.drawRect(box);

    p.setFont(makeFont());
    p.drawText(box, Qt::AlignCenter, "X");

    return pix;
}

//  symbols: rectangle with letter A in centre, color red
// Name : REAL TIME ADPTION
QPixmap RTWSymbolDrawing::makeRectA()
{
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF box(3, 3, size-6, size-6);
    p.setPen(Qt::red);
    p.drawRect(box);

    p.setFont(makeFont());
    p.drawText(box, Qt::AlignCenter, "A");

    return pix;
}


//  symbols: rectangle with letter A in centre color purple
// Name : PAST TIME ADPTION
QPixmap RTWSymbolDrawing::makeRectAPurple()
{
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF box(3, 3, size-6, size-6);
    p.setPen(QColor(128, 0, 128)); // Purple color
    p.drawRect(box);
    
    p.setFont(makeFont());
    p.drawText(box, Qt::AlignCenter, "A");

    return pix;
}

//------RTW MANUAL LOCATION RANGE SYMBOLS------

//  symbols: rectangle with letter K in centre color CYAN
// Name : EKELUND Range
QPixmap RTWSymbolDrawing::makeRectK()
{
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF box(3, 3, size-6, size-6);
    p.setPen(Qt::cyan);
    p.drawRect(box);
    
    p.setFont(makeFont());
    p.drawText(box, Qt::AlignCenter, "K");

    return pix;
}

//  symbols: circle with letter R in centre color yellow
// Name : LATERAL Range
QPixmap RTWSymbolDrawing::makeCircleRYellow()
{
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF circleRect(3, 3, size-6, size-6);
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
    QRectF textRect(3, 3, size-6, size-6);
    p.setFont(makeFont());
    p.drawText(textRect, Qt::AlignCenter, "R");

    return pix;
}


//  symbols: || in color yellow
// Name : MIN/MAX Range
QPixmap RTWSymbolDrawing::makeDoubleBarYellow()
{
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF box(3, 3, size-6, size-6);
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
QPixmap RTWSymbolDrawing::R(){
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    
    QRectF box(3, 3, size-6, size-6);
    p.setPen(QColor(255, 165, 0)); // Orange color
    
    p.setFont(makeFont());
    p.drawText(box, Qt::AlignCenter, "R");

    return pix;
}

//----RTWGLOBAL METHODS RANGES METHODOLOGY----

// Letter L in a circle , color green
// Name: BOPT
QPixmap RTWSymbolDrawing::L(){
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF box(3, 3, size-6, size-6);
    p.setPen(Qt::green);
    p.drawEllipse(box);

    p.setFont(makeFont());
    p.drawText(box, Qt::AlignCenter, "L");

    return pix;
}

// Letter L in a RECTANGLE , color green
// Name: BOT
QPixmap RTWSymbolDrawing::BOT(){
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF box(3, 3, size-6, size-6);
    p.setPen(Qt::green);
    p.drawRect(box);
    
    p.setFont(makeFont());
    p.drawText(box, Qt::AlignCenter, "L");

    return pix;
}

// Letter C , color green
// Name: BOTC
QPixmap RTWSymbolDrawing::BOTC(){
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF box(3, 3, size-6, size-6);
    p.setPen(Qt::green);
    
    p.setFont(makeFont());
    p.drawText(box, Qt::AlignCenter, "C");

    return pix;
}

// Letter F, color green
// Name: BFT
QPixmap RTWSymbolDrawing::BOTF(){
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF box(3, 3, size-6, size-6);
    p.setPen(Qt::green);
    
    p.setFont(makeFont());
    p.drawText(box, Qt::AlignCenter, "F");

    return pix;
}

//-----RTW GLOBAL METHODS RANGES BRAT METHODOLOGY----

// Letter D, color green
// Name: BRAT
QPixmap RTWSymbolDrawing::BOTD(){
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF box(3, 3, size-6, size-6);
    p.setPen(Qt::green);
    
    p.setFont(makeFont());
    p.drawText(box, Qt::AlignCenter, "D");

    return pix;
}

// Yellow solid circle with white number 1
// Parameterized numbered circle: solid fill with a centered digit.
// Used for ruler indicators (yellow = selected, white = active/unselected).
QPixmap RTWSymbolDrawing::makeNumberedCircle(int digit, const QColor &fill, const QColor &textColor)
{
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    const qreal offset = (size - RTW_NUMBERED_CIRCLE_SIZE) / 2.0;
    const QRectF circleRect(offset, offset, RTW_NUMBERED_CIRCLE_SIZE, RTW_NUMBERED_CIRCLE_SIZE);

    p.setPen(Qt::NoPen);
    p.setBrush(fill);
    p.drawEllipse(circleRect);

    p.setPen(textColor);
    p.setBrush(Qt::NoBrush);
    p.setFont(makeFont());
    p.drawText(circleRect, Qt::AlignCenter, QString::number(digit));

    return pix;
}

QPixmap RTWSymbolDrawing::makeYellowCircle1()
{
    return makeNumberedCircle(1, Qt::yellow, Qt::white);
}

QPixmap RTWSymbolDrawing::makeYellowCircle2()
{
    return makeNumberedCircle(2, Qt::yellow, Qt::white);
}

QPixmap RTWSymbolDrawing::makeYellowCircle3()
{
    return makeNumberedCircle(3, Qt::yellow, Qt::white);
}

QPixmap RTWSymbolDrawing::makeYellowCircle4()
{
    return makeNumberedCircle(4, Qt::yellow, Qt::white);
}

// Max symbol: like Dummy1 (yellow vertical line with 4 parallel dotted lines) but each
// dotted line is slanted 45 degrees clockwise ("\") instead of horizontal.
QPixmap RTWSymbolDrawing::makeMaxSymbol()
{
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    const qreal centerX = size / 2.0;
    const qreal dotSize = 1.8;
    const qreal dotStep = 2.4;                         // spacing between dot centres along the diagonal
    const int   numDots = 4;
    const int   numLines = 4;
    const qreal comp = dotStep * 0.70710678;           // 45-degree x/y component
    const qreal diag = (numDots - 1) * comp;           // total x (and y) span of one dotted line

    // Yellow vertical spine
    p.setPen(QPen(Qt::yellow, 2));
    p.drawLine(QPointF(centerX, 3), QPointF(centerX, size - 3));

    // 4 parallel diagonal dotted lines (45 deg clockwise), to the left of the spine.
    // Each line runs from its upper-left dot down to its lower-right dot near the spine.
    p.setPen(QPen(Qt::yellow, 1));
    p.setBrush(QBrush(Qt::yellow));

    const qreal lineSpacing = (size - 6.0 - diag) / (numLines - 1); // fit all lines vertically
    const qreal xNear = centerX - 2.0;                 // dot nearest the spine (its high end)
    for (int line = 0; line < numLines; ++line)
    {
        const qreal yTop = 3.0 + line * lineSpacing;   // near-spine dot of this line
        for (int dot = 0; dot < numDots; ++dot)
        {
            // Extend outward (left) and downward: the line droops down away from the spine.
            const qreal xPos = xNear - dot * comp;
            const qreal yPos = yTop + dot * comp;
            p.drawEllipse(QRectF(xPos - dotSize / 2, yPos - dotSize / 2, dotSize, dotSize));
        }
    }

    return pix;
}

// Min symbol: mirror image of the Max symbol - dotted lines slanted the other way ("/"),
// on the right of the yellow vertical spine.
QPixmap RTWSymbolDrawing::makeMinSymbol()
{
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    const qreal centerX = size / 2.0;
    const qreal dotSize = 1.8;
    const qreal dotStep = 2.4;
    const int   numDots = 4;
    const int   numLines = 4;
    const qreal comp = dotStep * 0.70710678;
    const qreal diag = (numDots - 1) * comp;

    // Yellow vertical spine
    p.setPen(QPen(Qt::yellow, 2));
    p.drawLine(QPointF(centerX, 3), QPointF(centerX, size - 3));

    // 4 parallel diagonal dotted lines, mirrored about the spine (dots on the right, "/").
    p.setPen(QPen(Qt::yellow, 1));
    p.setBrush(QBrush(Qt::yellow));

    const qreal lineSpacing = (size - 6.0 - diag) / (numLines - 1);
    const qreal xNear = centerX + 2.0;                 // dot nearest the spine (its high end)
    for (int line = 0; line < numLines; ++line)
    {
        const qreal yTop = 3.0 + line * lineSpacing;
        for (int dot = 0; dot < numDots; ++dot)
        {
            // Mirror of Max: extend outward (right) and downward.
            const qreal xPos = xNear + dot * comp;
            const qreal yPos = yTop + dot * comp;
            p.drawEllipse(QRectF(xPos - dotSize / 2, yPos - dotSize / 2, dotSize, dotSize));
        }
    }

    return pix;
}

// Dummy 1 (former Max symbol): yellow vertical line with 4 lines made of 4 cyan dots each behind it
QPixmap RTWSymbolDrawing::makeDummy1()
{
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    
    qreal centerX = size / 2.0;
    qreal dotSize = 2.0; // Size of each dot
    qreal dotSpacing = 3.0; // Spacing between dots
    qreal totalDotWidth = 4 * dotSize + 3 * dotSpacing; // Total width of 4 dots with spacing
    qreal availableHeight = size - 6; // Available vertical space
    qreal lineSpacing = availableHeight / 5.0; // Evenly space 4 lines with margins
    
    // Draw yellow vertical line in center
    p.setPen(QPen(Qt::yellow, 2));
    p.drawLine(QPointF(centerX, 3), QPointF(centerX, size - 3));
    
    // Draw 4 horizontal lines made of 4 cyan dots each, positioned behind (left of) the yellow line
    p.setPen(QPen(Qt::cyan, 1));
    p.setBrush(QBrush(Qt::cyan));
    
    for (int line = 0; line < 4; ++line)
    {
        qreal yPos = 3 + (line + 1) * lineSpacing;
        qreal startX = centerX - totalDotWidth - 2; // Position dots to the left of yellow line
        
        // Draw 4 dots in a horizontal line
        for (int dot = 0; dot < 4; ++dot)
        {
            qreal xPos = startX + dot * (dotSize + dotSpacing);
            p.drawEllipse(QRectF(xPos - dotSize/2, yPos - dotSize/2, dotSize, dotSize));
        }
    }
    
    return pix;
}

// Dummy 2 (former Min symbol): yellow vertical line with 4 lines made of 4 cyan dots each in front
QPixmap RTWSymbolDrawing::makeDummy2()
{
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    
    qreal centerX = size / 2.0;
    qreal dotSize = 2.0; // Size of each dot
    qreal dotSpacing = 3.0; // Spacing between dots
    qreal availableHeight = size - 6; // Available vertical space
    qreal lineSpacing = availableHeight / 5.0; // Evenly space 4 lines with margins
    
    // Draw yellow vertical line in center
    p.setPen(QPen(Qt::yellow, 2));
    p.drawLine(QPointF(centerX, 3), QPointF(centerX, size - 3));
    
    // Draw 4 horizontal lines made of 4 cyan dots each, positioned in front (right of) the yellow line
    p.setPen(QPen(Qt::cyan, 1));
    p.setBrush(QBrush(Qt::cyan));
    
    for (int line = 0; line < 4; ++line)
    {
        qreal yPos = 3 + (line + 1) * lineSpacing;
        qreal startX = centerX + 2; // Position dots to the right of yellow line
        
        // Draw 4 dots in a horizontal line
        for (int dot = 0; dot < 4; ++dot)
        {
            qreal xPos = startX + dot * (dotSize + dotSpacing);
            p.drawEllipse(QRectF(xPos - dotSize/2, yPos - dotSize/2, dotSize, dotSize));
        }
    }
    
    return pix;
}