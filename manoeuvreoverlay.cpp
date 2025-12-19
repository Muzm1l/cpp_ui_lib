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
      m_manoeuvres(nullptr),
      m_hasInProgress(false)
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
    
    if (!m_minTime.isValid() || !m_maxTime.isValid())
    {
        return;
    }
    
    // Draw in-progress manoeuvre (just start line) if any
    if (m_hasInProgress && m_inProgressStartTime.isValid())
    {
        if (m_inProgressStartTime <= m_maxTime && m_inProgressStartTime >= m_minTime)
        {
            drawInProgressStartLine();
        }
    }
    
    // Draw completed manoeuvres
    if (m_manoeuvres && !m_manoeuvres->empty())
    {
        for (const auto &manoeuvre : *m_manoeuvres)
        {
            // Only draw if manoeuvre overlaps with visible time range
            if (manoeuvre.startTime <= m_maxTime && manoeuvre.endTime >= m_minTime)
            {
                drawManoeuvre(manoeuvre);
            }
        }
    }
}

void ManoeuvreOverlay::setInProgressManoeuvre(const QDateTime &startTime)
{
    m_hasInProgress = true;
    m_inProgressStartTime = startTime;
    updateOverlay();
}

void ManoeuvreOverlay::clearInProgressManoeuvre()
{
    m_hasInProgress = false;
    m_inProgressStartTime = QDateTime();
    updateOverlay();
}

void ManoeuvreOverlay::drawInProgressStartLine()
{
    if (rect().width() <= 0 || rect().height() <= 0)
    {
        return;
    }
    
    qreal startY = timeToY(m_inProgressStartTime);
    int widgetWidth = rect().width();
    
    // Cyan color
    QColor cyanColor(0, 255, 255);
    
    // Draw dashed horizontal line at start time
    QGraphicsLineItem *startLineItem = new QGraphicsLineItem(0, startY, widgetWidth, startY);
    QPen dashedPen(cyanColor, 3, Qt::DashLine);
    startLineItem->setPen(dashedPen);
    m_scene->addItem(startLineItem);
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
    
    // Cyan color for all drawing
    QColor cyanColor(0, 255, 255);
    
    // === START: Dashed horizontal line ===
    QGraphicsLineItem *startLineItem = new QGraphicsLineItem(0, startY, widgetWidth, startY);
    QPen dashedPen(cyanColor, 3, Qt::DashLine);
    startLineItem->setPen(dashedPen);
    m_scene->addItem(startLineItem);
    
    // === END: Horizontal line with V pointing DOWN ===
    // V parameters
    double vWidthPercent = 0.4; // 40% of widget width
    int vHeight = 8;
    
    int vWidth = static_cast<int>(widgetWidth * vWidthPercent);
    int vX = (widgetWidth - vWidth) / 2;
    int tipX = vX + vWidth / 2;
    
    // V tip points DOWN (below endY line)
    qreal vTipY = endY + vHeight;
    
    // Draw horizontal line at end time (left side)
    QGraphicsLineItem *endLineLeft = new QGraphicsLineItem(0, endY, vX, endY);
    endLineLeft->setPen(QPen(cyanColor, 3));
    m_scene->addItem(endLineLeft);
    
    // Draw horizontal line at end time (right side)
    QGraphicsLineItem *endLineRight = new QGraphicsLineItem(vX + vWidth, endY, widgetWidth, endY);
    endLineRight->setPen(QPen(cyanColor, 3));
    m_scene->addItem(endLineRight);
    
    // Draw V shape pointing down
    // Left edge of V: from (vX, endY) to (tipX, vTipY)
    QGraphicsLineItem *vLeft = new QGraphicsLineItem(vX, endY, tipX, vTipY);
    vLeft->setPen(QPen(cyanColor, 3));
    m_scene->addItem(vLeft);
    
    // Right edge of V: from (vX + vWidth, endY) to (tipX, vTipY)
    QGraphicsLineItem *vRight = new QGraphicsLineItem(vX + vWidth, endY, tipX, vTipY);
    vRight->setPen(QPen(cyanColor, 3));
    m_scene->addItem(vRight);
    
    // Draw text labels - font size 14 pixels
    QFont labelFont;
    labelFont.setPixelSize(14);
    labelFont.setBold(false);
    QFontMetrics fm(labelFont);
    int fontHeight = fm.height();
    
    // Speed: Above the V (centered above the horizontal line)
    QString speedText = QString::number(manoeuvre.speed);
    QGraphicsTextItem *speedLabel = new QGraphicsTextItem(speedText);
    speedLabel->setFont(labelFont);
    speedLabel->setDefaultTextColor(cyanColor);
    int speedWidth = fm.horizontalAdvance(speedText);
    int speedX = tipX - speedWidth / 2;
    int speedY = endY - fontHeight - 2;
    speedLabel->setPos(speedX, speedY);
    m_scene->addItem(speedLabel);
    
    // Course (Bearing): Bottom left of V
    QString bearingText = QString::number(manoeuvre.bearing);
    QGraphicsTextItem *bearingLabel = new QGraphicsTextItem(bearingText);
    bearingLabel->setFont(labelFont);
    bearingLabel->setDefaultTextColor(cyanColor);
    int bearingWidth = fm.horizontalAdvance(bearingText);
    int bearingX = vX - bearingWidth - 5;
    int bearingY = vTipY + 2;
    bearingLabel->setPos(bearingX, bearingY);
    m_scene->addItem(bearingLabel);
    
    // Depth: Bottom right of V
    QString depthText = QString::number(manoeuvre.depth);
    QGraphicsTextItem *depthLabel = new QGraphicsTextItem(depthText);
    depthLabel->setFont(labelFont);
    depthLabel->setDefaultTextColor(cyanColor);
    int depthX = vX + vWidth + 3;
    int depthY = vTipY + 2;
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

