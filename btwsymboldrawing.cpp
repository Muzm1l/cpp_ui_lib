#include "btwsymboldrawing.h"
#include "debugutils.h"
#include <QFont>
#include <QPen>
#include <QBrush>
#include <QColor>

static const int BTW_SYMBOL_FONT_SIZE = 10;

static QString normalizeSymbolName(const QString &symbolName)
{
    return symbolName.trimmed().toUpper().remove('_').remove(' ');
}

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
    auto it = cache.constFind(type);
    if (it != cache.constEnd())
    {
        return it.value();
    }

    static QPixmap emptyPixmap;
    DEBUG_OUT() << "BTWSymbolDrawing::get - Symbol type" << static_cast<int>(type) << "not found in cache!";
    return emptyPixmap;
}

BTWSymbolDrawing::SymbolType BTWSymbolDrawing::symbolNameToType(const QString &symbolName)
{
    const QString name = normalizeSymbolName(symbolName);

    if (name == "MAGENTACIRCLE") return SymbolType::MagentaCircle;
    if (name == "MAGENTACIRCLESYNCED") return SymbolType::MagentaCircleSynced;
    if (name == "YELLOWCIRCLE1" || name == "YC1") return SymbolType::YellowCircle1;
    if (name == "YELLOWCIRCLE2" || name == "YC2") return SymbolType::YellowCircle2;
    if (name == "YELLOWCIRCLE3" || name == "YC3") return SymbolType::YellowCircle3;
    if (name == "YELLOWCIRCLE4" || name == "YC4") return SymbolType::YellowCircle4;
    if (name == "WHITECIRCLE1" || name == "WC1") return SymbolType::WhiteCircle1;
    if (name == "WHITECIRCLE2" || name == "WC2") return SymbolType::WhiteCircle2;
    if (name == "WHITECIRCLE3" || name == "WC3") return SymbolType::WhiteCircle3;
    if (name == "WHITECIRCLE4" || name == "WC4") return SymbolType::WhiteCircle4;

    DEBUG_OUT() << "BTWSymbolDrawing::symbolNameToType - Unknown symbol" << symbolName << ", defaulting to MagentaCircle";
    return SymbolType::MagentaCircle;
}

QString BTWSymbolDrawing::symbolTypeToName(SymbolType type)
{
    switch (type) {
    case SymbolType::MagentaCircle:       return QStringLiteral("MagentaCircle");
    case SymbolType::MagentaCircleSynced: return QStringLiteral("MagentaCircleSynced");
    case SymbolType::YellowCircle1:     return QStringLiteral("YellowCircle1");
    case SymbolType::YellowCircle2:     return QStringLiteral("YellowCircle2");
    case SymbolType::YellowCircle3:     return QStringLiteral("YellowCircle3");
    case SymbolType::YellowCircle4:     return QStringLiteral("YellowCircle4");
    case SymbolType::WhiteCircle1:      return QStringLiteral("WhiteCircle1");
    case SymbolType::WhiteCircle2:      return QStringLiteral("WhiteCircle2");
    case SymbolType::WhiteCircle3:      return QStringLiteral("WhiteCircle3");
    case SymbolType::WhiteCircle4:      return QStringLiteral("WhiteCircle4");
    }
    return QStringLiteral("MagentaCircle");
}

QStringList BTWSymbolDrawing::registeredSymbolNames()
{
    return {
        QStringLiteral("MagentaCircle"),
        QStringLiteral("MagentaCircleSynced"),
        QStringLiteral("YellowCircle1"),
        QStringLiteral("YellowCircle2"),
        QStringLiteral("YellowCircle3"),
        QStringLiteral("YellowCircle4"),
        QStringLiteral("WhiteCircle1"),
        QStringLiteral("WhiteCircle2"),
        QStringLiteral("WhiteCircle3"),
        QStringLiteral("WhiteCircle4"),
    };
}

BTWSymbolDrawing::SymbolType BTWSymbolDrawing::resolveDisplayType(const QString &symbolName, bool isSynced)
{
    // The BTW marker symbol (MagentaCircle) is always rendered as a HOLLOW circle,
    // identically on the BTW graph, other waterfall graph types, and SCW graphs.
    // The synced state no longer changes its appearance (previously it upgraded to
    // the filled MagentaCircleSynced). Filled remains available only by explicitly
    // requesting the MagentaCircleSynced symbol name/type.
    Q_UNUSED(isSynced);
    return symbolNameToType(symbolName);
}

void BTWSymbolDrawing::generateAll()
{
    cache[SymbolType::MagentaCircle] = makeMagentaCircle();
    cache[SymbolType::MagentaCircleSynced] = makeMagentaCircleSynced();
    cache[SymbolType::YellowCircle1] = makeYellowCircle1();
    cache[SymbolType::YellowCircle2] = makeYellowCircle2();
    cache[SymbolType::YellowCircle3] = makeYellowCircle3();
    cache[SymbolType::YellowCircle4] = makeYellowCircle4();
    cache[SymbolType::WhiteCircle1] = makeWhiteCircle1();
    cache[SymbolType::WhiteCircle2] = makeWhiteCircle2();
    cache[SymbolType::WhiteCircle3] = makeWhiteCircle3();
    cache[SymbolType::WhiteCircle4] = makeWhiteCircle4();
}

QPixmap BTWSymbolDrawing::blank()
{
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);
    return pix;
}

QFont BTWSymbolDrawing::makeFont()
{
    QFont font("Noto Serif");
    font.setBold(true);
    font.setPointSize(BTW_SYMBOL_FONT_SIZE);
    font.setStyleStrategy(QFont::StyleStrategy(QFont::PreferAntialias | QFont::PreferQuality));
    return font;
}

QPixmap BTWSymbolDrawing::makeMagentaCircle()
{
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QColor magentaColor(255, 0, 255);
    QPen pen(magentaColor);
    pen.setWidthF(1.5);
    pen.setCosmetic(true);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    const qreal circleSize = 10.0;
    const qreal offset = (size - circleSize) / 2.0;
    p.drawEllipse(QRectF(offset, offset, circleSize, circleSize));
    return pix;
}

QPixmap BTWSymbolDrawing::makeMagentaCircleSynced()
{
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QColor magentaColor(255, 0, 255);
    QPen pen(magentaColor);
    pen.setWidthF(1.5);
    pen.setCosmetic(true);
    p.setPen(pen);
    p.setBrush(QBrush(magentaColor));

    const qreal circleSize = 10.0;
    const qreal offset = (size - circleSize) / 2.0;
    p.drawEllipse(QRectF(offset, offset, circleSize, circleSize));
    return pix;
}

QPixmap BTWSymbolDrawing::makeFilledCircleWithDigit(const QColor &fill, const QColor &textColor, const QString &digit)
{
    QPixmap pix = blank();
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    const qreal circleSize = 14.0;
    const qreal offset = (size - circleSize) / 2.0;
    const QRectF circleRect(offset, offset, circleSize, circleSize);

    p.setPen(Qt::NoPen);
    p.setBrush(fill);
    p.drawEllipse(circleRect);

    p.setPen(textColor);
    p.setBrush(Qt::NoBrush);
    p.setFont(makeFont());
    p.drawText(circleRect, Qt::AlignCenter, digit);
    return pix;
}

QPixmap BTWSymbolDrawing::makeYellowCircle1()
{
    return makeFilledCircleWithDigit(Qt::yellow, Qt::white, QStringLiteral("1"));
}

QPixmap BTWSymbolDrawing::makeYellowCircle2()
{
    return makeFilledCircleWithDigit(Qt::yellow, Qt::white, QStringLiteral("2"));
}

QPixmap BTWSymbolDrawing::makeYellowCircle3()
{
    return makeFilledCircleWithDigit(Qt::yellow, Qt::white, QStringLiteral("3"));
}

QPixmap BTWSymbolDrawing::makeYellowCircle4()
{
    return makeFilledCircleWithDigit(Qt::yellow, Qt::white, QStringLiteral("4"));
}

QPixmap BTWSymbolDrawing::makeWhiteCircle1()
{
    return makeFilledCircleWithDigit(Qt::white, Qt::black, QStringLiteral("1"));
}

QPixmap BTWSymbolDrawing::makeWhiteCircle2()
{
    return makeFilledCircleWithDigit(Qt::white, Qt::black, QStringLiteral("2"));
}

QPixmap BTWSymbolDrawing::makeWhiteCircle3()
{
    return makeFilledCircleWithDigit(Qt::white, Qt::black, QStringLiteral("3"));
}

QPixmap BTWSymbolDrawing::makeWhiteCircle4()
{
    return makeFilledCircleWithDigit(Qt::white, Qt::black, QStringLiteral("4"));
}
