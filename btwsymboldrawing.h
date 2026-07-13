#ifndef BTWSYMBOLDRAWING_H
#define BTWSYMBOLDRAWING_H

#include <QPainter>
#include <QPixmap>
#include <QMap>
#include <QString>
#include <QStringList>

class BTWSymbolDrawing
{
public:
    enum class SymbolType {
        MagentaCircle,       // Hollow magenta circle (manual-marker sync, default)
        MagentaCircleSynced, // Filled magenta circle (synced across graphs)
        YellowCircle1,       // Yellow circle, white "1"
        YellowCircle2,       // Yellow circle, white "2"
        YellowCircle3,       // Yellow circle, white "3"
        YellowCircle4,       // Yellow circle, white "4"
        WhiteCircle1,        // White circle, black "1"
        WhiteCircle2,        // White circle, black "2"
        WhiteCircle3,        // White circle, black "3"
        WhiteCircle4         // White circle, black "4"
    };

    BTWSymbolDrawing(int baseSize = 40);

    void draw(QPainter* p, QPointF pos, SymbolType type);
    const QPixmap& get(SymbolType type) const;

    /** Map API / storage name to cached symbol type (case-insensitive). */
    static SymbolType symbolNameToType(const QString &symbolName);
    /** Canonical storage name for a symbol type. */
    static QString symbolTypeToName(SymbolType type);
    /** All names accepted by addBTWSymbol APIs. */
    static QStringList registeredSymbolNames();
    /** Resolve stored symbol name to the type to draw. MagentaCircle always draws
     *  hollow regardless of synced state (isSynced retained for API compatibility). */
    static SymbolType resolveDisplayType(const QString &symbolName, bool isSynced);

private:
    int size;
    QMap<SymbolType, QPixmap> cache;

    void generateAll();

    QPixmap makeMagentaCircle();
    QPixmap makeMagentaCircleSynced();
    QPixmap makeYellowCircle1();
    QPixmap makeYellowCircle2();
    QPixmap makeYellowCircle3();
    QPixmap makeYellowCircle4();
    QPixmap makeWhiteCircle1();
    QPixmap makeWhiteCircle2();
    QPixmap makeWhiteCircle3();
    QPixmap makeWhiteCircle4();

    QPixmap makeFilledCircleWithDigit(const QColor &fill, const QColor &textColor, const QString &digit);

    QPixmap blank();
    QFont makeFont();
};

#endif
