#include "manoeuvreoverlay.h"
#include <QGraphicsLineItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsTextItem>
#include <QResizeEvent>
#include <QFrame>
#include <QFont>
#include <QDebug>

ManoeuvreOverlay::ManoeuvreOverlay(QWidget *parent)
    : QGraphicsView(parent),
      m_scene(new QGraphicsScene(this)),
      m_manoeuvres(nullptr)
{
    // Set transparent background
    setStyleSheet("background: transparent;");
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    
    // Set scene
    setScene(m_scene);
    
    // Set viewport to transparent
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setRenderHint(QPainter::Antialiasing);
    
    // Remove scrollbars
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    // Set frame style to no frame
    setFrameShape(QFrame::NoFrame);
}

ManoeuvreOverlay::~ManoeuvreOverlay()
{
    clearScene();
}

void ManoeuvreOverlay::setManoeuvres(const std::vector<Manoeuvre> *manoeuvres)
{
    m_manoeuvres = manoeuvres;
    updateOverlay();
}

void ManoeuvreOverlay::setTimeRange(const QDateTime &minTime, const QDateTime &maxTime)
{
    m_minTime = minTime;
    m_maxTime = maxTime;
    updateOverlay();
}

void ManoeuvreOverlay::updateOverlay()
{
    clearScene();
    
    if (!m_manoeuvres || m_manoeuvres->empty())
    {
        return;
    }
    
    if (!m_minTime.isValid() || !m_maxTime.isValid())
    {
        return;
    }
    
    // Draw each manoeuvre
    for (const auto &manoeuvre : *m_manoeuvres)
    {
        // Only draw if manoeuvre overlaps with visible time range
        if (manoeuvre.startTime <= m_maxTime && manoeuvre.endTime >= m_minTime)
        {
            drawManoeuvre(manoeuvre);
        }
    }
}

qreal ManoeuvreOverlay::timeToY(const QDateTime &time) const
{
    if (!time.isValid() || !m_minTime.isValid() || !m_maxTime.isValid())
    {
        return 0.0;
    }
    
    if (rect().height() <= 0)
    {
        return 0.0;
    }
    
    // Calculate total window duration
    qint64 totalWindowMs = m_minTime.msecsTo(m_maxTime);
    if (totalWindowMs <= 0)
    {
        return 0.0;
    }
    
    // Calculate how far the time is from maxTime (top)
    // Top (Y=0) = maxTime (newer), Bottom (Y=height) = minTime (older)
    qint64 timeFromMaxMs = time.msecsTo(m_maxTime);
    
    // Normalize: 0.0 at top (maxTime), 1.0 at bottom (minTime)
    qreal normalizedY = static_cast<qreal>(timeFromMaxMs) / static_cast<qreal>(totalWindowMs);
    normalizedY = qMax(0.0, qMin(1.0, normalizedY));
    
    // Map to widget height: top (maxTime) is Y=0, bottom (minTime) is Y=height
    return normalizedY * rect().height();
}

void ManoeuvreOverlay::drawManoeuvre(const Manoeuvre &manoeuvre)
{
    if (rect().width() <= 0 || rect().height() <= 0)
    {
        return;
    }
    
    // Calculate Y positions for start and end times
    // Top (Y=0) = maxTime (newer), Bottom (Y=height) = minTime (older)
    qreal startY = timeToY(manoeuvre.startTime); // Start time = bottom (larger Y value)
    qreal endY = timeToY(manoeuvre.endTime);       // End time = top (smaller Y value)
    
    // Ensure startY is at bottom (larger Y) and endY is at top (smaller Y)
    if (startY < endY)
    {
        std::swap(startY, endY);
    }
    
    int widgetWidth = rect().width();
    
    // Chevron parameters (matching existing chevron style)
    double chevronWidthPercent = 0.4; // 40% of widget width
    int chevronHeight = 8;
    int chevronBoxHeight = 30;
    
    int chevronWidth = static_cast<int>(widgetWidth * chevronWidthPercent);
    int chevronX = (widgetWidth - chevronWidth) / 2;
    
    // Chevron is at the BOTTOM (startTime)
    // Chevron tip Y position (bottom point of V, pointing down)
    qreal chevronTipY = startY;
    
    // Chevron box bottom Y position (where V connects to box)
    qreal chevronBoxBottomY = startY - chevronHeight;
    
    // Chevron box top Y position (top of the box)
    qreal chevronBoxTopY = chevronBoxBottomY - chevronBoxHeight;
    
    // Calculate tip X position (center of chevron)
    int tipX = chevronX + chevronWidth / 2;
    
    // Draw chevron polygon matching the documentation diagram:
    // Box at top, V shape at bottom pointing down to start time
    QPolygonF chevronPolygon;
    chevronPolygon << QPointF(0, endY)                            // 1. Top left of box
                   << QPointF(widgetWidth, endY)                   // 2. Top right of box
                   << QPointF(widgetWidth, chevronBoxBottomY)                 // 3. Bottom right of box (right edge where V connects)
                   << QPointF(chevronX + chevronWidth, chevronBoxBottomY)     // 4. V right point (top of right V edge)
                   << QPointF(tipX, chevronTipY)                             // 5. V tip (bottom point, pointing down)
                   << QPointF(chevronX, chevronBoxBottomY)                   // 6. V left point (top of left V edge)
                   << QPointF(0, chevronBoxBottomY);                        // 7. Bottom left of box (left edge where V connects)
    
    // Create and add chevron polygon item
    QGraphicsPolygonItem *chevronItem = new QGraphicsPolygonItem(chevronPolygon);
    chevronItem->setPen(QPen(QColor(0, 100, 255), 3)); // Blue color, 3px width (matching existing style)
    chevronItem->setBrush(Qt::NoBrush); // No fill, just outline
    m_scene->addItem(chevronItem);
    
    // // Draw horizontal line at end time (at the top)
    // if (endY < chevronBoxTopY)
    // {
    //     // Draw horizontal line at endY position
    //     QGraphicsLineItem *lineItem = new QGraphicsLineItem(0, endY, widgetWidth, endY);
    //     lineItem->setPen(QPen(QColor(0, 100, 255), 3)); // Blue color, 3px width (matching chevron)
    //     m_scene->addItem(lineItem);
    // }
    
    // Draw text labels on the chevron (bearing, speed, depth) with font size matching chevron height
    QFont labelFont;
    // Set font size to match chevron height (8 pixels)
    // Use pixel size slightly smaller than chevron height to ensure it fits nicely
    labelFont.setPixelSize(chevronHeight - 1); // 7 pixels to fit within 8-pixel chevron height
    labelFont.setBold(false);
    QFontMetrics fm(labelFont);
    
    // Speed: A little above the bottom of the chevron box in the middle
    QString speedText = QString::number(manoeuvre.speed);
    QGraphicsTextItem *speedLabel = new QGraphicsTextItem(speedText);
    speedLabel->setFont(labelFont);
    speedLabel->setDefaultTextColor(QColor(0, 100, 255)); // Blue text to match chevron
    int speedWidth = fm.horizontalAdvance(speedText);
    int speedX = tipX - speedWidth / 2; // Center horizontally at chevron tip
    int speedY = chevronBoxBottomY - 8; // A little above the bottom of the chevron box
    speedLabel->setPos(speedX, speedY);
    m_scene->addItem(speedLabel);
    
    // Bearing: Bottom left of the chevron
    QString bearingText = QString::number(manoeuvre.bearing);
    QGraphicsTextItem *bearingLabel = new QGraphicsTextItem(bearingText);
    bearingLabel->setFont(labelFont);
    bearingLabel->setDefaultTextColor(QColor(0, 100, 255)); // Blue text
    int bearingWidth = fm.horizontalAdvance(bearingText);
    int bearingX = chevronX - bearingWidth / 2; // Center at left edge of chevron
    int bearingY = chevronTipY + 5; // Below the chevron tip (bottom)
    bearingLabel->setPos(bearingX, bearingY);
    m_scene->addItem(bearingLabel);
    
    // Depth: Bottom right of the chevron
    QString depthText = QString::number(manoeuvre.depth);
    QGraphicsTextItem *depthLabel = new QGraphicsTextItem(depthText);
    depthLabel->setFont(labelFont);
    depthLabel->setDefaultTextColor(QColor(0, 100, 255)); // Blue text
    int depthWidth = fm.horizontalAdvance(depthText);
    int depthX = (chevronX + chevronWidth) - depthWidth / 2; // Center at right edge of chevron
    int depthY = chevronTipY + 5; // Below the chevron tip (bottom)
    depthLabel->setPos(depthX, depthY);
    m_scene->addItem(depthLabel);
}

void ManoeuvreOverlay::clearScene()
{
    if (m_scene)
    {
        m_scene->clear();
    }
}

void ManoeuvreOverlay::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    
    // Update scene rect to match view size
    if (m_scene)
    {
        m_scene->setSceneRect(0, 0, event->size().width(), event->size().height());
    }
    
    // Redraw manoeuvres with new size
    updateOverlay();
}

