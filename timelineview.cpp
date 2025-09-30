#include "timelineview.h"
#include <QDebug>
#include <QGraphicsView>
#include <QFrame>
#include <QBrush>
#include <algorithm>

TimelineVisualizerWidget::TimelineVisualizerWidget(QWidget *parent)
    : QWidget(parent)
    , m_currentTime(QTime::currentTime())
    , m_numberOfDivisions(15)
    , m_lastCurrentTime(QTime::currentTime())
    , m_pixelSpeed(0.0)
    , m_accumulatedOffset(0.0)
    , m_chevronDrawer(nullptr)
{
    setFixedWidth(TIMELINE_VIEW_GRAPHICS_VIEW_WIDTH);
    setMinimumHeight(50); // Set a minimum height
    
    // Remove all margins and padding for snug fit
    setContentsMargins(0, 0, 0, 0);
    
    // Create drawing objects
    createDrawingObjects();
}

void TimelineVisualizerWidget::setTimeLineLength(const QTime& length)
{
    m_timeLineLength = length;
    // Reset accumulated offset when timeline length changes
    m_accumulatedOffset = 0.0;
    updateVisualization();
}

void TimelineVisualizerWidget::setCurrentTime(const QTime& currentTime)
{
    m_lastCurrentTime = m_currentTime;
    m_currentTime = currentTime;
    updatePixelSpeed();
    updateVisualization();
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
    update(); // Trigger a repaint
}

void TimelineVisualizerWidget::updateAndDraw()
{
    // This method provides a clean interface for external code to trigger the update + draw loop
    update(); // Trigger a repaint
}

void TimelineVisualizerWidget::updatePixelSpeed()
{
    if (m_lastCurrentTime.isNull() || m_currentTime.isNull() || m_timeLineLength.isNull()) {
        m_pixelSpeed = 0.0;
        return;
    }
    
    // Calculate time difference in seconds
    int timeDiffMs = m_lastCurrentTime.msecsTo(m_currentTime);
    if (timeDiffMs <= 0) {
        m_pixelSpeed = 0.0;
        return;
    }
    
    // Calculate total timeline duration in seconds
    int totalSeconds = m_timeLineLength.hour() * 3600 + m_timeLineLength.minute() * 60 + m_timeLineLength.second();
    
    // Calculate pixel speed: pixels per second
    double segmentHeight = static_cast<double>(rect().height()) / m_numberOfDivisions;
    m_pixelSpeed = segmentHeight / (totalSeconds / m_numberOfDivisions);
    
    // Update accumulated offset based on time difference
    double timeDiffSeconds = timeDiffMs / 1000.0;
    m_accumulatedOffset += m_pixelSpeed * timeDiffSeconds;
    
    qDebug() << "Pixel speed updated:" << m_pixelSpeed << "pixels/sec, time diff:" << timeDiffMs << "ms, accumulated offset:" << m_accumulatedOffset;
}

double TimelineVisualizerWidget::calculateSmoothOffset()
{
    // Return the accumulated offset for smooth shifting
    return m_accumulatedOffset;
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
    QRect drawArea(0, 0, TIMELINE_VIEW_GRAPHICS_VIEW_WIDTH, height());
    
    // Create chevron drawer
    m_chevronDrawer = new TimelineChevronDrawer(drawArea, 50);
    
    // Create segment drawers for animation range (including off-screen segments)
    clearDrawingObjects(); // Clear existing ones first
    
    // Create segments starting from the top, going down
    // Start from segment 0 (current time) and go up and down
    int startSegment = -10; // Start well above current time
    int endSegment = 20; // End well below current time
    
    for (int i = startSegment; i < endSegment; ++i) {
        TimelineSegmentDrawer* segmentDrawer = new TimelineSegmentDrawer(
            i, m_timeLineLength, m_currentTime, m_numberOfDivisions, m_isAbsoluteTime, drawArea);
        segmentDrawer->setShowRelativeLabel(m_showRelativeLabels);
        m_segmentDrawers.push_back(segmentDrawer);
    }
}

void TimelineVisualizerWidget::clearDrawingObjects()
{
    // Clear segment drawers
    for (auto* segmentDrawer : m_segmentDrawers) {
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
    for (auto* segmentDrawer : m_segmentDrawers) {
        if (segmentDrawer) {
            segmentDrawer->setShowRelativeLabel(showRelative);
        }
    }
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
    
    // Remove segments that have gone completely out of view (below the bottom)
    auto it = m_segmentDrawers.begin();
    while (it != m_segmentDrawers.end()) {
        TimelineSegmentDrawer* segmentDrawer = *it;
        if (segmentDrawer) {
            double y = segmentDrawer->getSegmentNumber() * segmentHeight + smoothOffset;
            // If segment is completely below the visible area, remove it
            if (y > rect().height()) {
                delete segmentDrawer;
                it = m_segmentDrawers.erase(it);
                continue;
            }
        }
        ++it;
    }
    
    // Create new segments at the top if needed
    if (!m_segmentDrawers.empty()) {
        int minSegmentNumber = (*std::min_element(m_segmentDrawers.begin(), m_segmentDrawers.end(),
            [](TimelineSegmentDrawer* a, TimelineSegmentDrawer* b) {
                return a->getSegmentNumber() < b->getSegmentNumber();
            }))->getSegmentNumber();
        
        // Add segments above the current range if needed
        while (minSegmentNumber > -5) { // Keep some buffer above
            --minSegmentNumber;
            TimelineSegmentDrawer* segmentDrawer = new TimelineSegmentDrawer(
                minSegmentNumber, m_timeLineLength, m_currentTime, m_numberOfDivisions, m_isAbsoluteTime, rect());
            segmentDrawer->setShowRelativeLabel(m_showRelativeLabels);
            m_segmentDrawers.push_back(segmentDrawer);
        }
    }
    
    // Draw segments that are visible (including those that might be partially off-screen due to smooth shifting)
    for (auto* segmentDrawer : m_segmentDrawers) {
        if (segmentDrawer) {
            int segmentNumber = segmentDrawer->getSegmentNumber();
            // Calculate Y position for this segment with smooth offset (shift down)
            double y = segmentNumber * segmentHeight + smoothOffset;
            
            // Only draw if the segment is at least partially visible
            if (y + segmentHeight >= 0 && y < rect().height()) {
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
    
    // Draw chevron using drawing object
    if (m_chevronDrawer) {
        m_chevronDrawer->setDrawArea(rect());
        m_chevronDrawer->update();
        drawChevronWithPainter(painter, m_chevronDrawer);
    }
    
    // Draw a border to make it more visible
    painter.setPen(QPen(QColor(150, 150, 150), 1));
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
}

// Helper methods to draw using QPainter instead of QGraphicsScene
void TimelineVisualizerWidget::drawSegmentWithPainter(QPainter& painter, TimelineSegmentDrawer* segmentDrawer)
{
    if (!segmentDrawer) return;
    
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
    
    if (shouldShowLabel && segmentDrawer->isLabelSet()) {
        // Use the fixed label that was set during construction
        QString timestamp = segmentDrawer->getFixedLabel();
        if (!timestamp.isEmpty()) {
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

void TimelineVisualizerWidget::drawChevronWithPainter(QPainter& painter, TimelineChevronDrawer* chevronDrawer)
{
    if (!chevronDrawer) return;
    
    QRect drawArea = chevronDrawer->getDrawArea();
    int yOffset = chevronDrawer->getYOffset();
    double chevronWidthPercent = chevronDrawer->getChevronWidthPercent();
    int chevronHeight = chevronDrawer->getChevronHeight();
    int chevronBoxHeight = chevronDrawer->getChevronBoxHeight();
    
    int widgetWidth = drawArea.width();
    
    // Set pen for blue chevron outline
    painter.setPen(QPen(QColor(0, 100, 255), 2)); // Blue color, 2px width
    
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
        QPoint(0, chevronY - chevronBoxHeight),                           // Start point
        QPoint(0, chevronY),                           // Left edge
        QPoint(chevronX, chevronY),                           // Top left point
        QPoint(tipX, tipY),                                   // Bottom point (tip)
        QPoint(chevronX + chevronWidth, chevronY),              // Top right point
        QPoint(widgetWidth, chevronY),                           // Right edge
        QPoint(widgetWidth, chevronY - chevronBoxHeight),                           // Right edge
        QPoint(0, chevronY - chevronBoxHeight)                           // Start point
    };

    // Draw the chevron outline
    painter.drawPolygon(chevronPoints, 8);

    // Draw the 3 labels inside the chevron with a numeric value
    painter.setPen(QPen(QColor(255, 255, 255), 1)); // White text
    painter.drawText(QPoint(chevronX, chevronY), "1");
    painter.drawText(QPoint(chevronX + chevronWidth / 2, chevronY), "2");
    painter.drawText(QPoint(chevronX + chevronWidth, chevronY), "3");
}

TimelineView::TimelineView(QWidget *parent, QTimer *timer)
    : QWidget(parent)
    , m_intervalChangeButton(nullptr)
    , m_timeModeChangeButton(nullptr)
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
    m_intervalChangeButton = new QPushButton("dt: 00:15", this);
    m_intervalChangeButton->setFixedSize(TIMELINE_VIEW_GRAPHICS_VIEW_WIDTH, TIMELINE_VIEW_BUTTON_SIZE/2);
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
        "}"
    );

    // setup m_timeModeChangeButton
    m_timeModeChangeButton = new QPushButton("Abs", this);
    m_timeModeChangeButton->setFixedSize(TIMELINE_VIEW_GRAPHICS_VIEW_WIDTH, TIMELINE_VIEW_BUTTON_SIZE/2);
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
        "}"
    );

    m_isAbsoluteTime = true;
    // updateTimeModeButtonText(m_isAbsoluteTime);

    // Create visualizer widget
    m_visualizerWidget = new TimelineVisualizerWidget(this);
    
    // Add widgets to layout
    m_layout->addWidget(m_timeModeChangeButton);
    m_layout->addWidget(m_intervalChangeButton);
    m_layout->addWidget(m_visualizerWidget, 1); // Stretch factor of 1 to fill remaining space
    
    // Connect button click to internal handler
    connect(m_timeModeChangeButton, &QPushButton::clicked, this, &TimelineView::onTimeModeButtonClicked);
    connect(m_intervalChangeButton, &QPushButton::clicked, this, &TimelineView::onIntervalButtonClicked);
    
    // Set the layout
    setLayout(m_layout);
    
    // Set the TimelineView widget width to match button and graphics view width
    setFixedWidth(TIMELINE_VIEW_GRAPHICS_VIEW_WIDTH);

    // Set the default time interval to be 15 minutes
    this->setTimeLineLength(TimeInterval::FifteenMinutes);
    
    // Initialize button text with default interval
    updateButtonText(TimeInterval::FifteenMinutes);
}

TimelineView::~TimelineView()
{
    // Stop the timer if we own it
    if (m_timer && m_ownsTimer) {
        m_timer->stop();
        // Timer will be automatically deleted by Qt's parent-child system
    }
}

void TimelineView::setupTimer()
{
    // If no timer provided, create a default 1-second timer
    if (!m_timer) {
        m_timer = new QTimer(this);
        m_ownsTimer = true;
        m_timer->setInterval(1000); // 1 second
    }
    
    // Connect timer to our tick handler
    connect(m_timer, &QTimer::timeout, this, &TimelineView::onTimerTick);
    
    // Start the timer
    m_timer->start();
    
    qDebug() << "TimelineView: Timer setup completed - interval:" << m_timer->interval() << "ms";
}

void TimelineView::onTimerTick()
{
    // Update current time to the visualizer widget
    QTime currentTime = QTime::currentTime();
    
    if (m_visualizerWidget) {
        m_visualizerWidget->setCurrentTime(currentTime);
    }
    
    qDebug() << "TimelineView: Timer tick - updated current time to" << currentTime.toString();
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
    this->setTimeLineLength(nextInterval);
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