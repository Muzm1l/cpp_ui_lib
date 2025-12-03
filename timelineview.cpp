#include "timelineview.h"
#include "navtimeutils.h"
#include <QBrush>
#include <QDebug>
#include <QFrame>
#include <QGraphicsView>
#include <algorithm>
#include <QPair>
#include <cmath>

// ============================================================================
// SliderGeometry Implementation
// ============================================================================

const int SliderGeometry::TWELVE_HOURS_IN_MINUTES = 720;
const int SliderGeometry::MINIMUM_SLIDER_HEIGHT = 20;

int SliderGeometry::getMinimumSliderHeight()
{
    return MINIMUM_SLIDER_HEIGHT;
}

int SliderGeometry::calculateSliderHeight(const QTime& timeInterval, int widgetHeight)
{
    int intervalMinutes = timeInterval.hour() * 60 + timeInterval.minute();
    double rectangleHeightRatio = static_cast<double>(intervalMinutes) / static_cast<double>(TWELVE_HOURS_IN_MINUTES);
    int rectangleHeight = static_cast<int>(rectangleHeightRatio * widgetHeight);
    return qMax(rectangleHeight, MINIMUM_SLIDER_HEIGHT);
}

QRect SliderGeometry::calculateSliderRect(int widgetHeight, int widgetWidth,
                                         const QTime& timeInterval,
                                         int sliderYPosition)
{
    int sliderHeight = calculateSliderHeight(timeInterval, widgetHeight);
    QPair<int, int> bounds = getSliderBounds(widgetHeight, sliderHeight);
    
    // Clamp Y position to bounds
    int clampedY = qBound(bounds.first, sliderYPosition, bounds.second);
    
    return QRect(0, clampedY, widgetWidth, sliderHeight);
}

QPair<int, int> SliderGeometry::getSliderBounds(int widgetHeight, int sliderHeight)
{
    int minY = 0;
    int maxY = widgetHeight - sliderHeight;
    return QPair<int, int>(minY, maxY);
}

int SliderGeometry::calculateSliderYFromTime(const TimeSelectionSpan& timeWindow,
                                            int widgetHeight)
{
    QDateTime now = QDateTime::currentDateTime();
    QDateTime twelveHoursAgo = now.addSecs(-12 * 3600);
    int minutesFromStart = twelveHoursAgo.msecsTo(timeWindow.startTime) / 60000;
    minutesFromStart = qBound(0, minutesFromStart, TWELVE_HOURS_IN_MINUTES);
    double positionRatio = static_cast<double>(minutesFromStart) / static_cast<double>(TWELVE_HOURS_IN_MINUTES);
    return static_cast<int>(positionRatio * widgetHeight);
}

TimeSelectionSpan SliderGeometry::calculateTimeWindowFromY(int sliderY,
                                                           const QTime& timeInterval,
                                                           int widgetHeight)
{
    QDateTime now = QDateTime::currentDateTime();
    QDateTime twelveHoursAgo = now.addSecs(-12 * 3600);
    
    double positionRatio = static_cast<double>(sliderY) / static_cast<double>(widgetHeight);
    int minutesFromStart = static_cast<int>(positionRatio * TWELVE_HOURS_IN_MINUTES);
    QDateTime windowStart = twelveHoursAgo.addSecs(minutesFromStart * 60);
    
    int intervalSeconds = timeInterval.hour() * 3600 + timeInterval.minute() * 60 + timeInterval.second();
    QDateTime windowEnd = windowStart.addSecs(intervalSeconds);
    
    return TimeSelectionSpan(windowStart, windowEnd);
}

// ============================================================================
// SliderState Implementation
// ============================================================================

SliderState::SliderState()
    : m_yPosition(0), m_isDragging(false), m_dragStartSliderY(0)
{
    // Initialize with default time window (will be set by widget)
    QDateTime now = QDateTime::currentDateTime();
    QDateTime fifteenMinutesAgo = now.addSecs(-15 * 60);
    m_timeWindow = TimeSelectionSpan(fifteenMinutesAgo, now);
}

void SliderState::setYPosition(int y, int widgetHeight, const QTime& interval)
{
    int sliderHeight = SliderGeometry::calculateSliderHeight(interval, widgetHeight);
    QPair<int, int> bounds = SliderGeometry::getSliderBounds(widgetHeight, sliderHeight);
    m_yPosition = qBound(bounds.first, y, bounds.second);
    syncTimeWindowFromPosition(widgetHeight, interval);
}

int SliderState::getYPosition() const
{
    return m_yPosition;
}

void SliderState::setTimeWindow(const TimeSelectionSpan& window, int widgetHeight, const QTime& interval)
{
    m_timeWindow = window;
    // Sync position based on end time (top = now)
    syncPositionFromTimeWindow(widgetHeight);
    clampToBounds(widgetHeight, interval);
}

TimeSelectionSpan SliderState::getTimeWindow() const
{
    return m_timeWindow;
}

void SliderState::startDrag(const QPoint& mousePos)
{
    m_isDragging = true;
    m_dragStartMousePos = mousePos;
    m_dragStartSliderY = m_yPosition;
}

void SliderState::updateDrag(const QPoint& mousePos, int widgetHeight, const QTime& interval)
{
    if (!m_isDragging)
        return;
    
    // Calculate delta from drag start
    int deltaY = mousePos.y() - m_dragStartMousePos.y();
    int newSliderY = m_dragStartSliderY + deltaY;
    
    // Clamp to bounds
    int sliderHeight = SliderGeometry::calculateSliderHeight(interval, widgetHeight);
    QPair<int, int> bounds = SliderGeometry::getSliderBounds(widgetHeight, sliderHeight);
    m_yPosition = qBound(bounds.first, newSliderY, bounds.second);
    
    // Sync time window from new position
    syncTimeWindowFromPosition(widgetHeight, interval);
}

void SliderState::endDrag(int widgetHeight, const QTime& interval)
{
    if (!m_isDragging)
        return;
    
    m_isDragging = false;
    // Final sync to ensure time window is accurate
    syncTimeWindowFromPosition(widgetHeight, interval);
    clampToBounds(widgetHeight, interval);
}

bool SliderState::isDragging() const
{
    return m_isDragging;
}

void SliderState::clampToBounds(int widgetHeight, const QTime& interval)
{
    int sliderHeight = SliderGeometry::calculateSliderHeight(interval, widgetHeight);
    QPair<int, int> bounds = SliderGeometry::getSliderBounds(widgetHeight, sliderHeight);
    m_yPosition = qBound(bounds.first, m_yPosition, bounds.second);
}

void SliderState::syncTimeWindowFromPosition(int widgetHeight, const QTime& interval)
{
    // Calculate time window based on Y position
    // Top (Y=0) should represent "now", bottom represents "12 hours ago"
    QDateTime now = QDateTime::currentDateTime();
    QDateTime twelveHoursAgo = now.addSecs(-12 * 3600);
    
    // Convert Y position to time ratio (inverted: Y=0 means endTime=now)
    double positionRatio = 1.0 - (static_cast<double>(m_yPosition) / static_cast<double>(widgetHeight));
    int minutesFromStart = static_cast<int>(positionRatio * SliderGeometry::getTwelveHoursInMinutes());
    
    // Calculate window end time (top edge of slider)
    QDateTime windowEnd = twelveHoursAgo.addSecs(minutesFromStart * 60);
    
    // Calculate window start time based on interval
    int intervalSeconds = interval.hour() * 3600 + interval.minute() * 60 + interval.second();
    QDateTime windowStart = windowEnd.addSecs(-intervalSeconds);
    
    m_timeWindow = TimeSelectionSpan(windowStart, windowEnd);
}

void SliderState::syncPositionFromTimeWindow(int widgetHeight)
{
    // Calculate position based on the END time (top represents "now")
    // Position slider so its top edge represents the end time of the window
    QDateTime now = QDateTime::currentDateTime();
    QDateTime twelveHoursAgo = now.addSecs(-12 * 3600);
    
    // Calculate minutes from twelveHoursAgo to the window end time
    int minutesFromStart = twelveHoursAgo.msecsTo(m_timeWindow.endTime) / 60000;
    int twelveHoursInMinutes = SliderGeometry::getTwelveHoursInMinutes();
    minutesFromStart = qBound(0, minutesFromStart, twelveHoursInMinutes);
    
    // Convert to Y position (top = now, bottom = 12 hours ago)
    double positionRatio = static_cast<double>(minutesFromStart) / static_cast<double>(twelveHoursInMinutes);
    
    // Position is at the top of the slider rectangle, so we need to adjust
    // For a slider at the top, the top edge should be at Y=0 when endTime = now
    // So if endTime = now, Y should be 0 (after accounting for slider height)
    int calculatedY = static_cast<int>((1.0 - positionRatio) * widgetHeight);
    
    // But we want the slider rectangle's top edge at this position
    // Since positionRatio of 1.0 (endTime = now) should give Y = 0
    // We invert: Y = (1.0 - ratio) * height
    // For endTime at now: ratio = 1.0, so Y = 0 ✓
    // For endTime at twelveHoursAgo: ratio = 0.0, so Y = height (but need to account for slider height)
    
    m_yPosition = calculatedY;
}

// ============================================================================
// TimelineVisualizerWidget Implementation
// ============================================================================

TimelineVisualizerWidget::TimelineVisualizerWidget(QWidget *parent, GraphContainerSyncState *syncState)
    : QWidget(parent), m_currentTime(QTime::currentTime()), m_numberOfDivisions(15), m_lastCurrentTime(QTime::currentTime()), m_pixelSpeed(0.0), m_accumulatedOffset(0.0), m_chevronDrawer(nullptr), m_sliderIndicator(nullptr), m_syncState(syncState)
{
    setFixedWidth(TIMELINE_VIEW_GRAPHICS_VIEW_WIDTH);
    setMinimumHeight(50); // Set a minimum height

    // Remove all margins and padding for snug fit
    setContentsMargins(0, 0, 0, 0);

    // Enable mouse tracking for slider interaction
    setMouseTracking(true);

    // Initialize slider state: from "(now - interval)" to "now" (default 15 minutes)
    // Position slider at top (Y=0) representing the most recent time window
    QDateTime now = QDateTime::currentDateTime();
    int intervalSeconds = m_timeLineLength.hour() * 3600 + m_timeLineLength.minute() * 60 + m_timeLineLength.second();
    QDateTime startTime = now.addSecs(-intervalSeconds);
    TimeSelectionSpan initialWindow(startTime, now);
    
    // Set the time window (this will sync position to top since endTime = now)
    m_sliderState.setTimeWindow(initialWindow, rect().height(), m_timeLineLength);
    
    // Ensure slider is at the top (Y=0)
    m_sliderState.setYPosition(0, rect().height(), m_timeLineLength);
    
    // Keep legacy member in sync for now
    m_sliderVisibleWindow = m_sliderState.getTimeWindow();

    // Create drawing objects
    createDrawingObjects();
    
    // Create slider indicator
    createSliderIndicator();
}

void TimelineVisualizerWidget::setTimeLineLength(const QTime &length)
{
    m_timeLineLength = length;
    // Reset accumulated offset when timeline length changes
    m_accumulatedOffset = 0.0;
    updateVisualization();
}

void TimelineVisualizerWidget::setTimeInterval(TimeInterval interval)
{
    m_timeInterval = interval;

    // Convert TimeInterval to QTime and set timeline length
    QTime newLength = timeIntervalToQTime(interval);
    setTimeLineLength(newLength);

    // Reset accumulated offset when interval changes
    m_accumulatedOffset = 0.0;

    // Update slider state to match the new interval length
    // Keep the end time at current time, adjust the start time backward
    QDateTime now = QDateTime::currentDateTime();
    int intervalSeconds = newLength.hour() * 3600 + newLength.minute() * 60 + newLength.second();
    QDateTime startTime = now.addSecs(-intervalSeconds);
    TimeSelectionSpan newWindow(startTime, now);
    m_sliderState.setTimeWindow(newWindow, rect().height(), newLength);
    
    // Keep legacy member in sync
    m_sliderVisibleWindow = m_sliderState.getTimeWindow();

    // Recreate all drawing objects with new parameters
    // This will automatically calculate optimal divisions based on current area
    createDrawingObjects();

    // Emit signal for the updated time window
    emitTimeScopeChanged();

    // qDebug() << "Time interval set to:" << timeIntervalToString(interval)
    //          << "Divisions:" << m_numberOfDivisions
    //          << "Segment duration:" << calculateSegmentDurationSeconds() << "seconds"
    //          << "Min segment height:" << getMinimumSegmentHeight();
}

void TimelineVisualizerWidget::setCurrentTime(const QTime &currentTime)
{
    m_lastCurrentTime = m_currentTime;
    m_currentTime = currentTime;
    
    // Only update pixel speed and animate timeline when in follow mode
    // In frozen mode, keep the timeline static (no animation)
    if (m_timelineViewMode == TimelineViewMode::FOLLOW_MODE)
    {
        updatePixelSpeed();
    }
    
    // Don't update visualization if dragging (preserve dragged position)
    // The slider position will be recalculated when drag ends
    if (!m_sliderState.isDragging())
    {
        // Only update slider position to latest data when in follow mode
        if (m_timelineViewMode == TimelineViewMode::FOLLOW_MODE)
        {
            // In follow mode, keep slider at top (Y=0) and update time window to latest data
            QDateTime now = QDateTime::currentDateTime();
            int intervalSeconds = m_timeLineLength.hour() * 3600 + m_timeLineLength.minute() * 60 + m_timeLineLength.second();
            QDateTime startTime = now.addSecs(-intervalSeconds);
            TimeSelectionSpan newWindow(startTime, now);
            m_sliderState.setTimeWindow(newWindow, rect().height(), m_timeLineLength);
            // Ensure slider is at the top
            m_sliderState.setYPosition(0, rect().height(), m_timeLineLength);
            // Keep legacy member in sync
            m_sliderVisibleWindow = m_sliderState.getTimeWindow();
            // Emit signal to notify about time window change
            emitTimeScopeChanged();
        }
        // Always update visualization (needed for repaints), but animation only happens in follow mode
        updateVisualization();
    }
}

void TimelineVisualizerWidget::setNumberOfDivisions(int divisions)
{
    m_numberOfDivisions = divisions;
    // Recreate drawing objects with new division count
    createDrawingObjects();
    updateVisualization();
}

void TimelineVisualizerWidget::updateVisualization()
{
    // Trigger a repaint (slider position is calculated in paintEvent)
    update();
}

void TimelineVisualizerWidget::updateAndDraw()
{
    // This method provides a clean interface for external code to trigger the update + draw loop
    update(); // Trigger a repaint
}

void TimelineVisualizerWidget::updatePixelSpeed()
{
    // Don't update pixel speed or accumulate offset when in frozen mode
    // This prevents the timeline from animating/shifting
    if (m_timelineViewMode == TimelineViewMode::FROZEN_MODE)
    {
        return;
    }
    
    if (m_lastCurrentTime.isNull() || m_currentTime.isNull() || m_timeLineLength.isNull())
    {
        m_pixelSpeed = 0.0;
        return;
    }

    // Calculate time difference in seconds
    int timeDiffMs = m_lastCurrentTime.msecsTo(m_currentTime);
    if (timeDiffMs <= 0)
    {
        m_pixelSpeed = 0.0;
        return;
    }

    // Calculate segment duration in seconds
    double segmentDurationSeconds = calculateSegmentDurationSeconds();

    // Calculate pixel speed: pixels per second
    // This should be based on how fast we want segments to move
    double segmentHeight = static_cast<double>(rect().height()) / m_numberOfDivisions;

    // Pixel speed should be segmentHeight / segmentDurationSeconds
    // This means one segment height per segment duration
    m_pixelSpeed = segmentHeight / segmentDurationSeconds;

    // Update accumulated offset based on time difference
    // This creates the animation effect - only in follow mode
    double timeDiffSeconds = timeDiffMs / 1000.0;
    m_accumulatedOffset += m_pixelSpeed * timeDiffSeconds;

    // qDebug() << "Pixel speed updated:" << m_pixelSpeed << "pixels/sec, time diff:" << timeDiffMs << "ms, accumulated offset:" << m_accumulatedOffset
    //          << "Segment duration:" << segmentDurationSeconds << "seconds";
}

double TimelineVisualizerWidget::calculateSmoothOffset()
{
    // Return the accumulated offset for smooth shifting
    return m_accumulatedOffset;
}

int TimelineVisualizerWidget::calculateOptimalDivisions() const
{
    // Always use a fixed number of segments regardless of time interval
    return getFixedNumberOfSegments();
}

int TimelineVisualizerWidget::calculateOptimalDivisionsForArea(int areaHeight) const
{
    // Always return the fixed number of segments
    // The segment height will be calculated to fill the entire area
    return getFixedNumberOfSegments();
}

int TimelineVisualizerWidget::getFixedNumberOfSegments() const
{
    // Use a fixed number of segments for all time intervals
    // This ensures consistent drawing logic across all intervals
    return 20; // Fixed number of segments
}

double TimelineVisualizerWidget::getMinimumSegmentHeight() const
{
    // This method is no longer used in the fixed segment approach
    // Segment height is calculated dynamically based on widget height
    return 10.0; // Default value, not used
}

double TimelineVisualizerWidget::calculateSegmentDurationSeconds() const
{
    // Calculate how many seconds each segment represents
    int totalSeconds = m_timeLineLength.hour() * 3600 + m_timeLineLength.minute() * 60 + m_timeLineLength.second();
    return static_cast<double>(totalSeconds) / m_numberOfDivisions;
}

TimelineVisualizerWidget::~TimelineVisualizerWidget()
{
    clearDrawingObjects();
}

void TimelineVisualizerWidget::setIsAbsoluteTime(bool isAbsoluteTime)
{
    m_isAbsoluteTime = isAbsoluteTime;
    updateVisualization();
}

// No time selection methods needed for TimelineView

// No drawSelection method needed for TimelineView

// Drawing object management methods

void TimelineVisualizerWidget::createDrawingObjects()
{
    // Use a minimum height if widget height is not set yet
    int widgetHeight = height();
    if (widgetHeight <= 0)
    {
        widgetHeight = 300; // Default height for timeline view
        // qDebug() << "Widget height is 0, using default height:" << widgetHeight;
    }

    // Use fixed number of segments for all time intervals
    m_numberOfDivisions = getFixedNumberOfSegments();

    QRect drawArea(0, 0, TIMELINE_VIEW_GRAPHICS_VIEW_WIDTH, widgetHeight);

    // Calculate segment height to fill the entire drawing area
    double segmentHeight = static_cast<double>(widgetHeight) / m_numberOfDivisions;

    // qDebug() << "Creating drawing objects - Widget height:" << height()
    //          << "Using height:" << widgetHeight
    //          << "Draw area:" << drawArea
    //          << "Fixed divisions:" << m_numberOfDivisions
    //          << "Calculated segment height:" << segmentHeight
    //          << "Time interval:" << timeIntervalToString(m_timeInterval);

    // Create segment drawers for animation range (including off-screen segments)
    clearDrawingObjects(); // Clear existing ones first

    // Create chevron drawer - position it at the top of the timeline
    m_chevronDrawer = new TimelineChevronDrawer(drawArea, 30);

    // Create segments with fixed count but variable time gaps
    // We need enough segments to cover the entire visible area plus some buffer
    int segmentsNeeded = m_numberOfDivisions + 10; // Add buffer for smooth animation
    int startSegment = -(segmentsNeeded / 2);
    int endSegment = segmentsNeeded / 2;

    // qDebug() << "Creating fixed segments - Height:" << widgetHeight
    //          << "Fixed divisions:" << m_numberOfDivisions
    //          << "Segment height:" << segmentHeight
    //          << "Segments needed:" << segmentsNeeded
    //          << "Segments range:" << startSegment << "to" << endSegment;

    for (int i = startSegment; i < endSegment; ++i)
    {
        TimelineSegmentDrawer *segmentDrawer = new TimelineSegmentDrawer(
            i, m_timeLineLength, m_currentTime, m_numberOfDivisions, m_isAbsoluteTime, drawArea);
        segmentDrawer->setShowRelativeLabel(m_showRelativeLabels);
        m_segmentDrawers.push_back(segmentDrawer);
    }
}

void TimelineVisualizerWidget::clearDrawingObjects()
{
    // Clear segment drawers
    for (auto *segmentDrawer : m_segmentDrawers)
    {
        delete segmentDrawer;
    }
    m_segmentDrawers.clear();

    // Clear chevron drawer
    delete m_chevronDrawer;
    m_chevronDrawer = nullptr;
}

void TimelineVisualizerWidget::setShowRelativeLabels(bool showRelative)
{
    m_showRelativeLabels = showRelative;

    // Update all existing segment drawers
    for (auto *segmentDrawer : m_segmentDrawers)
    {
        if (segmentDrawer)
        {
            segmentDrawer->setShowRelativeLabel(showRelative);
        }
    }
}

void TimelineVisualizerWidget::setChevronLabel1(const QString &label)
{
    m_chevronLabel1 = label;
    updateVisualization();
}

void TimelineVisualizerWidget::setChevronLabel2(const QString &label)
{
    m_chevronLabel2 = label;
    updateVisualization();
}

void TimelineVisualizerWidget::setChevronLabel3(const QString &label)
{
    m_chevronLabel3 = label;
    updateVisualization();
}

QString TimelineVisualizerWidget::getChevronLabel1() const
{
    return m_chevronLabel1;
}

QString TimelineVisualizerWidget::getChevronLabel2() const
{
    return m_chevronLabel2;
}

QString TimelineVisualizerWidget::getChevronLabel3() const
{
    return m_chevronLabel3;
}

// TimelineView chevron label control methods
void TimelineView::setChevronLabel1(const QString &label)
{
    if (m_visualizerWidget)
    {
        m_visualizerWidget->setChevronLabel1(label);
    }
}

void TimelineView::setChevronLabel2(const QString &label)
{
    if (m_visualizerWidget)
    {
        m_visualizerWidget->setChevronLabel2(label);
    }
}

void TimelineView::setChevronLabel3(const QString &label)
{
    if (m_visualizerWidget)
    {
        m_visualizerWidget->setChevronLabel3(label);
    }
}

QString TimelineView::getChevronLabel1() const
{
    if (m_visualizerWidget)
    {
        return m_visualizerWidget->getChevronLabel1();
    }
    return QString();
}

QString TimelineView::getChevronLabel2() const
{
    if (m_visualizerWidget)
    {
        return m_visualizerWidget->getChevronLabel2();
    }
    return QString();
}

QString TimelineView::getChevronLabel3() const
{
    if (m_visualizerWidget)
    {
        return m_visualizerWidget->getChevronLabel3();
    }
    return QString();
}

void TimelineVisualizerWidget::paintEvent(QPaintEvent * /* event */)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Fill with black background
    painter.fillRect(rect(), QColor(0, 0, 0));

    // Calculate smooth offset to determine which segments to draw
    double smoothOffset = calculateSmoothOffset();
    double segmentHeight = static_cast<double>(rect().height()) / m_numberOfDivisions;

    // Debug output for segment height calculation
    static int debugCounter = 0;
    if (debugCounter++ % 60 == 0)
    { // Print every 60 frames to avoid spam
        // qDebug() << "PaintEvent - Widget rect:" << rect()
        //          << "Divisions:" << m_numberOfDivisions
        //          << "Segment height:" << segmentHeight
        //          << "Time interval:" << timeIntervalToString(m_timeInterval);
    }

    // Remove segments that have gone completely out of view (below the bottom)
    auto it = m_segmentDrawers.begin();
    while (it != m_segmentDrawers.end())
    {
        TimelineSegmentDrawer *segmentDrawer = *it;
        if (segmentDrawer)
        {
            double y = segmentDrawer->getSegmentNumber() * segmentHeight + smoothOffset;
            // If segment is completely below the visible area, remove it
            if (y > rect().height())
            {
                delete segmentDrawer;
                it = m_segmentDrawers.erase(it);
                continue;
            }
        }
        ++it;
    }

    // Create new segments as needed to ensure we have enough coverage
    if (!m_segmentDrawers.empty())
    {
        int minSegmentNumber = (*std::min_element(m_segmentDrawers.begin(), m_segmentDrawers.end(),
                                                  [](TimelineSegmentDrawer *a, TimelineSegmentDrawer *b)
                                                  {
                                                      return a->getSegmentNumber() < b->getSegmentNumber();
                                                  }))
                                   ->getSegmentNumber();

        int maxSegmentNumber = (*std::max_element(m_segmentDrawers.begin(), m_segmentDrawers.end(),
                                                  [](TimelineSegmentDrawer *a, TimelineSegmentDrawer *b)
                                                  {
                                                      return a->getSegmentNumber() < b->getSegmentNumber();
                                                  }))
                                   ->getSegmentNumber();

        // Calculate which segments should be visible to fill the entire area
        int firstVisibleSegment = static_cast<int>(-smoothOffset / segmentHeight);
        int lastVisibleSegment = firstVisibleSegment + m_numberOfDivisions;

        // Add segments above the current range if needed
        while (minSegmentNumber > firstVisibleSegment - 2)
        { // Keep some buffer above
            --minSegmentNumber;
            TimelineSegmentDrawer *segmentDrawer = new TimelineSegmentDrawer(
                minSegmentNumber, m_timeLineLength, m_currentTime, m_numberOfDivisions, m_isAbsoluteTime, rect());
            segmentDrawer->setShowRelativeLabel(m_showRelativeLabels);
            m_segmentDrawers.push_back(segmentDrawer);
        }

        // Add segments below the current range if needed
        while (maxSegmentNumber < lastVisibleSegment + 2)
        { // Keep some buffer below
            ++maxSegmentNumber;
            TimelineSegmentDrawer *segmentDrawer = new TimelineSegmentDrawer(
                maxSegmentNumber, m_timeLineLength, m_currentTime, m_numberOfDivisions, m_isAbsoluteTime, rect());
            segmentDrawer->setShowRelativeLabel(m_showRelativeLabels);
            m_segmentDrawers.push_back(segmentDrawer);
        }
    }

    // Calculate which segments should be visible to fill the entire area
    // We need to ensure we have exactly m_numberOfDivisions segments covering the full height
    int firstVisibleSegment = static_cast<int>(-smoothOffset / segmentHeight);
    int lastVisibleSegment = firstVisibleSegment + m_numberOfDivisions;

    // Draw segments that are visible (including those that might be partially off-screen due to smooth shifting)
    for (auto *segmentDrawer : m_segmentDrawers)
    {
        if (segmentDrawer)
        {
            int segmentNumber = segmentDrawer->getSegmentNumber();
            // Calculate Y position for this segment with smooth offset (shift down)
            double y = segmentNumber * segmentHeight + smoothOffset;

            // Only draw if the segment is within the visible range
            if (segmentNumber >= firstVisibleSegment && segmentNumber < lastVisibleSegment)
            {
                // Update the segment drawer with current state
                segmentDrawer->setDrawArea(rect());
                segmentDrawer->setTimelineLength(m_timeLineLength);
                segmentDrawer->setCurrentTime(m_currentTime);
                segmentDrawer->setNumberOfDivisions(m_numberOfDivisions);
                segmentDrawer->setIsAbsoluteTime(m_isAbsoluteTime);
                segmentDrawer->setSmoothOffset(smoothOffset);
                segmentDrawer->update();

                // Draw the segment using QPainter
                drawSegmentWithPainter(painter, segmentDrawer);
            }
        }
    }

    // Debug: Check if we're covering the entire area
    static int debugCounter2 = 0;
    if (debugCounter2++ % 60 == 0)
    {
        // qDebug() << "Segment coverage check - Widget height:" << rect().height()
        //          << "Total segments:" << m_segmentDrawers.size()
        //          << "Divisions:" << m_numberOfDivisions
        //          << "Segment height:" << segmentHeight
        //          << "Total coverage:" << (m_numberOfDivisions * segmentHeight)
        //          << "First visible:" << firstVisibleSegment
        //          << "Last visible:" << lastVisibleSegment
        //          << "Smooth offset:" << smoothOffset;
    }

    // Draw chevron using drawing object - position it at the top of the timeline
    // Only draw if chevron is visible
    if (m_chevronVisible && m_chevronDrawer)
    {
        m_chevronDrawer->setDrawArea(rect());
        m_chevronDrawer->setYOffset(30); // Position at the top with enough space for the chevron box
        m_chevronDrawer->update();
        drawChevronWithPainter(painter, m_chevronDrawer);
    }

    // Draw a border to make it more visible
    painter.setPen(QPen(QColor(150, 150, 150), 1));
    painter.drawRect(rect().adjusted(0, 0, -1, -1));

    // Draw slider indicator using geometry helper
    // Only draw if slider is visible
    if (m_sliderVisible)
    {
        QRect sliderRect = SliderGeometry::calculateSliderRect(
            rect().height(), rect().width(), m_timeLineLength,
            m_sliderState.getYPosition());
        
        QColor sliderColor(255, 255, 255, 128); // 50% opacity white
        painter.fillRect(sliderRect, sliderColor);
    }

    // Draw navtime labels if sync state is available
    if (m_syncState && m_syncState->hasCurrentNavTime)
    {
        drawNavTimeLabels(painter, rect());
    }
}

void TimelineVisualizerWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    // Recreate drawing objects with new dimensions
    // qDebug() << "Widget resized to:" << size();
    createDrawingObjects();
    
    // Update slider state for new size (clamp position to new bounds)
    m_sliderState.clampToBounds(rect().height(), m_timeLineLength);
    
    // Update slider indicator for new size
    updateSliderIndicator();
}

// Helper methods to draw using QPainter instead of QGraphicsScene
void TimelineVisualizerWidget::drawSegmentWithPainter(QPainter &painter, TimelineSegmentDrawer *segmentDrawer)
{
    if (!segmentDrawer)
        return;

    QRect drawArea = segmentDrawer->getDrawArea();
    int numberOfDivisions = segmentDrawer->getNumberOfDivisions();
    int segmentNumber = segmentDrawer->getSegmentNumber();
    double smoothOffset = segmentDrawer->getSmoothOffset();

    // Calculate segment height
    double segmentHeight = static_cast<double>(drawArea.height()) / numberOfDivisions;

    // Calculate Y position for this segment with smooth offset (shift down)
    double y = segmentNumber * segmentHeight + smoothOffset;

    // Only show labels on every third section (0, 3, 6, 9, 12, ...)
    bool shouldShowLabel = (segmentNumber % 3 == 0);

    if (shouldShowLabel && segmentDrawer->isLabelSet())
    {
        // Use the fixed label that was set during construction
        QString timestamp = segmentDrawer->getFixedLabel();
        if (!timestamp.isEmpty())
        {
            // Set text color to white for visibility on dark background
            painter.setPen(QPen(QColor(255, 255, 255), 1));

            // Calculate text metrics
            QFontMetrics fm(painter.font());
            int textWidth = fm.horizontalAdvance(timestamp);
            int textHeight = fm.height();

            // Calculate center position for the text within the segment
            int centerX = (drawArea.width() - textWidth) / 2;
            int centerY = static_cast<int>(y + segmentHeight / 2 + textHeight / 2);

            // Draw the timestamp centered in the segment
            painter.drawText(QPoint(centerX, centerY), timestamp);
        }
    }

    // Draw two ticks which are 15% of the segment width
    int tickWidth = static_cast<int>(drawArea.width() * 0.15);
    int tickY = static_cast<int>(y + segmentHeight / 2);

    // Left tick
    painter.setPen(QPen(QColor(255, 255, 255), 1)); // White pen
    painter.drawLine(0, tickY, tickWidth, tickY);

    // Right tick
    painter.drawLine(drawArea.width(), tickY, drawArea.width() - tickWidth, tickY);
}

void TimelineVisualizerWidget::drawChevronWithPainter(QPainter &painter, TimelineChevronDrawer *chevronDrawer)
{
    if (!chevronDrawer)
    {
        // qDebug() << "Chevron drawer is null!";
        return;
    }

    QRect drawArea = chevronDrawer->getDrawArea();
    int yOffset = chevronDrawer->getYOffset();
    double chevronWidthPercent = chevronDrawer->getChevronWidthPercent();
    int chevronHeight = chevronDrawer->getChevronHeight();
    int chevronBoxHeight = chevronDrawer->getChevronBoxHeight();

    int widgetWidth = drawArea.width();

    // Set pen for blue chevron outline
    painter.setPen(QPen(QColor(0, 100, 255), 3)); // Blue color, 3px width for better visibility

    // Define chevron size (width and height)
    int chevronWidth = static_cast<int>(widgetWidth * chevronWidthPercent);

    // Calculate chevron position (centered horizontally)
    int chevronX = (widgetWidth - chevronWidth) / 2;
    int chevronY = yOffset;

    // Calculate the tip position (bottom center of V)
    int tipX = chevronX + chevronWidth / 2;
    int tipY = chevronY + chevronHeight;

    // Define chevron points (pointing down: V) and the lines to the edges
    QPoint chevronPoints[8] = {
        QPoint(0, chevronY - chevronBoxHeight),           // Start point
        QPoint(0, chevronY),                              // Left edge
        QPoint(chevronX, chevronY),                       // Top left point
        QPoint(tipX, tipY),                               // Bottom point (tip)
        QPoint(chevronX + chevronWidth, chevronY),        // Top right point
        QPoint(widgetWidth, chevronY),                    // Right edge
        QPoint(widgetWidth, chevronY - chevronBoxHeight), // Right edge
        QPoint(0, chevronY - chevronBoxHeight)            // Start point
    };

    // Draw the chevron outline
    painter.drawPolygon(chevronPoints, 8);

    // Draw the 3 labels - 1 and 3 below the V, 2 at the top
    painter.setPen(QPen(QColor(0, 100, 255), 2)); // Blue text, thicker

    // Calculate text metrics for centering
    QFontMetrics fontMetrics(painter.font());

    // Label 1: below the chevron, left of the V (centered at chevronX)
    if (!m_chevronLabel1.isEmpty())
    {
        int label1Width = fontMetrics.horizontalAdvance(m_chevronLabel1);
        int label1X = chevronX - label1Width / 2; // Center at chevronX
        int label1Y = tipY + 15;
        painter.drawText(QPoint(label1X, label1Y), m_chevronLabel1);
    }

    // Label 2: at the top center (centered at tipX)
    if (!m_chevronLabel2.isEmpty())
    {
        int label2Width = fontMetrics.horizontalAdvance(m_chevronLabel2);
        int label2X = tipX - label2Width / 2; // Center at tipX
        int label2Y = chevronY;
        painter.drawText(QPoint(label2X, label2Y), m_chevronLabel2);
    }

    // Label 3: below the chevron, right of the V (centered at chevronX + chevronWidth)
    if (!m_chevronLabel3.isEmpty())
    {
        int label3Width = fontMetrics.horizontalAdvance(m_chevronLabel3);
        int label3X = (chevronX + chevronWidth) - label3Width / 2; // Center at right edge
        int label3Y = tipY + 15;
        painter.drawText(QPoint(label3X, label3Y), m_chevronLabel3);
    }
}

// Slider methods implementation (following zoom slider pattern - vertical orientation)
void TimelineVisualizerWidget::createSliderIndicator()
{
    // Create a simple QGraphicsRectItem for the slider indicator
    // We'll use the widget's rect as the "scene" and draw directly
    // Actually, we need to create it without a scene for now since we're using QPainter
    // Let's create it as a simple rectangle we can track
    
    // For now, we'll keep using QPainter but structure the code like zoom slider
    updateSliderIndicator();
}

void TimelineVisualizerWidget::updateSliderIndicator()
{
    // Sync slider position from time window (called when interval changes)
    // The actual drawing is done in paintEvent using SliderState
    if (rect().height() <= 0)
    {
        return;
    }
    
    // Sync position from current time window
    m_sliderState.syncPositionFromTimeWindow(rect().height());
    m_sliderState.clampToBounds(rect().height(), m_timeLineLength);
    
    // Keep legacy member in sync
    m_sliderVisibleWindow = m_sliderState.getTimeWindow();
}

void TimelineVisualizerWidget::updateSliderFromMousePosition(const QPoint& currentPos)
{
    // This method is now replaced by SliderState::updateDrag()
    // Keeping for backward compatibility but delegating to state manager
    if (rect().height() <= 0)
    {
        return;
    }
    
    m_sliderState.updateDrag(currentPos, rect().height(), m_timeLineLength);
    
    // Keep legacy member in sync
    m_sliderVisibleWindow = m_sliderState.getTimeWindow();
    
    // Trigger immediate repaint to show updated slider position
    repaint();
    
    // Emit signal during drag
    emitTimeScopeChanged();
}

void TimelineVisualizerWidget::emitTimeScopeChanged()
{
    // Get time window from state manager and ensure it's valid before emitting
    TimeSelectionSpan window = m_sliderState.getTimeWindow();
    
    // Keep legacy member in sync
    m_sliderVisibleWindow = window;
    
    if (window.startTime.isValid() && window.endTime.isValid())
    {
        // Ensure endTime is after startTime
        if (window.startTime <= window.endTime)
        {
            emit visibleTimeWindowChanged(window);
        }
        else
        {
            // Swap times if they're reversed
            TimeSelectionSpan corrected(window.endTime, window.startTime);
            emit visibleTimeWindowChanged(corrected);
        }
    }
}

void TimelineVisualizerWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && !m_sliderState.isDragging())
    {
        QPoint pos = event->pos();
        
        // Calculate slider rectangle using geometry helper
        QRect sliderRect = SliderGeometry::calculateSliderRect(
            rect().height(), rect().width(), m_timeLineLength,
            m_sliderState.getYPosition());
        
        if (sliderRect.contains(pos))
        {
            m_sliderState.startDrag(pos);
            setCursor(Qt::ClosedHandCursor);
            qDebug() << "Slider drag started at Y:" << pos.y() << "Slider Y:" << m_sliderState.getYPosition();
            event->accept();
            return;
        }
    }
    QWidget::mousePressEvent(event);
}

void TimelineVisualizerWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_sliderState.isDragging())
    {
        // Update slider based on mouse movement using state manager
        m_sliderState.updateDrag(event->pos(), rect().height(), m_timeLineLength);
        
        // Keep legacy member in sync
        m_sliderVisibleWindow = m_sliderState.getTimeWindow();
        
        // Trigger immediate repaint for smooth dragging
        repaint();
        
        // Emit signal during drag
        emitTimeScopeChanged();
        
        event->accept();
        return;
    }
    else
    {
        // Update cursor based on hover position using geometry helper
        QPoint pos = event->pos();
        
        QRect sliderRect = SliderGeometry::calculateSliderRect(
            rect().height(), rect().width(), m_timeLineLength,
            m_sliderState.getYPosition());
        
        setCursor(sliderRect.contains(pos) ? Qt::OpenHandCursor : Qt::ArrowCursor);
    }
    QWidget::mouseMoveEvent(event);
}

void TimelineVisualizerWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_sliderState.isDragging())
    {
        // End drag using state manager (finalizes position and syncs time window)
        m_sliderState.endDrag(rect().height(), m_timeLineLength);
        
        // Check if slider is at the top (Y=0 or very close to it)
        int sliderY = m_sliderState.getYPosition();
        const int SNAP_THRESHOLD = 5; // pixels - threshold for snapping to top
        
        if (sliderY <= SNAP_THRESHOLD)
        {
            // Snap slider to top and switch to follow mode
            m_sliderState.setYPosition(0, rect().height(), m_timeLineLength);
            m_timelineViewMode = TimelineViewMode::FOLLOW_MODE;
            
            // Update time window to latest data
            QDateTime now = QDateTime::currentDateTime();
            int intervalSeconds = m_timeLineLength.hour() * 3600 + m_timeLineLength.minute() * 60 + m_timeLineLength.second();
            QDateTime startTime = now.addSecs(-intervalSeconds);
            TimeSelectionSpan newWindow(startTime, now);
            m_sliderState.setTimeWindow(newWindow, rect().height(), m_timeLineLength);
            
            // Emit signal to notify parent TimelineView
            emit timelineViewModeChanged(TimelineViewMode::FOLLOW_MODE);
            
            qDebug() << "Slider snapped to top - switched to FOLLOW_MODE";
        }
        else
        {
            // Slider is not at top, switch to frozen mode
            m_timelineViewMode = TimelineViewMode::FROZEN_MODE;
            
            // Emit signal to notify parent TimelineView
            emit timelineViewModeChanged(TimelineViewMode::FROZEN_MODE);
            
            qDebug() << "Slider not at top - switched to FROZEN_MODE at Y:" << sliderY;
        }
        
        // Keep legacy member in sync
        m_sliderVisibleWindow = m_sliderState.getTimeWindow();
        
        setCursor(Qt::ArrowCursor);
        
        // Emit signal when drag is complete (final update)
        emitTimeScopeChanged();
        qDebug() << "Slider drag ended - Final window:" 
                 << m_sliderVisibleWindow.startTime.toString("HH:mm:ss") 
                 << "to" << m_sliderVisibleWindow.endTime.toString("HH:mm:ss");
        
        // Trigger repaint to show final position
        update();
    }
    QWidget::mouseReleaseEvent(event);
}

void TimelineVisualizerWidget::enterEvent(QEnterEvent* event)
{
    QWidget::enterEvent(event);
    // Cursor will be updated in mouseMoveEvent
}

void TimelineVisualizerWidget::setTimelineViewMode(TimelineViewMode mode)
{
    m_timelineViewMode = mode;
    
    // If switching to follow mode, snap slider to top and update to latest data
    if (mode == TimelineViewMode::FOLLOW_MODE)
    {
        QDateTime now = QDateTime::currentDateTime();
        int intervalSeconds = m_timeLineLength.hour() * 3600 + m_timeLineLength.minute() * 60 + m_timeLineLength.second();
        QDateTime startTime = now.addSecs(-intervalSeconds);
        TimeSelectionSpan newWindow(startTime, now);
        m_sliderState.setTimeWindow(newWindow, rect().height(), m_timeLineLength);
        // Ensure slider is at the top
        m_sliderState.setYPosition(0, rect().height(), m_timeLineLength);
        // Keep legacy member in sync
        m_sliderVisibleWindow = m_sliderState.getTimeWindow();
        emitTimeScopeChanged();
        update();
    }
}

void TimelineVisualizerWidget::setTimeWindowSilent(const TimeSelectionSpan& window)
{
    // Update slider state with new time window (this will sync the position)
    m_sliderState.setTimeWindow(window, rect().height(), m_timeLineLength);
    
    // Keep legacy member in sync
    m_sliderVisibleWindow = m_sliderState.getTimeWindow();
    
    // Update visualization to reflect new slider position
    // Note: We do NOT emit visibleTimeWindowChanged signal to avoid feedback loops
    updateVisualization();
}

// Navtime label calculation methods
int TimelineVisualizerWidget::getLabelSpacingMinutes(TimeInterval interval) const
{
    // Determine label spacing based on interval:
    // 15 minutes -> every 3 minutes
    // 30 minutes -> every 6 minutes
    // 1 hour -> every 12 minutes
    // 2 hours -> every 24 minutes
    // 3 hours -> every 36 minutes
    // 6 hours -> every 72 minutes (1 hour 12 minutes)
    // 12 hours -> every 144 minutes (2 hours 24 minutes)
    
    int intervalMinutes = static_cast<int>(interval);
    
    // Calculate spacing as 20% of interval (rounded to nearest minute)
    // This gives us: 15->3, 30->6, 60->12, 120->24, 180->36, 360->72, 720->144
    int spacing = static_cast<int>(std::round(intervalMinutes * 0.2));
    
    // Ensure minimum spacing of 1 minute
    return qMax(1, spacing);
}

std::vector<QDateTime> TimelineVisualizerWidget::calculateNavTimeLabels(
    const QDateTime& currentNavTime, TimeInterval interval, const QTime& timelineLength) const
{
    std::vector<QDateTime> labels;
    
    if (!currentNavTime.isValid())
    {
        return labels;
    }
    
    int spacingMinutes = getLabelSpacingMinutes(interval);
    int timelineLengthMinutes = timelineLength.hour() * 60 + timelineLength.minute();
    
    // Calculate the start time (currentNavTime - timelineLength)
    QDateTime startNavTime = currentNavTime.addSecs(-timelineLengthMinutes * 60);
    
    // Find the first label time that's >= startNavTime
    // Round down to the nearest spacing interval
    qint64 startSeconds = startNavTime.toMSecsSinceEpoch() / 1000;
    qint64 spacingSeconds = spacingMinutes * 60;
    qint64 firstLabelSeconds = (startSeconds / spacingSeconds) * spacingSeconds;
    
    // Generate labels from first label to currentNavTime
    QDateTime labelTime = QDateTime::fromMSecsSinceEpoch(firstLabelSeconds * 1000);
    QDateTime endTime = currentNavTime.addSecs(60); // Add 1 minute buffer to include current time
    
    while (labelTime <= endTime)
    {
        labels.push_back(labelTime);
        labelTime = labelTime.addSecs(spacingSeconds);
    }
    
    return labels;
}

double TimelineVisualizerWidget::calculateLabelYPosition(
    const QDateTime& labelNavTime, const QDateTime& currentNavTime, 
    const QTime& timelineLength, int widgetHeight) const
{
    if (!labelNavTime.isValid() || !currentNavTime.isValid())
    {
        return 0.0;
    }
    
    // Calculate time difference in seconds (positive if labelNavTime is before currentNavTime)
    qint64 diffSeconds = labelNavTime.msecsTo(currentNavTime) / 1000;
    
    // Convert to minutes
    double diffMinutes = static_cast<double>(diffSeconds) / 60.0;
    
    // Get timeline length in minutes
    int timelineLengthMinutes = timelineLength.hour() * 60 + timelineLength.minute();
    
    // Calculate position ratio (0.0 = top = currentNavTime, 1.0 = bottom = currentNavTime - timelineLength)
    // Note: In the timeline, top (Y=0) represents the most recent time (currentNavTime)
    // If labelNavTime is before currentNavTime, diffMinutes is positive
    double positionRatio = diffMinutes / static_cast<double>(timelineLengthMinutes);
    
    // Clamp to [0, 1] - labels outside the timeline range won't be drawn
    positionRatio = qBound(0.0, positionRatio, 1.0);
    
    // Convert to Y position (top = 0, bottom = widgetHeight)
    return positionRatio * widgetHeight;
}

void TimelineVisualizerWidget::drawNavTimeLabels(QPainter& painter, const QRect& drawArea)
{
    if (!m_syncState || !m_syncState->hasCurrentNavTime)
    {
        return;
    }
    
    QDateTime currentNavTime = m_syncState->currentNavTime;
    
    // Calculate which labels to show
    std::vector<QDateTime> labels = calculateNavTimeLabels(currentNavTime, m_timeInterval, m_timeLineLength);
    
    // Set text color to white for visibility on dark background
    painter.setPen(QPen(QColor(255, 255, 255), 1));
    QFontMetrics fm(painter.font());
    
    for (const QDateTime& labelNavTime : labels)
    {
        // Calculate Y position for this label
        double y = calculateLabelYPosition(labelNavTime, currentNavTime, m_timeLineLength, drawArea.height());
        
        // Only draw if label is within visible area
        if (y >= 0 && y <= drawArea.height())
        {
            // Format the label as HH:mm
            QString labelText = labelNavTime.toString("HH:mm");
            
            // Calculate text metrics
            int textWidth = fm.horizontalAdvance(labelText);
            int textHeight = fm.height();
            
            // Calculate center position for the text
            int centerX = (drawArea.width() - textWidth) / 2;
            int centerY = static_cast<int>(y + textHeight / 2);
            
            // Draw the timestamp
            painter.drawText(QPoint(centerX, centerY), labelText);
        }
    }
}

TimelineView::TimelineView(QWidget *parent, QTimer *timer, GraphContainerSyncState *syncState)
    : QWidget(parent), 
    m_intervalChangeButton(nullptr), 
    m_timeModeChangeButton(nullptr), 
    m_visualizerWidget(nullptr), 
    m_layout(nullptr), 
    m_timer(timer), 
    m_ownsTimer(false),
    m_timelineViewMode(TimelineViewMode::FOLLOW_MODE),
    m_syncState(syncState)
{
    // Setup timer (create default if none provided)
    setupTimer();

    // Remove all margins and padding for snug fit
    setContentsMargins(0, 0, 0, 0);

    // Create vertical layout with no margins or spacing
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    // Create button with grey background and white border
    m_intervalChangeButton = new QPushButton("dt: 00:15", this);
    m_intervalChangeButton->setFixedSize(TIMELINE_VIEW_GRAPHICS_VIEW_WIDTH, TIMELINE_VIEW_BUTTON_SIZE / 2);
    m_intervalChangeButton->setContentsMargins(0, 0, 0, 0); // Remove button margins
    m_intervalChangeButton->setStyleSheet(
        "QPushButton {"
        "    background-color: black;"
        "    border: 2px solid white;"
        "    color: white;"
        "    font-weight: bold;"
        "    margin: 0px;"
        "    padding: 0px;"
        "}"
        "QPushButton:hover {"
        "    background-color: darkgrey;"
        "}"
        "QPushButton:pressed {"
        "    background-color: dimgrey;"
        "}");

    // setup m_timeModeChangeButton
    m_timeModeChangeButton = new QPushButton("Abs", this);
    m_timeModeChangeButton->setFixedSize(TIMELINE_VIEW_GRAPHICS_VIEW_WIDTH, TIMELINE_VIEW_BUTTON_SIZE / 2);
    m_timeModeChangeButton->setContentsMargins(0, 0, 0, 0); // Remove button margins
    m_timeModeChangeButton->setStyleSheet(
        "QPushButton {"
        "    background-color: black;"
        "    border: 2px solid white;"
        "    color: white;"
        "    font-weight: bold;"
        "    margin: 0px;"
        "    padding: 0px;"
        "}"
        "QPushButton:hover {"
        "    background-color: darkgrey;"
        "}"
        "QPushButton:pressed {"
        "    background-color: dimgrey;"
        "}");

    m_isAbsoluteTime = true;
    // updateTimeModeButtonText(m_isAbsoluteTime);

    // Create visualizer widget with sync state
    m_visualizerWidget = new TimelineVisualizerWidget(this, m_syncState);

    // Add widgets to layout
    m_layout->addWidget(m_timeModeChangeButton);
    m_layout->addWidget(m_intervalChangeButton);
    m_layout->addWidget(m_visualizerWidget, 1); // Stretch factor of 1 to fill remaining space

    // Connect button click to internal handler
    connect(m_timeModeChangeButton, &QPushButton::clicked, this, &TimelineView::onTimeModeButtonClicked);
    connect(m_intervalChangeButton, &QPushButton::clicked, this, &TimelineView::onIntervalButtonClicked);

    // Connect slider signal to emit TimeScopeChanged
    if (m_visualizerWidget)
    {
        connect(m_visualizerWidget, &TimelineVisualizerWidget::visibleTimeWindowChanged, 
                this, &TimelineView::onVisibleTimeWindowChanged);
        connect(m_visualizerWidget, &TimelineVisualizerWidget::timelineViewModeChanged,
                this, &TimelineView::onTimelineViewModeChanged);
        // Initialize the visualizer widget's mode
        m_visualizerWidget->setTimelineViewMode(m_timelineViewMode);
    }

    // Set the layout
    setLayout(m_layout);

    // Set the TimelineView widget width to match button and graphics view width
    setFixedWidth(TIMELINE_VIEW_GRAPHICS_VIEW_WIDTH);

    // Set the default time interval to be 15 minutes
    m_visualizerWidget->setTimeInterval(TimeInterval::FifteenMinutes);

    // Initialize button text with default interval
    updateButtonText(TimeInterval::FifteenMinutes);
    // Note: setTimeInterval will emit TimeScopeChanged signal via the connection established above
}

TimelineView::~TimelineView()
{
    // Stop the timer if we own it
    if (m_timer && m_ownsTimer)
    {
        m_timer->stop();
        // Timer will be automatically deleted by Qt's parent-child system
    }
}

void TimelineView::setupTimer()
{
    // If no timer provided, create a default 1-second timer
    if (!m_timer)
    {
        m_timer = new QTimer(this);
        m_ownsTimer = true;
        m_timer->setInterval(1000); // 1 second
    }

    // Connect timer to our tick handler
    connect(m_timer, &QTimer::timeout, this, &TimelineView::onTimerTick);

    // Start the timer
    m_timer->start();

    // qDebug() << "TimelineView: Timer setup completed - interval:" << m_timer->interval() << "ms";
}

void TimelineView::onTimerTick()
{
    // Update current time to the visualizer widget
    QTime currentTime = QTime::currentTime();

    if (m_visualizerWidget)
    {
        m_visualizerWidget->setCurrentTime(currentTime);
    }

    // qDebug() << "TimelineView: Timer tick - updated current time to" << currentTime.toString();
}

void TimelineView::updateButtonText(TimeInterval interval)
{
    QTime timeInterval = timeIntervalToQTime(interval);
    QString timeString = timeInterval.toString("HH:mm");
    QString buttonText = QString("dt: %1").arg(timeString);
    m_intervalChangeButton->setText(buttonText);
}

void TimelineView::onIntervalButtonClicked()
{
    // Cycle through valid time intervals on each button click
    static const std::vector<TimeInterval> intervals = getValidTimeIntervals();
    static int intervalIndex = 0;

    // Advance to next interval
    intervalIndex = (intervalIndex + 1) % intervals.size();
    TimeInterval nextInterval = intervals[intervalIndex];
    m_visualizerWidget->setTimeInterval(nextInterval);
    updateButtonText(nextInterval);

    // Trigger the intervalChanged signal
    emit TimeIntervalChanged(nextInterval);
}

void TimelineView::onTimeModeButtonClicked()
{
    m_isAbsoluteTime = !m_isAbsoluteTime;
    m_visualizerWidget->setShowRelativeLabels(!m_isAbsoluteTime);
    updateTimeModeButtonText(m_isAbsoluteTime);
}

void TimelineView::updateTimeModeButtonText(bool isAbsoluteTime)
{
    QString buttonText = isAbsoluteTime ? "Abs" : "Rel";
    m_timeModeChangeButton->setText(buttonText);
}

void TimelineView::onVisibleTimeWindowChanged(const TimeSelectionSpan& selection)
{
    // Ensure we emit a valid timespan with proper start and end times
    if (selection.startTime.isValid() && selection.endTime.isValid())
    {
        emit TimeScopeChanged(selection);
    }
}


void TimelineView::handleModeTransitionLogic(TimelineViewMode newMode)
{
    // Handles all the logic for the mode transition
    // Case 1 : FOLLOW_MODE -> FROZEN_MODE
    // Case 2 : FROZEN_MODE -> FOLLOW_MODE
    // TODO: TBA
}

// Primarily handles the mode change signal from the visualizer widget
void TimelineView::onTimelineViewModeChanged(TimelineViewMode mode)
{
    // Update our mode to match the visualizer widget
    m_timelineViewMode = mode;
    

    // This is the standard location where the mode transition logic is handled
    handleModeTransitionLogic(mode);

    // Emit signal for mode change
    bool isInFollowMode = (mode == TimelineViewMode::FOLLOW_MODE);
    emit GraphContainerInFollowModeChanged(isInFollowMode);
}

void TimelineView::setTimeWindowSilent(const TimeSelectionSpan& window)
{
    // Delegate to visualizer widget
    if (m_visualizerWidget)
    {
        m_visualizerWidget->setTimeWindowSilent(window);
    }
}

// Handles the mode change request from the user / outside the widget
void TimelineView::setTimelineViewMode(TimelineViewMode mode)
{
    m_timelineViewMode = mode;
    
    // Update the visualizer widget's mode
    if (m_visualizerWidget)
    {
        m_visualizerWidget->setTimelineViewMode(mode);
    }

    // This is the standard location where the mode transition logic is handled
    // Case 1 : FOLLOW_MODE -> FROZEN_MODE

    handleModeTransitionLogic(TimelineViewMode::FROZEN_MODE);

    // Case 2 : FROZEN_MODE -> FOLLOW_MODE

    handleModeTransitionLogic(mode);
    
}

// Navtime label calculation methods (delegate to visualizer widget)
int TimelineView::getLabelSpacingMinutes(TimeInterval interval) const
{
    if (m_visualizerWidget)
    {
        return m_visualizerWidget->getLabelSpacingMinutes(interval);
    }
    return 3; // Default
}

std::vector<QDateTime> TimelineView::calculateNavTimeLabels(
    const QDateTime& currentNavTime, TimeInterval interval, const QTime& timelineLength) const
{
    if (m_visualizerWidget)
    {
        return m_visualizerWidget->calculateNavTimeLabels(currentNavTime, interval, timelineLength);
    }
    return std::vector<QDateTime>();
}

double TimelineView::calculateLabelYPosition(
    const QDateTime& labelNavTime, const QDateTime& currentNavTime, 
    const QTime& timelineLength, int widgetHeight) const
{
    if (m_visualizerWidget)
    {
        return m_visualizerWidget->calculateLabelYPosition(labelNavTime, currentNavTime, timelineLength, widgetHeight);
    }
    return 0.0;
}

// Optional rendering control methods for TimelineVisualizerWidget
void TimelineVisualizerWidget::setSliderVisible(bool visible)
{
    if (m_sliderVisible != visible)
    {
        m_sliderVisible = visible;
        update(); // Trigger repaint
    }
}

void TimelineVisualizerWidget::setChevronVisible(bool visible)
{
    if (m_chevronVisible != visible)
    {
        m_chevronVisible = visible;
        update(); // Trigger repaint
    }
}

// Optional rendering control methods for TimelineView (delegate to visualizer widget)
void TimelineView::setSliderVisible(bool visible)
{
    if (m_visualizerWidget)
    {
        m_visualizerWidget->setSliderVisible(visible);
    }
}

bool TimelineView::isSliderVisible() const
{
    if (m_visualizerWidget)
    {
        return m_visualizerWidget->isSliderVisible();
    }
    return true; // Default
}

void TimelineView::setChevronVisible(bool visible)
{
    if (m_visualizerWidget)
    {
        m_visualizerWidget->setChevronVisible(visible);
    }
}

bool TimelineView::isChevronVisible() const
{
    if (m_visualizerWidget)
    {
        return m_visualizerWidget->isChevronVisible();
    }
    return true; // Default
}