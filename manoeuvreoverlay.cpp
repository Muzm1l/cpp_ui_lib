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
    
    // Draw plain horizontal line at START time (manoeuvre start)
    QGraphicsLineItem *startLineItem = new QGraphicsLineItem(0, startY, widgetWidth, startY);
    startLineItem->setPen(QPen(QColor(0, 100, 255), 3)); // Blue color, 3px width
    m_scene->addItem(startLineItem);
    
    // Chevron parameters
    double chevronWidthPercent = 0.4; // 40% of widget width
    int chevronHeight = 8;
    int chevronBoxHeight = 30;
    
    int chevronWidth = static_cast<int>(widgetWidth * chevronWidthPercent);
    int chevronX = (widgetWidth - chevronWidth) / 2;
    
    // Chevron is at the TOP (endTime) - marks manoeuvre end
    // Chevron tip Y position (top point of V, pointing up)
    qreal chevronTipY = endY;
    
    // Chevron box top Y position (where V connects to box)
    qreal chevronBoxTopY = endY + chevronHeight;
    
    // Chevron box bottom Y position (bottom of the box)
    qreal chevronBoxBottomY = chevronBoxTopY + chevronBoxHeight;
    
    // Calculate tip X position (center of chevron)
    int tipX = chevronX + chevronWidth / 2;
    
    // Draw chevron polygon at END time:
    // V shape at top pointing up to end time, box below
    QPolygonF chevronPolygon;
    chevronPolygon << QPointF(0, chevronBoxBottomY)                        // 1. Bottom left of box
                   << QPointF(widgetWidth, chevronBoxBottomY)               // 2. Bottom right of box
                   << QPointF(widgetWidth, chevronBoxTopY)                  // 3. Top right of box
                   << QPointF(chevronX + chevronWidth, chevronBoxTopY)      // 4. V right point
                   << QPointF(tipX, chevronTipY)                            // 5. V tip (pointing up)
                   << QPointF(chevronX, chevronBoxTopY)                     // 6. V left point
                   << QPointF(0, chevronBoxTopY);                           // 7. Top left of box
    
    // Create and add chevron polygon item
    QGraphicsPolygonItem *chevronItem = new QGraphicsPolygonItem(chevronPolygon);
    chevronItem->setPen(QPen(QColor(0, 100, 255), 3)); // Blue color, 3px width
    chevronItem->setBrush(Qt::NoBrush); // No fill, just outline
    m_scene->addItem(chevronItem);
    
    // Draw text labels on the chevron (bearing, speed, depth)
    QFont labelFont;
    labelFont.setPixelSize(chevronHeight - 1); // 7 pixels
    labelFont.setBold(false);
    QFontMetrics fm(labelFont);
    
    // Speed: Inside the chevron box
    QString speedText = QString::number(manoeuvre.speed);
    QGraphicsTextItem *speedLabel = new QGraphicsTextItem(speedText);
    speedLabel->setFont(labelFont);
    speedLabel->setDefaultTextColor(QColor(0, 100, 255));
    int speedWidth = fm.horizontalAdvance(speedText);
    int speedX = tipX - speedWidth / 2;
    int speedY = chevronBoxTopY + 8;
    speedLabel->setPos(speedX, speedY);
    m_scene->addItem(speedLabel);
    
    // Bearing: Above chevron tip left
    QString bearingText = QString::number(manoeuvre.bearing);
    QGraphicsTextItem *bearingLabel = new QGraphicsTextItem(bearingText);
    bearingLabel->setFont(labelFont);
    bearingLabel->setDefaultTextColor(QColor(0, 100, 255));
    int bearingWidth = fm.horizontalAdvance(bearingText);
    int bearingX = chevronX - bearingWidth / 2;
    int bearingY = chevronTipY - 15;
    bearingLabel->setPos(bearingX, bearingY);
    m_scene->addItem(bearingLabel);
    
    // Depth: Above chevron tip right
    QString depthText = QString::number(manoeuvre.depth);
    QGraphicsTextItem *depthLabel = new QGraphicsTextItem(depthText);
    depthLabel->setFont(labelFont);
    depthLabel->setDefaultTextColor(QColor(0, 100, 255));
    int depthWidth = fm.horizontalAdvance(depthText);
    int depthX = (chevronX + chevronWidth) - depthWidth / 2;
    int depthY = chevronTipY - 15;
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

