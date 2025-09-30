#include "timelinedrawingobjects.h"
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QGraphicsPolygonItem>
#include <QDebug>

// TimelineSegmentDrawer Implementation

TimelineSegmentDrawer::TimelineSegmentDrawer(int segmentNumber,
                                           const QTime& timelineLength,
                                           const QTime& currentTime,
                                           int numberOfDivisions,
                                           bool isAbsoluteTime,
                                           const QRect& drawArea)
    : m_segmentNumber(segmentNumber)
    , m_timelineLength(timelineLength)
    , m_currentTime(currentTime)
    , m_numberOfDivisions(numberOfDivisions)
    , m_isAbsoluteTime(isAbsoluteTime)
    , m_drawArea(drawArea)
    , m_smoothOffset(0.0)
    , m_segmentTime()
    , m_labelSet(false)
{
    // Calculate and store the segment time once during construction
    if (segmentNumber % 3 == 0) { // Only every 3rd segment gets a label
        // Calculate the time interval per segment
        int totalSeconds = m_timelineLength.hour() * 3600 + m_timelineLength.minute() * 60 + m_timelineLength.second();
        int segmentIntervalSeconds = totalSeconds / m_numberOfDivisions;
        
        // Calculate the segment time by subtracting segmentNumber * segmentInterval from currentTime
        m_segmentTime = m_currentTime.addSecs(-segmentNumber * segmentIntervalSeconds);
        
        // Handle case where time goes into previous day - wrap around
        if (m_segmentTime.isNull()) {
            // If addSecs resulted in invalid time, try adding to start of day
            int currentSeconds = m_currentTime.hour() * 3600 + m_currentTime.minute() * 60 + m_currentTime.second();
            int targetSeconds = currentSeconds - (segmentNumber * segmentIntervalSeconds);
            
            // Wrap around if negative
            if (targetSeconds < 0) {
                targetSeconds += 24 * 3600; // Add 24 hours
            }
            
            int hours = targetSeconds / 3600;
            int minutes = (targetSeconds % 3600) / 60;
            m_segmentTime = QTime(hours, minutes, 0);
        }
        
        m_labelSet = true;
    }
}

void TimelineSegmentDrawer::update()
{
    // Update internal state if needed
    // This could include recalculating positions, updating animations, etc.
    // For now, we'll keep it simple as the main state is managed externally
}

bool TimelineSegmentDrawer::isVisible() const
{
    if (m_numberOfDivisions <= 0 || m_drawArea.height() <= 0) {
        return false;
    }
    
    double segmentHeight = static_cast<double>(m_drawArea.height()) / m_numberOfDivisions;
    double y = getYPosition();
    
    // Only draw if the segment is at least partially visible
    return (y + segmentHeight >= 0 && y < m_drawArea.height());
}

double TimelineSegmentDrawer::getYPosition() const
{
    if (m_numberOfDivisions <= 0 || m_drawArea.height() <= 0) {
        return 0.0;
    }
    
    double segmentHeight = static_cast<double>(m_drawArea.height()) / m_numberOfDivisions;
    return m_segmentNumber * segmentHeight + m_smoothOffset;
}

void TimelineSegmentDrawer::draw(QGraphicsScene* scene)
{
    if (!scene || m_numberOfDivisions <= 0 || m_drawArea.height() <= 0) {
        return; // Invalid parameters
    }

    // Calculate segment height
    double segmentHeight = static_cast<double>(m_drawArea.height()) / m_numberOfDivisions;
    
    // Calculate Y position for this segment with smooth offset (shift down)
    double y = getYPosition();
    
    // Only show labels on every third section (0, 3, 6, 9, 12, ...)
    bool shouldShowLabel = (m_segmentNumber % 3 == 0);
    
    if (shouldShowLabel) {
        // Calculate timestamp for this segment
        QString timestamp = getTimeLabel(m_segmentNumber, m_isAbsoluteTime);
        if (timestamp.isNull()) {
            return; // No timestamp to draw
        }
        
        // Create text item for the timestamp
        QGraphicsTextItem* textItem = new QGraphicsTextItem(timestamp);
        textItem->setDefaultTextColor(QColor(255, 255, 255)); // White text
        
        // Calculate center position for the text within the segment
        QFontMetrics fm(textItem->font());
        int textWidth = fm.horizontalAdvance(timestamp);
        int textHeight = fm.height();
        
        int centerX = (m_drawArea.width() - textWidth) / 2;
        int centerY = static_cast<int>(y + segmentHeight / 2 - textHeight / 2);
        
        textItem->setPos(centerX, centerY);
        scene->addItem(textItem);
    }

    // Draw two ticks which are 15% of the segment width
    int tickWidth = static_cast<int>(m_drawArea.width() * 0.15);
    int tickY = static_cast<int>(y + segmentHeight / 2);
    
    // Left tick
    QGraphicsLineItem* leftTick = new QGraphicsLineItem(0, tickY, tickWidth, tickY);
    leftTick->setPen(QPen(QColor(255, 255, 255), 1)); // White pen
    scene->addItem(leftTick);
    
    // Right tick
    QGraphicsLineItem* rightTick = new QGraphicsLineItem(m_drawArea.width(), tickY, 
                                                         m_drawArea.width() - tickWidth, tickY);
    rightTick->setPen(QPen(QColor(255, 255, 255), 1)); // White pen
    scene->addItem(rightTick);
}

QString TimelineSegmentDrawer::getTimeLabel(int segmentNumber, bool isAbsoluteTime)
{
    QString timestamp;
    if (m_timelineLength.isNull() || m_currentTime.isNull()) {
        return timestamp;
    }

    if (isAbsoluteTime) {
        // Calculate the time interval per segment
        int totalSeconds = m_timelineLength.hour() * 3600 + m_timelineLength.minute() * 60 + m_timelineLength.second();
        int segmentIntervalSeconds = totalSeconds / m_numberOfDivisions;
        
        // Subtract segmentNumber * segmentInterval from currentTime and set label in HH:MM format
        QTime segmentTime = m_currentTime.addSecs(-segmentNumber * segmentIntervalSeconds);
        
        // Handle case where time goes into previous day - wrap around
        if (segmentTime.isNull()) {
            // If addSecs resulted in invalid time, try adding to start of day
            int currentSeconds = m_currentTime.hour() * 3600 + m_currentTime.minute() * 60 + m_currentTime.second();
            int targetSeconds = currentSeconds - (segmentNumber * segmentIntervalSeconds);
            
            // Wrap around if negative
            if (targetSeconds < 0) {
                targetSeconds += 24 * 3600; // Add 24 hours
            }
            
            int hours = targetSeconds / 3600;
            int minutes = (targetSeconds % 3600) / 60;
            segmentTime = QTime(hours, minutes, 0);
        }
        
        timestamp = segmentTime.toString("HH:mm");
    }
    else {
        // Calculate the time interval per segment
        int totalSeconds = m_timelineLength.hour() * 3600 + m_timelineLength.minute() * 60 + m_timelineLength.second();
        int segmentIntervalSeconds = totalSeconds / m_numberOfDivisions;

        // The difference from current time to this segment is segmentNumber * segmentIntervalSeconds
        int diffSeconds = segmentNumber * segmentIntervalSeconds;

        int diffHours = diffSeconds / 3600;
        int diffMinutes = (diffSeconds % 3600) / 60;

        // Format as "-HH:MM"
        timestamp = QString("-%1:%2")
                        .arg(diffHours, 2, 10, QChar('0'))
                        .arg(diffMinutes, 2, 10, QChar('0'));
    }
    return timestamp;
}

QString TimelineSegmentDrawer::getDisplayLabel() const
{
    if (!m_labelSet) {
        return QString();
    }
    
    if (m_showRelativeLabel) {
        // Calculate relative label based on current time difference
        int currentSeconds = m_currentTime.hour() * 3600 + m_currentTime.minute() * 60 + m_currentTime.second();
        int segmentSeconds = m_segmentTime.hour() * 3600 + m_segmentTime.minute() * 60 + m_segmentTime.second();
        
        int diffSeconds = segmentSeconds - currentSeconds;
        int diffHours = diffSeconds / 3600;
        int diffMinutes = (diffSeconds % 3600) / 60;
        
        // Format as "-HH:MM" or "+HH:MM"
        if (diffSeconds >= 0) {
            return QString("+%1:%2")
                        .arg(diffHours, 2, 10, QChar('0'))
                        .arg(diffMinutes, 2, 10, QChar('0'));
        } else {
            return QString("-%1:%2")
                        .arg(-diffHours, 2, 10, QChar('0'))
                        .arg(-diffMinutes, 2, 10, QChar('0'));
        }
    } else {
        // Return absolute time
        return m_segmentTime.toString("HH:mm");
    }
}

// TimelineChevronDrawer Implementation

TimelineChevronDrawer::TimelineChevronDrawer(const QRect& drawArea, int yOffset)
    : m_drawArea(drawArea)
    , m_yOffset(yOffset)
    , m_chevronWidthPercent(0.4) // 40% of widget width
    , m_chevronHeight(8)
    , m_chevronBoxHeight(30)
{
}

void TimelineChevronDrawer::update()
{
    // Update internal state if needed
    // This could include recalculating positions, updating animations, etc.
    // For now, we'll keep it simple as the main state is managed externally
}

void TimelineChevronDrawer::draw(QGraphicsScene* scene)
{
    if (!scene) {
        return;
    }

    int widgetWidth = m_drawArea.width();
    
    // Define chevron size (width and height)
    int chevronWidth = static_cast<int>(widgetWidth * m_chevronWidthPercent);
    
    // Calculate chevron position (centered horizontally)
    int chevronX = (widgetWidth - chevronWidth) / 2;
    int chevronY = m_yOffset;
    
    // Calculate the tip position (bottom center of V)
    int tipX = chevronX + chevronWidth / 2;
    int tipY = chevronY + m_chevronHeight;
    
    // Define chevron points (pointing down: V) and the lines to the edges
    QPolygonF chevronPolygon;
    chevronPolygon << QPointF(0, chevronY - m_chevronBoxHeight)           // Start point
                   << QPointF(0, chevronY)                               // Left edge
                   << QPointF(chevronX, chevronY)                        // Top left point
                   << QPointF(tipX, tipY)                               // Bottom point (tip)
                   << QPointF(chevronX + chevronWidth, chevronY)        // Top right point
                   << QPointF(widgetWidth, chevronY)                    // Right edge
                   << QPointF(widgetWidth, chevronY - m_chevronBoxHeight) // Right edge
                   << QPointF(0, chevronY - m_chevronBoxHeight);        // Start point

    // Create polygon item for the chevron
    QGraphicsPolygonItem* chevronItem = new QGraphicsPolygonItem(chevronPolygon);
    chevronItem->setPen(QPen(QColor(0, 100, 255), 2)); // Blue color, 2px width
    scene->addItem(chevronItem);

    // Draw the 3 labels inside the chevron
    drawChevronLabels(scene);
}

void TimelineChevronDrawer::drawChevronLabels(QGraphicsScene* scene)
{
    int widgetWidth = m_drawArea.width();
    int chevronWidth = static_cast<int>(widgetWidth * m_chevronWidthPercent);
    int chevronX = (widgetWidth - chevronWidth) / 2;
    int chevronY = m_yOffset;

    // Create text items for the 3 labels
    QGraphicsTextItem* label1 = new QGraphicsTextItem("1");
    label1->setDefaultTextColor(QColor(255, 255, 255)); // White text
    label1->setPos(chevronX, chevronY);
    scene->addItem(label1);

    QGraphicsTextItem* label2 = new QGraphicsTextItem("2");
    label2->setDefaultTextColor(QColor(255, 255, 255)); // White text
    label2->setPos(chevronX + chevronWidth / 2, chevronY);
    scene->addItem(label2);

    QGraphicsTextItem* label3 = new QGraphicsTextItem("3");
    label3->setDefaultTextColor(QColor(255, 255, 255)); // White text
    label3->setPos(chevronX + chevronWidth, chevronY);
    scene->addItem(label3);
}

// TimelineBackgroundDrawer Implementation

TimelineBackgroundDrawer::TimelineBackgroundDrawer(const QRect& drawArea, const QColor& backgroundColor)
    : m_drawArea(drawArea)
    , m_backgroundColor(backgroundColor)
    , m_borderColor(QColor(150, 150, 150))
    , m_borderWidth(1)
{
}

void TimelineBackgroundDrawer::update()
{
    // Update internal state if needed
    // This could include recalculating positions, updating animations, etc.
    // For now, we'll keep it simple as the main state is managed externally
}

void TimelineBackgroundDrawer::draw(QGraphicsScene* scene)
{
    if (!scene) {
        return;
    }

    // Create background rectangle
    QGraphicsRectItem* backgroundItem = new QGraphicsRectItem(m_drawArea);
    backgroundItem->setBrush(QBrush(m_backgroundColor));
    backgroundItem->setPen(Qt::NoPen); // No border for background
    scene->addItem(backgroundItem);

    // Create border rectangle
    QGraphicsRectItem* borderItem = new QGraphicsRectItem(m_drawArea.adjusted(0, 0, -1, -1));
    borderItem->setPen(QPen(m_borderColor, m_borderWidth));
    borderItem->setBrush(Qt::NoBrush); // No fill for border
    scene->addItem(borderItem);
}
