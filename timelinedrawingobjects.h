#ifndef TIMELINEDRAWINGOBJECTS_H
#define TIMELINEDRAWINGOBJECTS_H

#include <QGraphicsScene>
#include <QPainter>
#include <QTime>
#include <QPoint>
#include <QRect>
#include <QFontMetrics>
#include <QPen>
#include <QColor>
#include "timelineutils.h"

/**
 * Base class for timeline drawing objects
 */
class TimelineDrawingObject
{
public:
    virtual ~TimelineDrawingObject() = default;
    virtual void update() = 0;
    virtual void draw(QGraphicsScene* scene) = 0;
    virtual void setPosition(const QPoint& position) { m_position = position; }
    virtual QPoint getPosition() const { return m_position; }

protected:
    QPoint m_position;
};

/**
 * Draws a timeline segment with ticks and labels
 */
class TimelineSegmentDrawer : public TimelineDrawingObject
{
public:
    TimelineSegmentDrawer(int segmentNumber, 
                          const QTime& timelineLength,
                          const QTime& currentTime,
                          int numberOfDivisions,
                          bool isAbsoluteTime,
                          const QRect& drawArea);

    void update() override;
    void draw(QGraphicsScene* scene) override;
    void setSegmentNumber(int segmentNumber) { m_segmentNumber = segmentNumber; }
    void setTimelineLength(const QTime& timelineLength) { m_timelineLength = timelineLength; }
    void setCurrentTime(const QTime& currentTime) { m_currentTime = currentTime; }
    void setNumberOfDivisions(int divisions) { m_numberOfDivisions = divisions; }
    void setIsAbsoluteTime(bool isAbsoluteTime) { m_isAbsoluteTime = isAbsoluteTime; }
    void setDrawArea(const QRect& drawArea) { m_drawArea = drawArea; }
    void setSmoothOffset(double offset) { m_smoothOffset = offset; }

    int getSegmentNumber() const { return m_segmentNumber; }
    QTime getTimelineLength() const { return m_timelineLength; }
    QTime getCurrentTime() const { return m_currentTime; }
    int getNumberOfDivisions() const { return m_numberOfDivisions; }
    bool getIsAbsoluteTime() const { return m_isAbsoluteTime; }
    QRect getDrawArea() const { return m_drawArea; }
    double getSmoothOffset() const { return m_smoothOffset; }
    
    // Animation and visibility methods
    bool isVisible() const;
    double getYPosition() const;
    
    // Time label calculation
    QString getTimeLabel(int segmentNumber, bool isAbsoluteTime);
    
    // Get fixed label (set once, never changes)
    QString getFixedLabel() const { return getDisplayLabel(); }
    bool isLabelSet() const { return m_labelSet; }
    
    // Mutable label mode control
    mutable bool m_showRelativeLabel = false;
    void setShowRelativeLabel(bool showRelative) const { m_showRelativeLabel = showRelative; }
    bool getShowRelativeLabel() const { return m_showRelativeLabel; }
    
    // Get the display label (absolute or relative based on mode)
    QString getDisplayLabel() const;
    
    // Get the stored segment time
    QTime getSegmentTime() const { return m_segmentTime; }

private:
    int m_segmentNumber;
    QTime m_timelineLength;
    QTime m_currentTime;
    int m_numberOfDivisions;
    bool m_isAbsoluteTime;
    QRect m_drawArea;
    double m_smoothOffset;
    
    // Fixed label values - set once and never updated
    QTime m_segmentTime;  // Store the actual time for this segment
    bool m_labelSet;

    void drawSegment(QPainter& painter);
    void drawTicks(QPainter& painter, double y, int tickWidth);
    void drawLabel(QPainter& painter, double y, const QString& timestamp);
};

/**
 * Draws a chevron element with labels
 */
class TimelineChevronDrawer : public TimelineDrawingObject
{
public:
    TimelineChevronDrawer(const QRect& drawArea, int yOffset = 50);

    void update() override;
    void draw(QGraphicsScene* scene) override;
    void setDrawArea(const QRect& drawArea) { m_drawArea = drawArea; }
    void setYOffset(int yOffset) { m_yOffset = yOffset; }
    void setChevronWidth(double widthPercent) { m_chevronWidthPercent = widthPercent; }
    void setChevronHeight(int height) { m_chevronHeight = height; }
    void setChevronBoxHeight(int height) { m_chevronBoxHeight = height; }

    QRect getDrawArea() const { return m_drawArea; }
    int getYOffset() const { return m_yOffset; }
    double getChevronWidthPercent() const { return m_chevronWidthPercent; }
    int getChevronHeight() const { return m_chevronHeight; }
    int getChevronBoxHeight() const { return m_chevronBoxHeight; }

private:
    QRect m_drawArea;
    int m_yOffset;
    double m_chevronWidthPercent; // Percentage of widget width
    int m_chevronHeight;
    int m_chevronBoxHeight;

    void drawChevron(QPainter& painter);
    void drawChevronLabels(QGraphicsScene* scene);
};

/**
 * Draws the timeline background
 */
class TimelineBackgroundDrawer : public TimelineDrawingObject
{
public:
    TimelineBackgroundDrawer(const QRect& drawArea, const QColor& backgroundColor = QColor(0, 0, 0));

    void update() override;
    void draw(QGraphicsScene* scene) override;
    void setDrawArea(const QRect& drawArea) { m_drawArea = drawArea; }
    void setBackgroundColor(const QColor& color) { m_backgroundColor = color; }
    void setBorderColor(const QColor& color) { m_borderColor = color; }
    void setBorderWidth(int width) { m_borderWidth = width; }

    QRect getDrawArea() const { return m_drawArea; }
    QColor getBackgroundColor() const { return m_backgroundColor; }
    QColor getBorderColor() const { return m_borderColor; }
    int getBorderWidth() const { return m_borderWidth; }

private:
    QRect m_drawArea;
    QColor m_backgroundColor;
    QColor m_borderColor;
    int m_borderWidth;

    void drawBackground(QPainter& painter);
    void drawBorder(QPainter& painter);
};

#endif // TIMELINEDRAWINGOBJECTS_H
