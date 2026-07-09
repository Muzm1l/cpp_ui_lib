#include "timeselectionvisualizer.h"
#include "debugutils.h"
#include <QDebug>
#include <algorithm>

TimeVisualizerWidget::TimeVisualizerWidget(QWidget* parent)
    : QWidget(parent)
    , m_timeLineLength(QTime(0, 0, 0))
    , m_currentTime(QTime(0, 0, 0))
    , m_validStartDateTime(QDateTime())
    , m_validEndDateTime(QDateTime())
    , m_isSelecting(false)
    , m_selectionStartY(0)
    , m_selectionEndY(0)
    , m_interactionMode(InteractionMode::None)
    , m_interactionSelectionIndex(-1)
    , m_dragStartY(0)
{
    setFixedWidth(GRAPHICS_VIEW_WIDTH);
    setMinimumHeight(50); // Set a minimum height

    // Remove all margins and padding for snug fit
    setContentsMargins(0, 0, 0, 0);
}


void TimeVisualizerWidget::drawSelection(QPainter& painter, const TimeSelectionSpan& span)
{
    // First get the draw area
    QRect drawArea = rect();
    int widgetHeight = drawArea.height();
    int widgetWidth = drawArea.width();

    // Calculate the total timeline duration in seconds
    int totalSeconds = m_timeLineLength.hour() * 3600 + m_timeLineLength.minute() * 60 + m_timeLineLength.second();

    if (totalSeconds <= 0 || widgetHeight <= 0) {
        return; // Invalid parameters
    }

    // Calculate pixels per second
    double pixelsPerSecond = static_cast<double>(widgetHeight) / totalSeconds;

    // Get current time and selection times in seconds
    int currentTimeSeconds = m_currentTime.hour() * 3600 + m_currentTime.minute() * 60 + m_currentTime.second();
    
    // Extract time portion from QDateTime for comparison with QTime-based calculations
    QTime selectionStartTime = span.startTime.time();
    QTime selectionEndTime = span.endTime.time();
    int selectionStartSeconds = selectionStartTime.hour() * 3600 + selectionStartTime.minute() * 60 + selectionStartTime.second();
    int selectionEndSeconds = selectionEndTime.hour() * 3600 + selectionEndTime.minute() * 60 + selectionEndTime.second();

    // Calculate the visible time range (currentTime is at top, currentTime-timespan is at bottom)
    int timeSpanStartSeconds = currentTimeSeconds - totalSeconds;

    // Check if selection overlaps with visible range
    if (selectionEndSeconds >= timeSpanStartSeconds && selectionStartSeconds <= currentTimeSeconds) {
        // Calculate Y positions relative to currentTime (top of widget)
        int topY = static_cast<int>((currentTimeSeconds - selectionEndSeconds) * pixelsPerSecond);
        int bottomY = static_cast<int>((currentTimeSeconds - selectionStartSeconds) * pixelsPerSecond);

        // Clamp to widget bounds
        topY = qMax(0, qMin(widgetHeight, topY));
        bottomY = qMax(0, qMin(widgetHeight, bottomY));

        // Ensure the rectangle is at least 1 pixel high
        int rectHeight = qMax(1, bottomY - topY);

        // Inset horizontally so the selection's own (inner) border sits inside the
        // component's outer border, producing a double-border look on the sides.
        int selX = SELECTION_SIDE_INSET;
        int selW = qMax(1, widgetWidth - 2 * SELECTION_SIDE_INSET);

        // The selection itself is white against the black component background.
        painter.fillRect(selX, topY, selW, rectHeight, QColor(255, 255, 255));

        // Inner border for the selection.
        painter.setPen(QPen(QColor(120, 120, 120), 1));
        painter.drawRect(selX, topY, selW - 1, rectHeight - 1);
    }
}

void TimeVisualizerWidget::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Fill with black background
    painter.fillRect(rect(), QColor(0, 0, 0));

    // Draw time selection rectangles
    if (!m_timeSelections.isEmpty() && !m_timeLineLength.isNull() && !m_currentTime.isNull()) {
        for (const TimeSelectionSpan& span : m_timeSelections) {
            drawSelection(painter, span);
        }
    }

    // Draw current selection being made
    if (m_isSelecting) {
        drawCurrentSelection(painter);
    }

    // Draw the component's outer border (white) so the selection's inner border
    // reads as a distinct double border on the sides.
    painter.setPen(QPen(QColor(255, 255, 255), 1));
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
}

bool TimeVisualizerWidget::addTimeSelection(TimeSelectionSpan span)
{
    // Normalize order
    if (span.startTime > span.endTime) {
        std::swap(span.startTime, span.endTime);
    }

    // Clamp to valid range if configured
    if (hasValidRange()) {
        span = clampToValidRange(span);
        // If clamped becomes invalid (end before start), ignore
        if (span.endTime < span.startTime) {
            return false;
        }
    }

    // Merge with overlapping selections (compute union and remove overlaps)
    TimeSelectionSpan merged = span;
    QList<int> indicesToRemove;
    for (int i = 0; i < m_timeSelections.size(); ++i) {
        const TimeSelectionSpan &existing = m_timeSelections[i];
        const bool overlaps = !(existing.endTime < merged.startTime || merged.endTime < existing.startTime);
        if (overlaps) {
            if (existing.startTime < merged.startTime) merged.startTime = existing.startTime;
            if (existing.endTime > merged.endTime) merged.endTime = existing.endTime;
            indicesToRemove.append(i);
        }
    }

    // Clamp merged selection to valid range if configured
    if (hasValidRange()) {
        merged = clampToValidRange(merged);
        // If merged becomes invalid after clamping, ignore
        if (merged.endTime < merged.startTime) {
            return false;
        }
    }

    // Remove overlapped selections (from highest index down)
    std::sort(indicesToRemove.begin(), indicesToRemove.end(), std::greater<int>());
    for (int idx : indicesToRemove) {
        m_timeSelections.removeAt(idx);
    }

    // Stop adding new selections once we reach the maximum (instead of FIFO)
    if (m_timeSelections.size() >= MAX_TIME_SELECTIONS) {
        return false;
    }

    // Assign a stable identity on creation (preserved through resize/drag/sync)
    // so the main system can address this exact selection later.
    if (merged.id.isNull())
        merged.id = QUuid::createUuid();

    m_timeSelections.append(merged);
    updateVisualization();
    return true;
}

void TimeVisualizerWidget::setTimeSelection(int index, const TimeSelectionSpan& span)
{
    if (index < 0 || index >= m_timeSelections.size()) return;
    TimeSelectionSpan s = span;
    if (s.startTime > s.endTime) std::swap(s.startTime, s.endTime);
    if (hasValidRange()) s = clampToValidRange(s);
    if (s.endTime < s.startTime) return;
    m_timeSelections[index] = s;
    updateVisualization();
}

void TimeVisualizerWidget::clearTimeSelections()
{
    m_timeSelections.clear();
    updateVisualization();
}

void TimeVisualizerWidget::createFullSelection()
{
    // Determine the full range to select: valid range if set, otherwise full visualizer range
    QDateTime currentDate = QDateTime::currentDateTime();
    TimeSelectionSpan span;
    
    if (hasValidRange()) {
        span.startTime = m_validStartDateTime;
        span.endTime = m_validEndDateTime;
    } else {
        // Map the entire widget: bottom corresponds to oldest (height), top to current (0)
        const int bottomY = rect().height();
        const int topY = 0;
        QTime startTime = yCoordinateToTime(bottomY);
        QTime endTime = yCoordinateToTime(topY);
        if (startTime > endTime) std::swap(startTime, endTime);
        
        span.startTime = QDateTime(currentDate.date(), startTime);
        span.endTime = QDateTime(currentDate.date(), endTime);
    }

    addTimeSelection(span);
    emit timeSelectionMade(span);
    update();
}

void TimeVisualizerWidget::createIntervalSelection()
{
    // Anchor policy (no BTW line): span is exactly one timeline interval long,
    // ending at the current time. Clamped to the valid data range if configured.
    const int totalSeconds = m_timeLineLength.hour() * 3600 + m_timeLineLength.minute() * 60 + m_timeLineLength.second();
    if (totalSeconds <= 0) {
        // No usable interval configured; fall back to the full range behaviour.
        createFullSelection();
        return;
    }

    const QDateTime end = QDateTime::currentDateTime();
    const QDateTime start = end.addSecs(-totalSeconds);

    TimeSelectionSpan span(start, end);
    if (hasValidRange()) {
        span = clampToValidRange(span);
        if (span.endTime < span.startTime)
            return;
    }

    addTimeSelection(span);
    emit timeSelectionMade(span);
    update();
}

void TimeVisualizerWidget::setTimeLineLength(const QTime& length)
{
    m_timeLineLength = length;
    updateVisualization();
}

void TimeVisualizerWidget::setTimeLineLength(TimeInterval interval)
{
    m_timeLineLength = timeIntervalToQTime(interval);
    updateVisualization();
}

void TimeVisualizerWidget::setCurrentTime(const QTime& currentTime)
{
    m_currentTime = currentTime;
    updateVisualization();
}

void TimeVisualizerWidget::updateVisualization()
{
    update(); // Trigger a repaint
}

TimeSelectionVisualizer::TimeSelectionVisualizer(QWidget* parent, QTimer* timer, int clearButtonHeight)
    : QWidget(parent)
    , m_button(nullptr)
    , m_visualizerWidget(nullptr)
    , m_layout(nullptr)
    , m_timer(timer)
    , m_ownsTimer(false)
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
    m_button = new QPushButton("H", this);
    m_button->setFixedSize(BUTTON_SIZE, clearButtonHeight);
    m_button->setContentsMargins(0, 0, 0, 0); // Remove button margins
    m_button->setStyleSheet(
        "QPushButton {"
        "    background-color: grey;"
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
        "}"
    );

    // Create visualizer widget
    m_visualizerWidget = new TimeVisualizerWidget(this);

    // Add widgets to layout
    m_layout->addWidget(m_button);
    m_layout->addWidget(m_visualizerWidget, 1); // Stretch factor of 1 to fill remaining space

    // Connect button click to internal handler
    connect(m_button, &QPushButton::clicked, this, &TimeSelectionVisualizer::onButtonClicked);

    // Connect visualizer widget signals to our signals
    connect(m_visualizerWidget, &TimeVisualizerWidget::timeSelectionMade, this, &TimeSelectionVisualizer::timeSelectionMade);
    connect(m_visualizerWidget, &TimeVisualizerWidget::timeSelectionModified, this, &TimeSelectionVisualizer::timeSelectionModified);

    // Set the layout
    setLayout(m_layout);
}

TimeSelectionVisualizer::~TimeSelectionVisualizer()
{
    // Stop the timer if we own it
    if (m_timer && m_ownsTimer) {
        m_timer->stop();
        // Timer will be automatically deleted by Qt's parent-child system
    }
}

void TimeSelectionVisualizer::setupTimer()
{
    // If no timer provided, create a default 1-second timer
    if (!m_timer) {
        m_timer = new QTimer(this);
        m_ownsTimer = true;
        m_timer->setInterval(1000); // 1 second
    }

    // Connect timer to our tick handler
    connect(m_timer, &QTimer::timeout, this, &TimeSelectionVisualizer::onTimerTick);

    // Start the timer
    m_timer->start();

    // DEBUG_OUT() << "TimeSelectionVisualizer: Timer setup completed - interval:" << m_timer->interval() << "ms";
}

void TimeSelectionVisualizer::onTimerTick()
{
    // Update current time to the visualizer widget
    QTime currentTime = QTime::currentTime();

    if (m_visualizerWidget) {
        m_visualizerWidget->setCurrentTime(currentTime);
    }

    // DEBUG_OUT() << "TimeSelectionVisualizer: Timer tick - updated current time to" << currentTime.toString();
}

void TimeSelectionVisualizer::onButtonClicked()
{
    if (hasTimeSelections()) {
        // If there are selections, clear them
        clearTimeSelections();
        emit timeSelectionsCleared();
        // DEBUG_OUT() << "Time selections cleared and signal emitted!";
    } else {
        // No selections: request full selection; container may use BTW horizontal line (real time to line) or fall back to full range
        emit fullSelectionRequested();
    }
}

// Mouse event handlers for TimeVisualizerWidget
void TimeVisualizerWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) return;
    std::pair<int, SelectionHitZone> hit = hitTest(event->pos().x(), event->pos().y());
    int index = hit.first;
    SelectionHitZone zone = hit.second;
    if (index >= 0 && zone != SelectionHitZone::None) {
        m_interactionSelectionIndex = index;
        m_dragStartY = event->pos().y();
        m_dragStartSpan = m_timeSelections.at(index);
        if (zone == SelectionHitZone::TopEdge)
            m_interactionMode = InteractionMode::ResizingTop;
        else if (zone == SelectionHitZone::BottomEdge)
            m_interactionMode = InteractionMode::ResizingBottom;
        else
            m_interactionMode = InteractionMode::Dragging;
        update();
        return;
    }
    m_interactionMode = InteractionMode::Creating;
    m_isSelecting = true;
    m_selectionStartY = event->pos().y();
    m_selectionEndY = event->pos().y();
    update();
}

void TimeVisualizerWidget::mouseMoveEvent(QMouseEvent* event)
{
    int y = event->pos().y();
    if (m_interactionMode == InteractionMode::ResizingTop && m_interactionSelectionIndex >= 0) {
        QDateTime newEnd = timeAtY(y);
        TimeSelectionSpan span = m_timeSelections.at(m_interactionSelectionIndex);
        span.endTime = newEnd;
        if (span.startTime > span.endTime) std::swap(span.startTime, span.endTime);
        if (span.startTime.secsTo(span.endTime) < MIN_SELECTION_SECONDS) return;
        if (hasValidRange()) span = clampToValidRange(span);
        if (span.endTime < span.startTime) return;
        m_timeSelections[m_interactionSelectionIndex] = span;
        update();
        return;
    }
    if (m_interactionMode == InteractionMode::ResizingBottom && m_interactionSelectionIndex >= 0) {
        QDateTime newStart = timeAtY(y);
        TimeSelectionSpan span = m_timeSelections.at(m_interactionSelectionIndex);
        span.startTime = newStart;
        if (span.startTime > span.endTime) std::swap(span.startTime, span.endTime);
        if (span.startTime.secsTo(span.endTime) < MIN_SELECTION_SECONDS) return;
        if (hasValidRange()) span = clampToValidRange(span);
        if (span.endTime < span.startTime) return;
        m_timeSelections[m_interactionSelectionIndex] = span;
        update();
        return;
    }
    if (m_interactionMode == InteractionMode::Dragging && m_interactionSelectionIndex >= 0) {
        qint64 deltaSecs = timeAtY(m_dragStartY).secsTo(timeAtY(y));
        TimeSelectionSpan span;
        span.id = m_dragStartSpan.id;
        span.startTime = m_dragStartSpan.startTime.addSecs(deltaSecs);
        span.endTime = m_dragStartSpan.endTime.addSecs(deltaSecs);
        if (hasValidRange()) span = clampToValidRange(span);
        if (span.endTime < span.startTime) return;
        m_timeSelections[m_interactionSelectionIndex] = span;
        update();
        return;
    }
    if (m_isSelecting) {
        m_selectionEndY = y;
        update();
    }
}

void TimeVisualizerWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) return;
    if (m_interactionMode == InteractionMode::ResizingTop || m_interactionMode == InteractionMode::ResizingBottom
        || m_interactionMode == InteractionMode::Dragging) {
        if (m_interactionSelectionIndex >= 0 && m_interactionSelectionIndex < m_timeSelections.size()) {
            TimeSelectionSpan newSpan = m_timeSelections.at(m_interactionSelectionIndex);
            if (hasValidRange()) newSpan = clampToValidRange(newSpan);
            // Idempotency guard: a press+release with no effective movement must
            // leave the span unchanged and emit nothing, so repeated clicks on an
            // existing selection cannot drift/grow it.
            const bool unchanged = newSpan.startTime == m_dragStartSpan.startTime
                                 && newSpan.endTime == m_dragStartSpan.endTime;
            if (newSpan.endTime >= newSpan.startTime && !unchanged) {
                m_timeSelections[m_interactionSelectionIndex] = newSpan;
                emit timeSelectionModified(m_interactionSelectionIndex, newSpan);
            } else if (unchanged) {
                // Restore exactly, in case a sub-threshold move mutated the stored span.
                m_timeSelections[m_interactionSelectionIndex] = m_dragStartSpan;
            }
        }
        m_interactionMode = InteractionMode::None;
        m_interactionSelectionIndex = -1;
        update();
        return;
    }
    if (m_isSelecting) {
        m_isSelecting = false;
        TimeSelectionSpan span = calculateSelectionSpan(m_selectionStartY, m_selectionEndY);
        if (hasValidRange()) {
            TimeSelectionSpan clamped = clampToValidRange(span);
            if (clamped.endTime >= clamped.startTime) span = clamped;
            else { update(); return; }
        }
        addTimeSelection(span);
        emit timeSelectionMade(span);
        update();
    }
}

void TimeVisualizerWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        // Determine the full range to select: valid range if set, otherwise full visualizer range
        QDateTime currentDate = QDateTime::currentDateTime();
        TimeSelectionSpan span;
        
        if (hasValidRange()) {
            span.startTime = m_validStartDateTime;
            span.endTime = m_validEndDateTime;
        } else {
            // Map the entire widget: bottom corresponds to oldest (height), top to current (0)
            const int bottomY = rect().height();
            const int topY = 0;
            QTime startTime = yCoordinateToTime(bottomY);
            QTime endTime = yCoordinateToTime(topY);
            if (startTime > endTime) std::swap(startTime, endTime);
            
            span.startTime = QDateTime(currentDate.date(), startTime);
            span.endTime = QDateTime(currentDate.date(), endTime);
        }

        addTimeSelection(span);
        emit timeSelectionMade(span);
        update();
    }
}

void TimeVisualizerWidget::drawCurrentSelection(QPainter& painter)
{
    QRect drawArea = rect();
    int widgetHeight = drawArea.height();
    int widgetWidth = drawArea.width();
    
    // Clamp Y coordinates to widget bounds
    int startY = qMax(0, qMin(widgetHeight, m_selectionStartY));
    int endY = qMax(0, qMin(widgetHeight, m_selectionEndY));
    
    // Ensure the rectangle is at least 1 pixel high
    int topY = qMin(startY, endY);
    int bottomY = qMax(startY, endY);
    int rectHeight = qMax(1, bottomY - topY);

    // Inset horizontally to match the committed-selection double-border look.
    int selX = SELECTION_SIDE_INSET;
    int selW = qMax(1, widgetWidth - 2 * SELECTION_SIDE_INSET);

    // Draw an in-progress selection as a lighter grey block so it is distinct
    // from a committed (white) selection on the black background.
    painter.fillRect(selX, topY, selW, rectHeight, QColor(180, 180, 180));

    // Inner border
    painter.setPen(QPen(QColor(120, 120, 120), 1));
    painter.drawRect(selX, topY, selW - 1, rectHeight - 1);
}

QTime TimeVisualizerWidget::yCoordinateToTime(int y) const
{
    QRect drawArea = rect();
    int widgetHeight = drawArea.height();
    
    // Calculate the total timeline duration in seconds
    int totalSeconds = m_timeLineLength.hour() * 3600 + m_timeLineLength.minute() * 60 + m_timeLineLength.second();
    
    if (totalSeconds <= 0 || widgetHeight <= 0) {
        return m_currentTime; // Return current time if invalid parameters
    }

    // Clamp Y to the visible strip. Dragging past the top/bottom edge must not
    // extrapolate a time outside the visible history window (this is what caused
    // selections to keep growing when dragged beyond the strip, especially in
    // short 2-row layouts where each pixel maps to many seconds).
    y = qBound(0, y, widgetHeight);

    // Calculate pixels per second
    double pixelsPerSecond = static_cast<double>(widgetHeight) / totalSeconds;
    
    // Get current time in seconds
    int currentTimeSeconds = m_currentTime.hour() * 3600 + m_currentTime.minute() * 60 + m_currentTime.second();
    
    // Calculate time at Y coordinate
    // Y=0 corresponds to currentTime, Y=height corresponds to currentTime-timespan
    int timeAtYSeconds = currentTimeSeconds - static_cast<int>(y / pixelsPerSecond);

    // Clamp to a valid time-of-day instead of wrapping across midnight. The old
    // "hours = 24 + hours" wrap turned out-of-range values into large bogus times.
    timeAtYSeconds = qBound(0, timeAtYSeconds, 24 * 3600 - 1);

    int hours = timeAtYSeconds / 3600;
    int minutes = (timeAtYSeconds % 3600) / 60;
    int seconds = timeAtYSeconds % 60;

    return QTime(hours, minutes, seconds);
}

QDateTime TimeVisualizerWidget::timeAtY(int y) const
{
    QTime t = yCoordinateToTime(y);
    return QDateTime(QDateTime::currentDateTime().date(), t);
}

QRect TimeVisualizerWidget::getSelectionRect(int index) const
{
    if (index < 0 || index >= m_timeSelections.size()) return QRect();
    const TimeSelectionSpan& span = m_timeSelections.at(index);
    QRect drawArea = rect();
    int widgetHeight = drawArea.height();
    int widgetWidth = drawArea.width();
    int totalSeconds = m_timeLineLength.hour() * 3600 + m_timeLineLength.minute() * 60 + m_timeLineLength.second();
    if (totalSeconds <= 0 || widgetHeight <= 0) return QRect();
    double pixelsPerSecond = static_cast<double>(widgetHeight) / totalSeconds;
    int currentTimeSeconds = m_currentTime.hour() * 3600 + m_currentTime.minute() * 60 + m_currentTime.second();
    QTime selectionStartTime = span.startTime.time();
    QTime selectionEndTime = span.endTime.time();
    int selectionStartSeconds = selectionStartTime.hour() * 3600 + selectionStartTime.minute() * 60 + selectionStartTime.second();
    int selectionEndSeconds = selectionEndTime.hour() * 3600 + selectionEndTime.minute() * 60 + selectionEndTime.second();
    int timeSpanStartSeconds = currentTimeSeconds - totalSeconds;
    if (selectionEndSeconds >= timeSpanStartSeconds && selectionStartSeconds <= currentTimeSeconds) {
        int topY = static_cast<int>((currentTimeSeconds - selectionEndSeconds) * pixelsPerSecond);
        int bottomY = static_cast<int>((currentTimeSeconds - selectionStartSeconds) * pixelsPerSecond);
        topY = qMax(0, qMin(widgetHeight, topY));
        bottomY = qMax(0, qMin(widgetHeight, bottomY));
        int rectHeight = qMax(1, bottomY - topY);
        return QRect(0, topY, widgetWidth, rectHeight);
    }
    return QRect();
}

std::pair<int, SelectionHitZone> TimeVisualizerWidget::hitTest(int x, int y) const
{
    (void)x;
    for (int i = 0; i < m_timeSelections.size(); ++i) {
        QRect r = getSelectionRect(i);
        if (!r.contains(0, y)) continue;
        int topY = r.top();
        int bottomY = r.bottom();
        // Only offer edge-resize when the rendered rectangle is tall enough that
        // the top and bottom edge zones do not overlap. For short selections
        // (e.g. a 5-min span on a 15-min strip that renders only a few pixels
        // tall) a plain click would otherwise be misread as a resize and the
        // selection would balloon on every click. Treat those as a center drag.
        const bool tallEnough = (bottomY - topY) > (2 * RESIZE_EDGE_THRESHOLD);
        if (tallEnough) {
            if (y - topY <= RESIZE_EDGE_THRESHOLD)
                return { i, SelectionHitZone::TopEdge };
            if (bottomY - y <= RESIZE_EDGE_THRESHOLD)
                return { i, SelectionHitZone::BottomEdge };
        }
        return { i, SelectionHitZone::Center };
    }
    return { -1, SelectionHitZone::None };
}

TimeSelectionSpan TimeVisualizerWidget::calculateSelectionSpan(int startY, int endY) const
{
    QTime startTime = yCoordinateToTime(startY);
    QTime endTime = yCoordinateToTime(endY);
    
    // Ensure startTime is before endTime
    if (startTime > endTime) {
        std::swap(startTime, endTime);
    }
    
    // Convert QTime to QDateTime using current date
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QDateTime startDateTime(currentDateTime.date(), startTime);
    QDateTime endDateTime(currentDateTime.date(), endTime);
    
    // Handle day rollover: if endTime appears to be from previous day, adjust
    // This can happen if the calculated times span midnight
    if (endDateTime < startDateTime && endTime < startTime) {
        // If times cross midnight, endDateTime might need to be next day
        // But since we swapped if startTime > endTime, we should be fine
        // However, if times don't naturally order, we may need to adjust
    }
    
    return TimeSelectionSpan(startDateTime, endDateTime);
}

void TimeVisualizerWidget::setValidSelectionRange(const QDateTime& start, const QDateTime& end)
{
    m_validStartDateTime = start;
    m_validEndDateTime = end;
    updateVisualization();
}

void TimeVisualizerWidget::setValidSelectionRange(const QTime& start, const QTime& end)
{
    // Backward-compatible overload: compose the time-of-day with the current date.
    if (start.isNull() || end.isNull()) {
        setValidSelectionRange(QDateTime(), QDateTime());
        return;
    }
    const QDate today = QDateTime::currentDateTime().date();
    setValidSelectionRange(QDateTime(today, start), QDateTime(today, end));
}

TimeSelectionSpan TimeVisualizerWidget::clampToValidRange(const TimeSelectionSpan& span) const
{
    if (!hasValidRange()) return span;

    TimeSelectionSpan clamped = span;

    if (clamped.startTime < m_validStartDateTime) {
        clamped.startTime = m_validStartDateTime;
    }
    if (clamped.endTime > m_validEndDateTime) {
        clamped.endTime = m_validEndDateTime;
    }
    return clamped;
}
