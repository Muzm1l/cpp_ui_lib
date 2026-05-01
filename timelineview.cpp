#include "timelineview.h"
#include "navtimeutils.h"
#include "debugutils.h"
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

void SliderState::setYPosition(int y, int widgetHeight, const QTime& interval, bool preserveTimeWindow, const QDateTime& timelineEnd)
{
    int sliderHeight = SliderGeometry::calculateSliderHeight(interval, widgetHeight);
    QPair<int, int> bounds = SliderGeometry::getSliderBounds(widgetHeight, sliderHeight);
    m_yPosition = qBound(bounds.first, y, bounds.second);
    // Only sync time window if not preserving it (for frozen mode)
    // When preserveTimeWindow is true, the time window should be set explicitly via setTimeWindow()
    if (!preserveTimeWindow)
    {
        // Not dragging, so use normal 12-hour range
        syncTimeWindowFromPosition(widgetHeight, interval, QDateTime(), false, timelineEnd);
    }
}

int SliderState::getYPosition() const
{
    return m_yPosition;
}

void SliderState::setTimeWindow(const TimeSelectionSpan& window, int widgetHeight, const QTime& interval, const QDateTime& rangeStart, const QDateTime& timelineEnd)
{
    m_timeWindow = window;
    // Sync position based on end time (top = now); use rangeStart when valid so "recent past" maps correctly
    syncPositionFromTimeWindow(widgetHeight, rangeStart, timelineEnd);
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

void SliderState::updateDrag(const QPoint& mousePos, int widgetHeight, const QTime& interval, const QDateTime& applicationStartTime, const QDateTime& timelineEnd)
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
    
    // Sync time window from new position (with application start time check when dragging)
    syncTimeWindowFromPosition(widgetHeight, interval, applicationStartTime, true, timelineEnd);
}

void SliderState::endDrag(int widgetHeight, const QTime& interval, const QDateTime& applicationStartTime, const QDateTime& timelineEnd)
{
    if (!m_isDragging)
        return;
    
    m_isDragging = false;
    // Final sync to ensure time window is accurate (with application start time check)
    syncTimeWindowFromPosition(widgetHeight, interval, applicationStartTime, true, timelineEnd);
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

void SliderState::syncTimeWindowFromPosition(int widgetHeight, const QTime& interval, const QDateTime& applicationStartTime, bool isDragging, const QDateTime& timelineEnd)
{
    // Calculate time window based on Y position
    QDateTime now = timelineEnd.isValid() ? timelineEnd : QDateTime::currentDateTime();
    
    // When dragging, use range from application start time to current time
    // When not dragging, use normal 12-hour range
    QDateTime rangeStart;
    int totalMinutes;
    
    if (isDragging && applicationStartTime.isValid())
    {
        // Dragging: range from application start to current time
        rangeStart = applicationStartTime;
        totalMinutes = static_cast<int>(rangeStart.msecsTo(now) / 60000);
    }
    else
    {
        // Normal behavior: 12 hours ago to now
        rangeStart = now.addSecs(-12 * 3600);
        totalMinutes = SliderGeometry::getTwelveHoursInMinutes();
    }
    
    // Convert Y position to time ratio (inverted: Y=0 means endTime=now)
    double positionRatio = 1.0 - (static_cast<double>(m_yPosition) / static_cast<double>(widgetHeight));
    int minutesFromStart = static_cast<int>(positionRatio * totalMinutes);
    
    // Calculate window end time (top edge of slider)
    QDateTime windowEnd = rangeStart.addSecs(minutesFromStart * 60);
    
    // Calculate window start time based on interval
    int intervalSeconds = interval.hour() * 3600 + interval.minute() * 60 + interval.second();
    QDateTime windowStart = windowEnd.addSecs(-intervalSeconds);
    
    // CRITICAL FIX: Block slider movement below system start time when dragging
    if (isDragging && applicationStartTime.isValid() && windowStart < applicationStartTime)
    {
        windowStart = applicationStartTime;
        // Recalculate window end to maintain interval, but clamp to not exceed "now"
        windowEnd = windowStart.addSecs(intervalSeconds);
        if (windowEnd > now)
        {
            windowEnd = now;
            // If windowEnd is clamped, adjust windowStart to maintain interval
            windowStart = windowEnd.addSecs(-intervalSeconds);
            // Ensure windowStart doesn't go below application start time
            if (windowStart < applicationStartTime)
            {
                windowStart = applicationStartTime;
            }
        }
        // Sync slider Y position to the clamped window so the thumb cannot be dragged below system start
        if (totalMinutes > 0)
        {
            double positionRatio = static_cast<double>(rangeStart.msecsTo(windowEnd)) / 60000.0 / totalMinutes;
            positionRatio = qBound(0.0, positionRatio, 1.0);
            int sliderHeight = SliderGeometry::calculateSliderHeight(interval, widgetHeight);
            QPair<int, int> bounds = SliderGeometry::getSliderBounds(widgetHeight, sliderHeight);
            m_yPosition = qBound(bounds.first, static_cast<int>((1.0 - positionRatio) * widgetHeight), bounds.second);
        }
    }
    
    m_timeWindow = TimeSelectionSpan(windowStart, windowEnd);
}

void SliderState::syncPositionFromTimeWindow(int widgetHeight, const QDateTime& rangeStart, const QDateTime& timelineEnd)
{
    // Calculate position based on the END time (top represents "now")
    // Use application-start→now when rangeStart is valid so "recent past" windows map to correct Y (not Y≈0)
    QDateTime now = timelineEnd.isValid() ? timelineEnd : QDateTime::currentDateTime();
    QDateTime rangeStartActual = rangeStart.isValid() ? rangeStart : now.addSecs(-12 * 3600);
    int totalMinutes = rangeStart.isValid()
        ? qMax(1, static_cast<int>(rangeStartActual.msecsTo(now) / 60000))
        : SliderGeometry::getTwelveHoursInMinutes();
    
    // Minutes from range start to the window end time
    int minutesFromStart = rangeStartActual.msecsTo(m_timeWindow.endTime) / 60000;
    minutesFromStart = qBound(0, minutesFromStart, totalMinutes);
    
    // Convert to Y position (top = now, bottom = range start)
    double positionRatio = static_cast<double>(minutesFromStart) / static_cast<double>(totalMinutes);
    int calculatedY = static_cast<int>((1.0 - positionRatio) * widgetHeight);
    
    m_yPosition = calculatedY;
}

// ============================================================================
// TimelineVisualizerWidget Implementation
// ============================================================================

TimelineVisualizerWidget::TimelineVisualizerWidget(QWidget *parent, GraphContainerSyncState *syncState, bool sliderVisible, bool chevronVisible)
    : QWidget(parent), m_currentTime(QTime::currentTime()), m_numberOfDivisions(15), m_lastCurrentTime(QTime::currentTime()), m_pixelSpeed(0.0), m_accumulatedOffset(0.0), m_sliderIndicator(nullptr), m_syncState(syncState), m_sliderVisible(sliderVisible), m_chevronVisible(chevronVisible), m_manoeuvreOverlay(nullptr), m_showCrosshairTimestamp(false)
{
    setFixedWidth(TIMELINE_VIEW_GRAPHICS_VIEW_WIDTH);
    setMinimumHeight(50); // Set a minimum height

    // Remove all margins and padding for snug fit
    setContentsMargins(0, 0, 0, 0);

    // Enable mouse tracking for slider interaction
    setMouseTracking(true);

    if (m_syncState && m_syncState->hasApplicationStartTime && m_syncState->applicationStartTime.isValid())
        m_applicationStartTime = m_syncState->applicationStartTime;
    else
        m_applicationStartTime = QDateTime::currentDateTime();
    
    // Initialize slider state: from "(now - interval)" to effective timeline end (default 15 minutes)
    QDateTime now = effectiveTimelineEnd();
    int intervalSeconds = m_timeLineLength.hour() * 3600 + m_timeLineLength.minute() * 60 + m_timeLineLength.second();
    QDateTime startTime = now.addSecs(-intervalSeconds);
    TimeSelectionSpan initialWindow(startTime, now);
    
    m_sliderState.setTimeWindow(initialWindow, rect().height(), m_timeLineLength, getEffectiveRangeStart(), effectiveTimelineEnd());
    m_sliderState.setYPosition(0, rect().height(), m_timeLineLength, false, effectiveTimelineEnd());
    
    // Keep legacy member in sync for now
    m_sliderVisibleWindow = m_sliderState.getTimeWindow();

    // Create drawing objects
    createDrawingObjects();
    
    // Create slider indicator
    createSliderIndicator();
    
    // Create manoeuvre overlay
    m_manoeuvreOverlay = new ManoeuvreOverlay(this);
    m_manoeuvreOverlay->setGeometry(0, 0, TIMELINE_VIEW_GRAPHICS_VIEW_WIDTH, height());
    m_manoeuvreOverlay->raise(); // Ensure overlay is on top
    m_manoeuvreOverlay->show();
    
    // Initialize overlay with manoeuvres from sync state if available
    if (m_syncState && m_syncState->hasManoeuvres)
    {
        m_manoeuvreOverlay->setManoeuvres(&m_syncState->manoeuvres);
    }
    
    // Initialize overlay time range from slider state
    TimeSelectionSpan window = m_sliderState.getTimeWindow();
    if (window.startTime.isValid() && window.endTime.isValid())
    {
        m_manoeuvreOverlay->setTimeRange(window.startTime, window.endTime);
    }
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

    // When interval changes, reset to live window and slider at top (y=0).
    int intervalSeconds = newLength.hour() * 3600 + newLength.minute() * 60 + newLength.second();
    QDateTime now = effectiveTimelineEnd();
    QDateTime startTime = now.addSecs(-intervalSeconds);
    TimeSelectionSpan newWindow(startTime, now);
    m_sliderState.setTimeWindow(newWindow, rect().height(), newLength, getEffectiveRangeStart(), effectiveTimelineEnd());
    m_sliderState.setYPosition(0, rect().height(), newLength, true, effectiveTimelineEnd());
    
    // Keep legacy member in sync
    m_sliderVisibleWindow = m_sliderState.getTimeWindow();
    
    // Recalculate cursor timestamp label position if there's a current cursor timestamp
    // This ensures the label stays aligned with the cursor when interval changes
    if (m_showCrosshairTimestamp && m_crosshairTimestamp.isValid())
    {
        updateCrosshairTimestampFromTime(m_crosshairTimestamp);
    }

    // Recreate all drawing objects with new parameters
    // This will automatically calculate optimal divisions based on current area
    createDrawingObjects();
    
    // Invalidate background cache since interval changed
    m_backgroundNeedsRedraw = true;

    // Force an update to ensure animation continues after interval change
    // This is critical to resume the animation after interval customization
    updateVisualization();

    // Emit signal for the updated time window
    emitTimeScopeChanged();

    // DEBUG_OUT() << "Time interval set to:" << timeIntervalToString(interval)
    //          << "Divisions:" << m_numberOfDivisions
    //          << "Segment duration:" << calculateSegmentDurationSeconds() << "seconds"
    //          << "Min segment height:" << getMinimumSegmentHeight();
}

void TimelineVisualizerWidget::setCurrentTime(const QTime &currentTime)
{
    m_lastCurrentTime = m_currentTime;
    // When frozen, use visible window end so segment labels match the slider (not real time)
    if (m_timelineViewMode == TimelineViewMode::FROZEN_MODE)
    {
        TimeSelectionSpan w = m_sliderState.getTimeWindow();
        if (w.endTime.isValid())
            m_currentTime = w.endTime.time();
        else
            m_currentTime = currentTime;
    }
    else
        m_currentTime = currentTime;
    
    // Only update pixel speed and animate timeline when in follow mode
    // In frozen mode, keep the timeline static (no animation)
    if (m_timelineViewMode == TimelineViewMode::FOLLOW_MODE)
    {
        updatePixelSpeed();
        // Invalidate background cache since smooth offset changes with time
        m_backgroundNeedsRedraw = true;
    }
    
    // Don't update visualization if dragging (preserve dragged position)
    // The slider position will be recalculated when drag ends
    if (!m_sliderState.isDragging())
    {
        // Only update slider position to latest data when in follow mode
        if (m_timelineViewMode == TimelineViewMode::FOLLOW_MODE)
        {
            QDateTime now = effectiveTimelineEnd();
            int intervalSeconds = m_timeLineLength.hour() * 3600 + m_timeLineLength.minute() * 60 + m_timeLineLength.second();
            QDateTime startTime = now.addSecs(-intervalSeconds);
            TimeSelectionSpan newWindow(startTime, now);
            m_sliderState.setTimeWindow(newWindow, rect().height(), m_timeLineLength, getEffectiveRangeStart(), effectiveTimelineEnd());
            m_sliderState.setYPosition(0, rect().height(), m_timeLineLength, false, effectiveTimelineEnd());
            // Keep legacy member in sync
            m_sliderVisibleWindow = m_sliderState.getTimeWindow();
            
            // Update manoeuvre overlay time range
            if (m_manoeuvreOverlay && newWindow.startTime.isValid() && newWindow.endTime.isValid())
            {
                m_manoeuvreOverlay->setTimeRange(newWindow.startTime, newWindow.endTime);
            }
            
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

    // DEBUG_OUT() << "Pixel speed updated:" << m_pixelSpeed << "pixels/sec, time diff:" << timeDiffMs << "ms, accumulated offset:" << m_accumulatedOffset
    //          << "Segment duration:" << segmentDurationSeconds << "seconds";
}

double TimelineVisualizerWidget::calculateSmoothOffset()
{
    // Return the accumulated offset for smooth shifting
    return m_accumulatedOffset;
}

QDateTime TimelineVisualizerWidget::getEffectiveRangeStart() const
{
    if (m_syncState && m_syncState->hasApplicationStartTime && m_syncState->applicationStartTime.isValid())
        return m_syncState->applicationStartTime;
    return m_applicationStartTime;
}

QDateTime TimelineVisualizerWidget::effectiveTimelineEnd() const
{
    if (m_syncState)
        return m_syncState->effectiveTimelineEnd();
    return QDateTime::currentDateTime();
}

void TimelineVisualizerWidget::setSystemStartTime(const QDateTime &t)
{
    if (!t.isValid())
        return;
    m_applicationStartTime = t;
    m_backgroundNeedsRedraw = true;
    m_lastCachedWindowStart = QDateTime();
    if (m_timelineViewMode == TimelineViewMode::FOLLOW_MODE)
    {
        const QDateTime end = effectiveTimelineEnd();
        int intervalSeconds = m_timeLineLength.hour() * 3600 + m_timeLineLength.minute() * 60 + m_timeLineLength.second();
        TimeSelectionSpan newWindow(end.addSecs(-intervalSeconds), end);
        m_sliderState.setTimeWindow(newWindow, rect().height(), m_timeLineLength, getEffectiveRangeStart(), effectiveTimelineEnd());
        m_sliderState.setYPosition(0, rect().height(), m_timeLineLength, true, effectiveTimelineEnd());
        m_sliderVisibleWindow = m_sliderState.getTimeWindow();
        if (m_manoeuvreOverlay && newWindow.startTime.isValid() && newWindow.endTime.isValid())
            m_manoeuvreOverlay->setTimeRange(newWindow.startTime, newWindow.endTime);
        emitTimeScopeChanged();
    }
    else
        m_sliderVisibleWindow = m_sliderState.getTimeWindow();
    updateVisualization();
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
    if (m_manoeuvreOverlay)
    {
        delete m_manoeuvreOverlay;
        m_manoeuvreOverlay = nullptr;
    }
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
        // DEBUG_OUT() << "Widget height is 0, using default height:" << widgetHeight;
    }

    // Use fixed number of segments for all time intervals
    m_numberOfDivisions = getFixedNumberOfSegments();

    QRect drawArea(0, 0, TIMELINE_VIEW_GRAPHICS_VIEW_WIDTH, widgetHeight);

    // Calculate segment height to fill the entire drawing area
    double segmentHeight = static_cast<double>(widgetHeight) / m_numberOfDivisions;

    // DEBUG_OUT() << "Creating drawing objects - Widget height:" << height()
    //          << "Using height:" << widgetHeight
    //          << "Draw area:" << drawArea
    //          << "Fixed divisions:" << m_numberOfDivisions
    //          << "Calculated segment height:" << segmentHeight
    //          << "Time interval:" << timeIntervalToString(m_timeInterval);

    // Create segment drawers for animation range (including off-screen segments)
    clearDrawingObjects(); // Clear existing ones first

    // Create segments with fixed count but variable time gaps
    // We need enough segments to cover the entire visible area plus some buffer
    int segmentsNeeded = m_numberOfDivisions + 10; // Add buffer for smooth animation
    int startSegment = -(segmentsNeeded / 2);
    int endSegment = segmentsNeeded / 2;

    // DEBUG_OUT() << "Creating fixed segments - Height:" << widgetHeight
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
    
    // CRITICAL FIX: Invalidate timestamp cache to force regeneration with new mode
    // The cache stores labels in either absolute or relative format, so when mode changes,
    // we need to regenerate all labels with the new format
    m_cachedTimestampLabels.clear();
    m_lastCachedWindowStart = QDateTime(); // Invalidate cache keys
    m_lastCachedWindowEnd = QDateTime();
    m_backgroundNeedsRedraw = true; // Force background redraw
    update(); // Trigger repaint
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

void TimelineVisualizerWidget::renderBackgroundToCache()
{
    if (rect().isEmpty())
    {
        return;
    }
    
    // Create pixmap matching widget size
    m_cachedBackground = QPixmap(rect().size());
    m_cachedBackground.fill(Qt::black);
    
    QPainter painter(&m_cachedBackground);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // In follow mode use 0 so segment ticks align with the visible time window (and labels).
    // Otherwise the scrolling smoothOffset would misalign ticks with time labels.
    double smoothOffset = (m_timelineViewMode == TimelineViewMode::FOLLOW_MODE)
        ? 0.0
        : calculateSmoothOffset();
    double segmentHeight = static_cast<double>(rect().height()) / m_numberOfDivisions;
    
    // Calculate which segments should be visible
    int firstVisibleSegment = static_cast<int>(-smoothOffset / segmentHeight);
    int lastVisibleSegment = firstVisibleSegment + m_numberOfDivisions;
    
    // Draw segments that are visible (ticks only, no labels)
    for (auto *segmentDrawer : m_segmentDrawers)
    {
        if (segmentDrawer)
        {
            int segmentNumber = segmentDrawer->getSegmentNumber();
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
                
                // Draw the segment using QPainter (ticks only)
                drawSegmentWithPainter(painter, segmentDrawer);
            }
        }
    }
    
    // Draw border
    painter.setPen(QPen(QColor(150, 150, 150), 1));
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
    
    // CRITICAL FIX: Slider is NOT drawn in background cache - it's drawn directly in paintEvent()
    // This ensures immediate updates during drag without expensive cache regeneration
    
    // Draw regular interval timestamps
    drawRegularIntervalTimestamps(painter, rect());
    
    // Draw navtime labels if sync state is available
    if (m_syncState && m_syncState->hasCurrentNavTime)
    {
        drawNavTimeLabels(painter, rect());
    }
    
    m_backgroundNeedsRedraw = false;
}

void TimelineVisualizerWidget::paintEvent(QPaintEvent * /* event */)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // In follow mode use 0 so segment ticks align with the visible time window (and labels).
    double smoothOffset = (m_timelineViewMode == TimelineViewMode::FOLLOW_MODE)
        ? 0.0
        : calculateSmoothOffset();
    double segmentHeight = static_cast<double>(rect().height()) / m_numberOfDivisions;

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
                m_backgroundNeedsRedraw = true; // Invalidate cache when segments change
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
            m_backgroundNeedsRedraw = true; // Invalidate cache when segments change
        }

        // Add segments below the current range if needed
        while (maxSegmentNumber < lastVisibleSegment + 2)
        { // Keep some buffer below
            ++maxSegmentNumber;
            TimelineSegmentDrawer *segmentDrawer = new TimelineSegmentDrawer(
                maxSegmentNumber, m_timeLineLength, m_currentTime, m_numberOfDivisions, m_isAbsoluteTime, rect());
            segmentDrawer->setShowRelativeLabel(m_showRelativeLabels);
            m_segmentDrawers.push_back(segmentDrawer);
            m_backgroundNeedsRedraw = true; // Invalidate cache when segments change
        }
    }

    // Redraw background cache if needed (size changed or content invalidated)
    if (m_backgroundNeedsRedraw || m_cachedBackground.size() != rect().size())
    {
        renderBackgroundToCache();
    }

    // Fast blit of cached background (all static elements)
    painter.drawPixmap(0, 0, m_cachedBackground);
    
    // CRITICAL FIX: Draw slider directly (not in cache) for immediate updates during drag
    // The slider is dynamic and changes position frequently, so it shouldn't be cached
    if (m_sliderVisible)
    {
        QRect sliderRect = SliderGeometry::calculateSliderRect(
            rect().height(), rect().width(), m_timeLineLength,
            m_sliderState.getYPosition());
        QColor sliderColor(255, 255, 255, 128); // 50% opacity white
        painter.fillRect(sliderRect, sliderColor);
    }
    
    // Draw only the crosshair timestamp label on top (lightweight, changes with mouse movement)
    if (m_showCrosshairTimestamp && m_crosshairTimestamp.isValid() && 
        m_crosshairYPosition >= 0 && m_crosshairYPosition <= rect().height())
    {
        drawCrosshairTimestampLabel(painter, rect());
    }
}

void TimelineVisualizerWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    // Recreate drawing objects with new dimensions
    // DEBUG_OUT() << "Widget resized to:" << size();
    createDrawingObjects();
    
    // Invalidate background cache since size changed
    m_backgroundNeedsRedraw = true;
    
    // Update slider state for new size (clamp position to new bounds)
    m_sliderState.clampToBounds(rect().height(), m_timeLineLength);
    
    // Update slider indicator for new size
    updateSliderIndicator();
    
    // Update manoeuvre overlay geometry
    if (m_manoeuvreOverlay)
    {
        m_manoeuvreOverlay->setGeometry(0, 0, width(), height());
    }
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

    // Labels are now drawn by drawRegularIntervalTimestamps() for regular clock intervals
    // This method only draws ticks

    // Draw two ticks which are 15% of the segment width
    int tickWidth = static_cast<int>(drawArea.width() * 0.15);
    int tickY = static_cast<int>(y + segmentHeight / 2);

    // Left tick
    painter.setPen(QPen(QColor(255, 255, 255), 1)); // White pen
    painter.drawLine(0, tickY, tickWidth, tickY);

    // Right tick
    painter.drawLine(drawArea.width(), tickY, drawArea.width() - tickWidth, tickY);
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
    
    // Sync position from current time window (use app start range so recent-past windows map correctly)
    m_sliderState.syncPositionFromTimeWindow(rect().height(), getEffectiveRangeStart(), effectiveTimelineEnd());
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
    
        m_sliderState.updateDrag(currentPos, rect().height(), m_timeLineLength, getEffectiveRangeStart(), effectiveTimelineEnd());
    
    // Keep legacy member in sync
    m_sliderVisibleWindow = m_sliderState.getTimeWindow();
    
    // Trigger immediate repaint to show updated slider position
    repaint();
}

void TimelineVisualizerWidget::updateCrosshairTimestamp(const QDateTime &timestamp, qreal yPosition)
{
    m_crosshairTimestamp = timestamp;
    m_crosshairYPosition = yPosition;
    m_showCrosshairTimestamp = timestamp.isValid();
    
    update(); // Trigger repaint
}

void TimelineVisualizerWidget::updateCrosshairTimestampFromTime(const QDateTime &timestamp)
{
    if (!timestamp.isValid())
    {
        clearCrosshairTimestamp();
        return;
    }
    
    if (rect().height() <= 0)
    {
        clearCrosshairTimestamp();
        return;
    }
    
    // Get the visible time window from the slider state
    // This matches what the waterfall graph uses (timeMax = window.endTime, timeMin = window.startTime)
    TimeSelectionSpan visibleWindow = m_sliderState.getTimeWindow();
    
    if (!visibleWindow.startTime.isValid() || !visibleWindow.endTime.isValid())
    {
        clearCrosshairTimestamp();
        return;
    }
    
    // Use the slider window's time range to match waterfall graph alignment
    // The timeline view shows: top (Y=0) = window.endTime, bottom (Y=height) = window.startTime
    QDateTime windowStart = visibleWindow.startTime;
    QDateTime windowEnd = visibleWindow.endTime;
    
    // Ensure windowStart < windowEnd (start is older, end is newer)
    if (windowStart > windowEnd)
    {
        std::swap(windowStart, windowEnd);
    }
    
    // Check if timestamp is within the visible window
    if (timestamp < windowStart || timestamp > windowEnd)
    {
        clearCrosshairTimestamp();
        return;
    }
    
    // Calculate Y position: windowEnd is at top (Y=0), windowStart is at bottom (Y=height)
    // This matches how waterfall graph maps time: timeMax (top) to timeMin (bottom)
    qint64 totalWindowMs = windowStart.msecsTo(windowEnd);
    if (totalWindowMs <= 0)
    {
        clearCrosshairTimestamp();
        return;
    }
    
    // Calculate how far the timestamp is from the end (top)
    qint64 timeFromEndMs = timestamp.msecsTo(windowEnd);
    
    // Normalize: 0.0 at top (windowEnd), 1.0 at bottom (windowStart)
    qreal normalizedY = static_cast<qreal>(timeFromEndMs) / static_cast<qreal>(totalWindowMs);
    normalizedY = qMax(0.0, qMin(1.0, normalizedY));
    
    // Map to widget height: top (windowEnd) is Y=0, bottom (windowStart) is Y=height
    qreal yPosition = normalizedY * rect().height();
    yPosition = qMax(0.0, qMin(static_cast<qreal>(rect().height()), yPosition));
    
    updateCrosshairTimestamp(timestamp, yPosition);
}

void TimelineVisualizerWidget::clearCrosshairTimestamp()
{
    m_showCrosshairTimestamp = false;
    m_crosshairTimestamp = QDateTime();
    update(); // Trigger repaint
}

void TimelineVisualizerWidget::setVisibleTimeWindow(const TimeSelectionSpan &window)
{
    if (!window.startTime.isValid() || !window.endTime.isValid())
    {
        return;
    }
    
    // CRITICAL FIX: In FROZEN_MODE, ignore external sync calls
    // The slider position and time window should remain frozen as set by the user
    // This prevents signal-based sync from resetting frozen slider positions
    // (e.g., when another timeline view or SCW timeline changes)
    if (m_timelineViewMode == TimelineViewMode::FROZEN_MODE)
    {
        // In frozen mode, do NOT update slider position or time window from external sync
        // The user has explicitly frozen the view, so we preserve it
        // Only update manoeuvre overlay if needed (this doesn't affect slider position)
        if (m_manoeuvreOverlay && window.startTime.isValid() && window.endTime.isValid())
        {
            // Use the current frozen time window, not the synced one
            TimeSelectionSpan currentWindow = m_sliderState.getTimeWindow();
            if (currentWindow.startTime.isValid() && currentWindow.endTime.isValid())
            {
                m_manoeuvreOverlay->setTimeRange(currentWindow.startTime, currentWindow.endTime);
            }
        }
        // Do NOT call updateVisualization() - no changes needed in frozen mode
        return;
    }
    
    // Calculate the interval from the window size to ensure we use the correct interval
    // This is important because TimeScopeChanged might arrive before TimeIntervalChanged
    qint64 windowDurationMs = window.startTime.msecsTo(window.endTime);
    if (windowDurationMs > 0)
    {
        // Calculate the interval in minutes from the window duration
        int windowDurationMinutes = static_cast<int>(windowDurationMs / 60000);
        // Find the closest matching interval
        static const std::vector<TimeInterval> intervals = getValidTimeIntervals();
        TimeInterval calculatedInterval = m_timeInterval; // Default to current
        for (TimeInterval interval : intervals)
        {
            int intervalMinutes = static_cast<int>(interval);
            if (abs(intervalMinutes - windowDurationMinutes) < abs(static_cast<int>(calculatedInterval) - windowDurationMinutes))
            {
                calculatedInterval = interval;
            }
        }
        
        // If the calculated interval differs from current, update it
        // This handles the case where TimeScopeChanged arrives before TimeIntervalChanged
        if (calculatedInterval != m_timeInterval)
        {
            QTime newLength = timeIntervalToQTime(calculatedInterval);
            m_timeLineLength = newLength;
            m_timeInterval = calculatedInterval;
        }
    }
    
    // Update the slider state with the new time window using the correct interval length
    m_sliderState.setTimeWindow(window, rect().height(), m_timeLineLength, getEffectiveRangeStart(), effectiveTimelineEnd());
    
    // Keep legacy member in sync
    m_sliderVisibleWindow = m_sliderState.getTimeWindow();
    
    // OPTIMIZATION: Only invalidate background cache if this is a user-initiated change
    // When syncing from another timeline view, the background doesn't need to be redrawn
    // The cache is only needed when the user drags the slider or changes interval
    // m_backgroundNeedsRedraw = true; // REMOVED: Unnecessary cache clearing during sync
    
    // Update manoeuvre overlay time range
    if (m_manoeuvreOverlay)
    {
        m_manoeuvreOverlay->setTimeRange(window.startTime, window.endTime);
    }
    
    // Recalculate cursor timestamp label position if there's a current cursor timestamp
    // This ensures the label stays aligned with the cursor when the window changes
    if (m_showCrosshairTimestamp && m_crosshairTimestamp.isValid())
    {
        updateCrosshairTimestampFromTime(m_crosshairTimestamp);
    }
    
    // Update the visualization to reflect the new slider position
    updateVisualization();
    
    // Note: We don't emit visibleTimeWindowChanged here to avoid feedback loops
    // when syncing from another timeline view
}

void TimelineVisualizerWidget::setVisibleTimeWindowFromSync(const TimeSelectionSpan &window)
{
    // Same as setVisibleTimeWindow but always apply (no FROZEN_MODE early return).
    // Used when another timeline's slider is dragged so all visible sliders stay in sync
    // and timeline window matches graph range (crosshair timestamp aligns with graph crosshair).
    if (!window.startTime.isValid() || !window.endTime.isValid())
    {
        return;
    }

    // Calculate the interval from the window size to ensure we use the correct interval
    qint64 windowDurationMs = window.startTime.msecsTo(window.endTime);
    if (windowDurationMs > 0)
    {
        int windowDurationMinutes = static_cast<int>(windowDurationMs / 60000);
        static const std::vector<TimeInterval> intervals = getValidTimeIntervals();
        TimeInterval calculatedInterval = m_timeInterval;
        for (TimeInterval interval : intervals)
        {
            int intervalMinutes = static_cast<int>(interval);
            if (abs(intervalMinutes - windowDurationMinutes) < abs(static_cast<int>(calculatedInterval) - windowDurationMinutes))
            {
                calculatedInterval = interval;
            }
        }
        if (calculatedInterval != m_timeInterval)
        {
            QTime newLength = timeIntervalToQTime(calculatedInterval);
            m_timeLineLength = newLength;
            m_timeInterval = calculatedInterval;
        }
    }

    m_sliderState.setTimeWindow(window, rect().height(), m_timeLineLength, getEffectiveRangeStart(), effectiveTimelineEnd());
    m_sliderVisibleWindow = m_sliderState.getTimeWindow();

    if (m_manoeuvreOverlay)
    {
        m_manoeuvreOverlay->setTimeRange(window.startTime, window.endTime);
    }

    if (m_showCrosshairTimestamp && m_crosshairTimestamp.isValid())
    {
        updateCrosshairTimestampFromTime(m_crosshairTimestamp);
    }

    updateVisualization();
}

void TimelineVisualizerWidget::emitTimeScopeChanged(bool forceEmit)
{
    // Get time window from state manager and ensure it's valid before emitting
    TimeSelectionSpan window = m_sliderState.getTimeWindow();
    
    // Keep legacy member in sync
    m_sliderVisibleWindow = window;
    
    if (!window.startTime.isValid() || !window.endTime.isValid())
    {
        return;
    }

    // Ensure endTime is after startTime
    TimeSelectionSpan normalizedWindow = window;
    if (normalizedWindow.startTime > normalizedWindow.endTime)
    {
        normalizedWindow = TimeSelectionSpan(normalizedWindow.endTime, normalizedWindow.startTime);
    }

    // Dedupe by value (epoch milliseconds)
    if (m_hasLastEmittedTimeWindow)
    {
        const qint64 newStartEpoch = normalizedWindow.startTime.toMSecsSinceEpoch();
        const qint64 newEndEpoch = normalizedWindow.endTime.toMSecsSinceEpoch();
        const qint64 lastStartEpoch = m_lastEmittedTimeWindow.startTime.toMSecsSinceEpoch();
        const qint64 lastEndEpoch = m_lastEmittedTimeWindow.endTime.toMSecsSinceEpoch();

        if (newStartEpoch == lastStartEpoch && newEndEpoch == lastEndEpoch)
        {
            return;
        }
    }

    // Throttle high-frequency drag updates; always allow forced emits
    if (!forceEmit && m_sliderState.isDragging() && m_timeScopeEmitTimer.isValid())
    {
        if (m_timeScopeEmitTimer.elapsed() < DRAG_EMIT_THROTTLE_MS)
        {
            return;
        }
    }

    m_lastEmittedTimeWindow = normalizedWindow;
    m_hasLastEmittedTimeWindow = true;
    m_timeScopeEmitTimer.restart();
    emit visibleTimeWindowChanged(normalizedWindow);
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
            // When slider is dragged, enter frozen mode immediately so graph drawing stops updating
            m_timelineViewMode = TimelineViewMode::FROZEN_MODE;
            emit timelineViewModeChanged(TimelineViewMode::FROZEN_MODE);
            setCursor(Qt::ClosedHandCursor);
            DEBUG_OUT() << "Slider drag started at Y:" << pos.y() << "Slider Y:" << m_sliderState.getYPosition() << "- FROZEN_MODE";
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
        m_sliderState.updateDrag(event->pos(), rect().height(), m_timeLineLength, getEffectiveRangeStart(), effectiveTimelineEnd());
        
        // Keep legacy member in sync
        m_sliderVisibleWindow = m_sliderState.getTimeWindow();
        
        // CRITICAL FIX: Don't invalidate background cache during drag
        // Slider is now drawn directly in paintEvent(), so cache doesn't need regeneration
        // This improves performance during drag operations
        
        // Update manoeuvre overlay time range during drag
        if (m_manoeuvreOverlay)
        {
            TimeSelectionSpan window = m_sliderState.getTimeWindow();
            if (window.startTime.isValid() && window.endTime.isValid())
            {
                m_manoeuvreOverlay->setTimeRange(window.startTime, window.endTime);
            }
        }
        
        // Recalculate cursor timestamp label position if there's a current cursor timestamp
        // This ensures the label stays aligned with the cursor during drag
        if (m_showCrosshairTimestamp && m_crosshairTimestamp.isValid())
        {
            updateCrosshairTimestampFromTime(m_crosshairTimestamp);
        }
        
        // Trigger immediate repaint for smooth dragging
        repaint();
        
        // Emit signal during drag
        emitTimeScopeChanged(true);
        
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
        m_sliderState.endDrag(rect().height(), m_timeLineLength, getEffectiveRangeStart(), effectiveTimelineEnd());
        
        // Check if slider is at the top (Y=0 or very close to it)
        int sliderY = m_sliderState.getYPosition();
        const int SNAP_THRESHOLD = 5; // pixels - threshold for snapping to top
        
        if (sliderY <= SNAP_THRESHOLD)
        {
            // Snap slider to top and switch to follow mode
            m_sliderState.setYPosition(0, rect().height(), m_timeLineLength, false, effectiveTimelineEnd());
            m_timelineViewMode = TimelineViewMode::FOLLOW_MODE;
            
            QDateTime now = effectiveTimelineEnd();
            int intervalSeconds = m_timeLineLength.hour() * 3600 + m_timeLineLength.minute() * 60 + m_timeLineLength.second();
            QDateTime startTime = now.addSecs(-intervalSeconds);
            TimeSelectionSpan newWindow(startTime, now);
            m_sliderState.setTimeWindow(newWindow, rect().height(), m_timeLineLength, getEffectiveRangeStart(), effectiveTimelineEnd());
            
            // Emit signal to notify parent TimelineView
            emit timelineViewModeChanged(TimelineViewMode::FOLLOW_MODE);
            
            DEBUG_OUT() << "Slider snapped to top - switched to FOLLOW_MODE";
        }
        else
        {
            // Slider is not at top, switch to frozen mode
            m_timelineViewMode = TimelineViewMode::FROZEN_MODE;
            
            // CRITICAL FIX: Preserve the time window that was set during drag
            // Don't recalculate it - endDrag() already set it correctly using the application start time range
            // The time window is already correct from endDrag(), so we just need to ensure it's preserved
            // No need to call setYPosition or setTimeWindow here - they would recalculate incorrectly
            
            // Emit signal to notify parent TimelineView
            emit timelineViewModeChanged(TimelineViewMode::FROZEN_MODE);
            
            DEBUG_OUT() << "Slider not at top - switched to FROZEN_MODE at Y:" << sliderY
                     << "Time window:" << m_sliderState.getTimeWindow().startTime.toString("HH:mm:ss")
                     << "to" << m_sliderState.getTimeWindow().endTime.toString("HH:mm:ss");
        }
        
        // Keep legacy member in sync
        m_sliderVisibleWindow = m_sliderState.getTimeWindow();
        
        // Invalidate background cache since slider position changed
        m_backgroundNeedsRedraw = true;
        
        // Update manoeuvre overlay time range after drag ends
        if (m_manoeuvreOverlay)
        {
            TimeSelectionSpan window = m_sliderState.getTimeWindow();
            if (window.startTime.isValid() && window.endTime.isValid())
            {
                m_manoeuvreOverlay->setTimeRange(window.startTime, window.endTime);
            }
        }
        
        // Recalculate cursor timestamp label position after drag ends
        // This ensures the label is correctly positioned with the final window
        if (m_showCrosshairTimestamp && m_crosshairTimestamp.isValid())
        {
            updateCrosshairTimestampFromTime(m_crosshairTimestamp);
        }
        
        setCursor(Qt::ArrowCursor);
        
        // Emit signal when drag is complete (final update)
        emitTimeScopeChanged();
        emit timeScopeCommitted(m_sliderVisibleWindow);
        DEBUG_OUT() << "Slider drag ended - Final window:" 
                 << m_sliderVisibleWindow.startTime.toString("HH:mm:ss") 
                 << "to" << m_sliderVisibleWindow.endTime.toString("HH:mm:ss");
        
        // Force visualization update to resume animation after drag ends
        // This is critical - we need to explicitly resume the animation
        updateVisualization();
    }
    QWidget::mouseReleaseEvent(event);
}

void TimelineVisualizerWidget::enterEvent(QEvent* event)
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
        QDateTime now = effectiveTimelineEnd();
        int intervalSeconds = m_timeLineLength.hour() * 3600 + m_timeLineLength.minute() * 60 + m_timeLineLength.second();
        QDateTime startTime = now.addSecs(-intervalSeconds);
        TimeSelectionSpan newWindow(startTime, now);
        m_sliderState.setTimeWindow(newWindow, rect().height(), m_timeLineLength, getEffectiveRangeStart(), effectiveTimelineEnd());
        m_sliderState.setYPosition(0, rect().height(), m_timeLineLength, false, effectiveTimelineEnd());
        // Keep legacy member in sync
        m_sliderVisibleWindow = m_sliderState.getTimeWindow();
        
        // Invalidate background cache since slider position changed
        m_backgroundNeedsRedraw = true;
        
        // Update manoeuvre overlay time range
        if (m_manoeuvreOverlay && newWindow.startTime.isValid() && newWindow.endTime.isValid())
        {
            m_manoeuvreOverlay->setTimeRange(newWindow.startTime, newWindow.endTime);
        }
        
        emitTimeScopeChanged();
        update();
    }
}

void TimelineVisualizerWidget::setTimeWindowSilent(const TimeSelectionSpan& window)
{
    // CRITICAL FIX: In FROZEN_MODE, ignore external sync calls
    // The slider position and time window should remain frozen as set by the user
    // This prevents GraphLayout sync from resetting frozen slider positions
    if (m_timelineViewMode == TimelineViewMode::FROZEN_MODE)
    {
        // In frozen mode, do NOT update slider position or time window from external sync
        // The user has explicitly frozen the view, so we preserve it
        // Only update manoeuvre overlay if needed (this doesn't affect slider position)
        if (m_manoeuvreOverlay && window.startTime.isValid() && window.endTime.isValid())
        {
            // Use the current frozen time window, not the synced one
            TimeSelectionSpan currentWindow = m_sliderState.getTimeWindow();
            if (currentWindow.startTime.isValid() && currentWindow.endTime.isValid())
            {
                m_manoeuvreOverlay->setTimeRange(currentWindow.startTime, currentWindow.endTime);
            }
        }
        // Do NOT call updateVisualization() - no changes needed in frozen mode
        return;
    }
    
    // In FOLLOW_MODE, update slider state with new time window (this will sync the position)
    m_sliderState.setTimeWindow(window, rect().height(), m_timeLineLength, getEffectiveRangeStart(), effectiveTimelineEnd());
    
    // Keep legacy member in sync
    m_sliderVisibleWindow = m_sliderState.getTimeWindow();
    
    // Invalidate background cache since time window changed
    m_backgroundNeedsRedraw = true;
    
    // Update manoeuvre overlay time range
    if (m_manoeuvreOverlay && window.startTime.isValid() && window.endTime.isValid())
    {
        m_manoeuvreOverlay->setTimeRange(window.startTime, window.endTime);
    }
    
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

void TimelineVisualizerWidget::drawCrosshairTimestampLabel(QPainter& painter, const QRect& drawArea)
{
    if (!m_showCrosshairTimestamp)
    {
        return;
    }
    
    if (!m_crosshairTimestamp.isValid())
    {
        return;
    }
    
    // Check if Y position is within visible area (allow boundary values)
    if (m_crosshairYPosition < 0 || m_crosshairYPosition > drawArea.height())
    {
        return;
    }
    
    // Format the timestamp as HH:mm:ss.zzz
    QString labelText = m_crosshairTimestamp.toString("HH:mm:ss");
    
    // Set text color to yellow for visibility (different from navtime labels)
    painter.setPen(QPen(QColor(255, 255, 0), 1)); // Yellow
    
    QFontMetrics fm(painter.font());
    int textWidth = fm.horizontalAdvance(labelText);
    int textHeight = fm.height();
    
    // Position the label at the crosshair Y position
    // Center it horizontally in the draw area
    int centerX = (drawArea.width() - textWidth) / 2;
    int centerY = static_cast<int>(m_crosshairYPosition + textHeight / 2);
    
    // Draw a background rectangle for better visibility
    int padding = 2;
    QRect bgRect(centerX - padding, centerY - textHeight - padding, 
                 textWidth + (2 * padding), textHeight + (2 * padding));
    painter.fillRect(bgRect, QColor(0, 0, 0, 200)); // Semi-transparent black background
    
    // Draw the timestamp text
    painter.drawText(QPoint(centerX, centerY), labelText);
}

void TimelineVisualizerWidget::updateCachedTimestampLabels()
{
    m_cachedTimestampLabels.clear();
    
    TimeSelectionSpan visibleWindow = m_sliderState.getTimeWindow();
    if (!visibleWindow.startTime.isValid() || !visibleWindow.endTime.isValid())
    {
        return;
    }
    
    QDateTime windowStart = visibleWindow.startTime;
    QDateTime windowEnd = visibleWindow.endTime;
    if (windowStart > windowEnd)
    {
        std::swap(windowStart, windowEnd);
    }
    
    qint64 windowDurationMs = windowStart.msecsTo(windowEnd);
    if (windowDurationMs <= 0)
    {
        return;
    }
    
    // Helper to add a label to cache
    auto addLabel = [&](const QDateTime& time) {
        if (time >= windowStart && time <= windowEnd)
        {
            qint64 timeFromEndMs = time.msecsTo(windowEnd);
            qreal normalizedY = static_cast<qreal>(timeFromEndMs) / static_cast<qreal>(windowDurationMs);
            normalizedY = qMax(0.0, qMin(1.0, normalizedY));
            
            CachedTimestampLabel label;
            
            // CRITICAL FIX: Generate relative or absolute labels based on mode
            if (m_showRelativeLabels)
            {
                // Relative to "now" (current time) so when slider is dragged to e.g. 1h ago,
                // labels show -01:00, -01:15 etc., not 00:00 to -00:15 from window end.
                QDateTime now = effectiveTimelineEnd();
                qint64 diffMs = time.msecsTo(now);
                qint64 diffSeconds = diffMs / 1000;
                int diffHours = static_cast<int>(diffSeconds / 3600);
                int diffMinutes = static_cast<int>((diffSeconds % 3600) / 60);
                
                // Format as "-HH:MM" or "+HH:MM"
                if (diffSeconds >= 0)
                {
                    label.text = QString("+%1:%2")
                        .arg(diffHours, 2, 10, QChar('0'))
                        .arg(diffMinutes, 2, 10, QChar('0'));
                }
                else
                {
                    label.text = QString("-%1:%2")
                        .arg(-diffHours, 2, 10, QChar('0'))
                        .arg(-diffMinutes, 2, 10, QChar('0'));
                }
            }
            else
            {
                // Absolute time format
                label.text = time.toString("HH:mm");
            }
            
            label.normalizedY = normalizedY;
            m_cachedTimestampLabels.push_back(label);
        }
    };
    
    // Add application start time as anchor
    if (m_applicationStartTime.isValid())
    {
        addLabel(m_applicationStartTime);
    }
    
    // Calculate regular interval labels
    QDateTime effectiveWindowStart = windowStart;
    if (m_applicationStartTime.isValid() && windowStart < m_applicationStartTime)
    {
        effectiveWindowStart = m_applicationStartTime;
    }
    
    int spacingMinutes = getLabelSpacingMinutes(m_timeInterval);
    qint64 spacingSeconds = spacingMinutes * 60;
    
    qint64 startSeconds = effectiveWindowStart.toMSecsSinceEpoch() / 1000;
    qint64 firstLabelSeconds = ((startSeconds / spacingSeconds) + 1) * spacingSeconds;
    
    QDateTime labelTime = QDateTime::fromMSecsSinceEpoch(firstLabelSeconds * 1000);
    QDateTime endTime = windowEnd.addSecs(spacingSeconds);
    
    while (labelTime <= endTime)
    {
        addLabel(labelTime);
        labelTime = labelTime.addSecs(spacingSeconds);
    }
    
    m_lastCachedWindowStart = windowStart;
    m_lastCachedWindowEnd = windowEnd;
    m_lastCachedShowRelativeLabels = m_showRelativeLabels; // Track mode for cache invalidation
}

void TimelineVisualizerWidget::drawRegularIntervalTimestamps(QPainter& painter, const QRect& drawArea)
{
    // Check if cache needs update (time window changed OR mode changed)
    TimeSelectionSpan visibleWindow = m_sliderState.getTimeWindow();
    if (visibleWindow.startTime != m_lastCachedWindowStart || 
        visibleWindow.endTime != m_lastCachedWindowEnd ||
        m_showRelativeLabels != m_lastCachedShowRelativeLabels) // CRITICAL FIX: Check mode change
    {
        updateCachedTimestampLabels();
    }
    
    if (m_cachedTimestampLabels.empty())
    {
        return;
    }
    
    // Draw cached labels - fast path (no QDateTime operations)
    painter.setPen(QPen(QColor(255, 255, 255), 1));
    QFontMetrics fm(painter.font());
    int textHeight = fm.height();
    
    for (const auto& label : m_cachedTimestampLabels)
    {
        int yPosition = static_cast<int>(label.normalizedY * drawArea.height());
        int textWidth = fm.horizontalAdvance(label.text);
        int centerX = (drawArea.width() - textWidth) / 2;
        int centerY = yPosition + textHeight / 4;
        
        painter.drawText(QPoint(centerX, centerY), label.text);
    }
}

TimelineView::TimelineView(QWidget *parent, QTimer *timer, GraphContainerSyncState *syncState, bool sliderVisible, bool chevronVisible)
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
    m_visualizerWidget = new TimelineVisualizerWidget(this, m_syncState, sliderVisible, chevronVisible);

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
        connect(m_visualizerWidget, &TimelineVisualizerWidget::timeScopeCommitted,
                this, &TimelineView::TimeScopeCommitted);
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
    // Initialize intervalIndex to 0 (FifteenMinutes is the first interval)
    static const std::vector<TimeInterval> intervals = getValidTimeIntervals();
    intervalIndex = 0; // FifteenMinutes is the first interval in the list
    
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

    // Connect timer to our tick handler with UniqueConnection to prevent duplicates
    // This ensures the animation continues even after timeline view customization
    connect(m_timer, &QTimer::timeout, this, &TimelineView::onTimerTick, Qt::UniqueConnection);

    // Ensure timer is started (in case it was stopped during customization)
    if (!m_timer->isActive())
    {
        m_timer->start();
    }

    // DEBUG_OUT() << "TimelineView: Timer setup completed - interval:" << m_timer->interval() << "ms";
}

void TimelineView::onTimerTick()
{
    // Update current time to the visualizer widget
    QTime currentTime = QTime::currentTime();

    if (m_visualizerWidget)
    {
        m_visualizerWidget->setCurrentTime(currentTime);
    }
    
    // Update absolute/relative time mode from sync state if available
    if (m_syncState && m_syncState->hasAbsoluteTime)
    {
        if (m_isAbsoluteTime != m_syncState->isAbsoluteTime)
        {
            // Update without emitting signal to avoid feedback loop
            m_isAbsoluteTime = m_syncState->isAbsoluteTime;
            m_visualizerWidget->setShowRelativeLabels(!m_isAbsoluteTime);
            updateTimeModeButtonText(m_isAbsoluteTime);
        }
    }
    
    // Update time interval from sync state if available
    if (m_syncState && m_syncState->hasInterval && m_visualizerWidget)
    {
        // Get current interval from visualizer widget
        TimeInterval currentInterval = m_visualizerWidget->getTimeInterval();
        if (currentInterval != m_syncState->currentInterval)
        {
            // Update without emitting signal to avoid feedback loop
            setTimeLineLength(m_syncState->currentInterval);
        }
    }

    // DEBUG_OUT() << "TimelineView: Timer tick - updated current time to" << currentTime.toString();
}

void TimelineView::ensureTimerRunning()
{
    if (m_timer && !m_timer->isActive())
    {
        m_timer->start();
        DEBUG_OUT() << "TimelineView: Timer restarted to resume animation";
    }
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

    // Advance to next interval using member variable (not static) for per-instance state
    intervalIndex = (intervalIndex + 1) % intervals.size();
    TimeInterval nextInterval = intervals[intervalIndex];
    m_visualizerWidget->setTimeInterval(nextInterval);
    updateButtonText(nextInterval);

    // Update sync state if available
    if (m_syncState)
    {
        m_syncState->currentInterval = nextInterval;
        m_syncState->hasInterval = true;
    }

    // Trigger the intervalChanged signal
    emit TimeIntervalChanged(nextInterval);
}

void TimelineView::onTimeModeButtonClicked()
{
    m_isAbsoluteTime = !m_isAbsoluteTime;
    m_visualizerWidget->setShowRelativeLabels(!m_isAbsoluteTime);
    updateTimeModeButtonText(m_isAbsoluteTime);
    
    // Update sync state if available
    if (m_syncState)
    {
        m_syncState->isAbsoluteTime = m_isAbsoluteTime;
        m_syncState->hasAbsoluteTime = true;
    }
    
    // Emit signal to sync with other timeline views
    emit AbsoluteTimeModeChanged(m_isAbsoluteTime);
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
        // Update sync state if available (for sync state pattern)
        if (m_syncState)
        {
            m_syncState->currentTimeScope = selection;
            m_syncState->hasTimeScope = true;
        }
        
        emit TimeScopeChanged(selection, m_timelineViewMode == TimelineViewMode::FROZEN_MODE);
    }


    //---- syed ------------------------------rebase conflict
    // Ensure timer is running after window changes (e.g., after slider drag)
    // This is critical to resume animation after user interactions
    ensureTimerRunning();
}

void TimelineView::updateCrosshairTimestamp(const QDateTime &timestamp, qreal yPosition)
{
    if (m_visualizerWidget)
    {
        m_visualizerWidget->updateCrosshairTimestamp(timestamp, yPosition);
    }
}

void TimelineView::updateCrosshairTimestampFromTime(const QDateTime &timestamp)
{
    if (m_visualizerWidget)
    {
        m_visualizerWidget->updateCrosshairTimestampFromTime(timestamp);
    }
}

void TimelineView::clearCrosshairTimestamp()
{
    if (m_visualizerWidget)
    {
        m_visualizerWidget->clearCrosshairTimestamp();
    }
}

void TimelineView::setVisibleTimeWindow(const TimeSelectionSpan &window)
{
    if (m_visualizerWidget)
    {
        m_visualizerWidget->setVisibleTimeWindow(window);
    }
}

void TimelineView::setVisibleTimeWindowFromSync(const TimeSelectionSpan &window)
{
    if (m_visualizerWidget)
    {
        m_visualizerWidget->setVisibleTimeWindowFromSync(window);
    }
}

void TimelineView::onTimeScopeChangedFromOtherTimeline(const TimeSelectionSpan &selection, bool fromFrozenUserDrag)
{
    if (fromFrozenUserDrag)
    {
        setVisibleTimeWindowFromSync(selection);
        // Keep this timeline frozen at the synced window so the next timer tick does not revert it to live
        setTimelineViewMode(TimelineViewMode::FROZEN_MODE);
    }
    else
        setVisibleTimeWindow(selection);
}

TimeSelectionSpan TimelineView::getVisibleTimeWindow() const
{
    if (m_visualizerWidget)
    {
        return m_visualizerWidget->getVisibleTimeWindow();
    }
    return TimeSelectionSpan();
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
    
    // CRITICAL FIX: Update sync state immediately so other containers see the change
    // This prevents the sync state from being out of sync and causing mode resets
    if (m_syncState)
    {
        m_syncState->isGraphContainerInFollowMode = (mode == TimelineViewMode::FOLLOW_MODE);
    }

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

QDateTime TimelineView::getApplicationStartTime() const
{
    if (m_visualizerWidget)
    {
        return m_visualizerWidget->getApplicationStartTime();
    }
    return QDateTime();
}

void TimelineView::setSystemStartTime(const QDateTime &t)
{
    if (m_visualizerWidget)
        m_visualizerWidget->setSystemStartTime(t);
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

    // CRITICAL FIX: Update sync state immediately (same as onTimelineViewModeChanged)
    // This prevents the sync state from being out of sync and causing mode resets
    if (m_syncState)
    {
        m_syncState->isGraphContainerInFollowMode = (mode == TimelineViewMode::FOLLOW_MODE);
    }

    // Handle mode transition logic (only once, with correct mode)
    handleModeTransitionLogic(mode);
    
    // Emit signal for mode change (for consistency with onTimelineViewModeChanged)
    bool isInFollowMode = (mode == TimelineViewMode::FOLLOW_MODE);
    emit GraphContainerInFollowModeChanged(isInFollowMode);
}

void TimelineView::onOtherContainerEnteredFollowMode(bool isInFollowMode)
{
    if (!isInFollowMode)
        return;
    // Already in follow mode: avoid re-entrant setTimelineViewMode (prevents cascade and QTextLayout re-entry segfault)
    if (m_timelineViewMode == TimelineViewMode::FOLLOW_MODE)
        return;
    setTimelineViewMode(TimelineViewMode::FOLLOW_MODE);
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

void TimelineView::setManoeuvres(const std::vector<Manoeuvre> *manoeuvres)
{
    if (m_visualizerWidget)
    {
        m_visualizerWidget->setManoeuvres(manoeuvres);
    }
}

void TimelineView::setInProgressManoeuvre(const QDateTime &startTime)
{
    if (m_visualizerWidget)
    {
        m_visualizerWidget->setInProgressManoeuvre(startTime);
    }
}

void TimelineView::clearInProgressManoeuvre()
{
    if (m_visualizerWidget)
    {
        m_visualizerWidget->clearInProgressManoeuvre();
    }
}

void TimelineView::setIsAbsoluteTime(bool isAbsoluteTime)
{
    // Only update if the value has changed to avoid unnecessary updates
    if (m_isAbsoluteTime != isAbsoluteTime)
    {
        m_isAbsoluteTime = isAbsoluteTime;
        m_visualizerWidget->setShowRelativeLabels(!m_isAbsoluteTime);
        updateTimeModeButtonText(m_isAbsoluteTime);
        
        // Update sync state if available
        if (m_syncState)
        {
            m_syncState->isAbsoluteTime = m_isAbsoluteTime;
            m_syncState->hasAbsoluteTime = true;
        }
    }
}

void TimelineVisualizerWidget::setManoeuvres(const std::vector<Manoeuvre> *manoeuvres)
{
    if (m_manoeuvreOverlay)
    {
        m_manoeuvreOverlay->setManoeuvres(manoeuvres);
        
        // Update time range from current slider state
        TimeSelectionSpan window = m_sliderState.getTimeWindow();
        if (window.startTime.isValid() && window.endTime.isValid())
        {
            m_manoeuvreOverlay->setTimeRange(window.startTime, window.endTime);
        }
    }
}

void TimelineVisualizerWidget::setInProgressManoeuvre(const QDateTime &startTime)
{
    if (m_manoeuvreOverlay)
    {
        m_manoeuvreOverlay->setInProgressManoeuvre(startTime);
    }
}

void TimelineVisualizerWidget::clearInProgressManoeuvre()
{
    if (m_manoeuvreOverlay)
    {
        m_manoeuvreOverlay->clearInProgressManoeuvre();
    }
}