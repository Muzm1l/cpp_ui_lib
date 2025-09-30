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

void TimelineVisualizerWidget::setTimeInterval(TimeInterval interval)
{
    m_timeInterval = interval;
    
    // Convert TimeInterval to QTime and set timeline length
    QTime newLength = timeIntervalToQTime(interval);
    setTimeLineLength(newLength);
    
    // Reset accumulated offset when interval changes
    m_accumulatedOffset = 0.0;
    
    // Recreate all drawing objects with new parameters
    // This will automatically calculate optimal divisions based on current area
    createDrawingObjects();
    
    qDebug() << "Time interval set to:" << timeIntervalToString(interval) 
             << "Divisions:" << m_numberOfDivisions 
             << "Segment duration:" << calculateSegmentDurationSeconds() << "seconds"
             << "Min segment height:" << getMinimumSegmentHeight();
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
    
    // Calculate segment duration in seconds
    double segmentDurationSeconds = calculateSegmentDurationSeconds();
    
    // Calculate pixel speed: pixels per second
    // This should be based on how fast we want segments to move
    double segmentHeight = static_cast<double>(rect().height()) / m_numberOfDivisions;
    
    // Pixel speed should be segmentHeight / segmentDurationSeconds
    // This means one segment height per segment duration
    m_pixelSpeed = segmentHeight / segmentDurationSeconds;
    
    // Update accumulated offset based on time difference
    double timeDiffSeconds = timeDiffMs / 1000.0;
    m_accumulatedOffset += m_pixelSpeed * timeDiffSeconds;
    
    qDebug() << "Pixel speed updated:" << m_pixelSpeed << "pixels/sec, time diff:" << timeDiffMs << "ms, accumulated offset:" << m_accumulatedOffset
             << "Segment duration:" << segmentDurationSeconds << "seconds";
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
    if (widgetHeight <= 0) {
        widgetHeight = 300; // Default height for timeline view
        qDebug() << "Widget height is 0, using default height:" << widgetHeight;
    }
    
    // Use fixed number of segments for all time intervals
    m_numberOfDivisions = getFixedNumberOfSegments();
    
    QRect drawArea(0, 0, TIMELINE_VIEW_GRAPHICS_VIEW_WIDTH, widgetHeight);
    
    // Calculate segment height to fill the entire drawing area
    double segmentHeight = static_cast<double>(widgetHeight) / m_numberOfDivisions;
    
    qDebug() << "Creating drawing objects - Widget height:" << height() 
             << "Using height:" << widgetHeight
             << "Draw area:" << drawArea 
             << "Fixed divisions:" << m_numberOfDivisions
             << "Calculated segment height:" << segmentHeight
             << "Time interval:" << timeIntervalToString(m_timeInterval);
    
    // Create segment drawers for animation range (including off-screen segments)
    clearDrawingObjects(); // Clear existing ones first
    
    // Create chevron drawer - position it at the top of the timeline
    m_chevronDrawer = new TimelineChevronDrawer(drawArea, 30);
    
    // Create segments with fixed count but variable time gaps
    // We need enough segments to cover the entire visible area plus some buffer
    int segmentsNeeded = m_numberOfDivisions + 10; // Add buffer for smooth animation
    int startSegment = -(segmentsNeeded / 2);
    int endSegment = segmentsNeeded / 2;
    
    qDebug() << "Creating fixed segments - Height:" << widgetHeight 
             << "Fixed divisions:" << m_numberOfDivisions 
             << "Segment height:" << segmentHeight
             << "Segments needed:" << segmentsNeeded
             << "Segments range:" << startSegment << "to" << endSegment;
    
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

void TimelineVisualizerWidget::setChevronLabel1(const QString& label)
{
    m_chevronLabel1 = label;
    updateVisualization();
}

void TimelineVisualizerWidget::setChevronLabel2(const QString& label)
{
    m_chevronLabel2 = label;
    updateVisualization();
}

void TimelineVisualizerWidget::setChevronLabel3(const QString& label)
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
void TimelineView::setChevronLabel1(const QString& label)
{
    if (m_visualizerWidget) {
        m_visualizerWidget->setChevronLabel1(label);
    }
}

void TimelineView::setChevronLabel2(const QString& label)
{
    if (m_visualizerWidget) {
        m_visualizerWidget->setChevronLabel2(label);
    }
}

void TimelineView::setChevronLabel3(const QString& label)
{
    if (m_visualizerWidget) {
        m_visualizerWidget->setChevronLabel3(label);
    }
}

QString TimelineView::getChevronLabel1() const
{
    if (m_visualizerWidget) {
        return m_visualizerWidget->getChevronLabel1();
    }
    return QString();
}

QString TimelineView::getChevronLabel2() const
{
    if (m_visualizerWidget) {
        return m_visualizerWidget->getChevronLabel2();
    }
    return QString();
}

QString TimelineView::getChevronLabel3() const
{
    if (m_visualizerWidget) {
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
    if (debugCounter++ % 60 == 0) { // Print every 60 frames to avoid spam
        qDebug() << "PaintEvent - Widget rect:" << rect() 
                 << "Divisions:" << m_numberOfDivisions
                 << "Segment height:" << segmentHeight
                 << "Time interval:" << timeIntervalToString(m_timeInterval);
    }
    
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
    
    // Create new segments as needed to ensure we have enough coverage
    if (!m_segmentDrawers.empty()) {
        int minSegmentNumber = (*std::min_element(m_segmentDrawers.begin(), m_segmentDrawers.end(),
            [](TimelineSegmentDrawer* a, TimelineSegmentDrawer* b) {
                return a->getSegmentNumber() < b->getSegmentNumber();
            }))->getSegmentNumber();
        
        int maxSegmentNumber = (*std::max_element(m_segmentDrawers.begin(), m_segmentDrawers.end(),
            [](TimelineSegmentDrawer* a, TimelineSegmentDrawer* b) {
                return a->getSegmentNumber() < b->getSegmentNumber();
            }))->getSegmentNumber();
        
        // Calculate which segments should be visible to fill the entire area
        int firstVisibleSegment = static_cast<int>(-smoothOffset / segmentHeight);
        int lastVisibleSegment = firstVisibleSegment + m_numberOfDivisions;
        
        // Add segments above the current range if needed
        while (minSegmentNumber > firstVisibleSegment - 2) { // Keep some buffer above
            --minSegmentNumber;
            TimelineSegmentDrawer* segmentDrawer = new TimelineSegmentDrawer(
                minSegmentNumber, m_timeLineLength, m_currentTime, m_numberOfDivisions, m_isAbsoluteTime, rect());
            segmentDrawer->setShowRelativeLabel(m_showRelativeLabels);
            m_segmentDrawers.push_back(segmentDrawer);
        }
        
        // Add segments below the current range if needed
        while (maxSegmentNumber < lastVisibleSegment + 2) { // Keep some buffer below
            ++maxSegmentNumber;
            TimelineSegmentDrawer* segmentDrawer = new TimelineSegmentDrawer(
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
    for (auto* segmentDrawer : m_segmentDrawers) {
        if (segmentDrawer) {
            int segmentNumber = segmentDrawer->getSegmentNumber();
            // Calculate Y position for this segment with smooth offset (shift down)
            double y = segmentNumber * segmentHeight + smoothOffset;
            
            // Only draw if the segment is within the visible range
            if (segmentNumber >= firstVisibleSegment && segmentNumber < lastVisibleSegment) {
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
    if (debugCounter2++ % 60 == 0) {
        qDebug() << "Segment coverage check - Widget height:" << rect().height()
                 << "Total segments:" << m_segmentDrawers.size()
                 << "Divisions:" << m_numberOfDivisions
                 << "Segment height:" << segmentHeight
                 << "Total coverage:" << (m_numberOfDivisions * segmentHeight)
                 << "First visible:" << firstVisibleSegment
                 << "Last visible:" << lastVisibleSegment
                 << "Smooth offset:" << smoothOffset;
    }
    
    // Draw chevron using drawing object - position it at the top of the timeline
    if (m_chevronDrawer) {
        m_chevronDrawer->setDrawArea(rect());
        m_chevronDrawer->setYOffset(30); // Position at the top with enough space for the chevron box
        m_chevronDrawer->update();
        drawChevronWithPainter(painter, m_chevronDrawer);
    }
    
    // Draw a border to make it more visible
    painter.setPen(QPen(QColor(150, 150, 150), 1));
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
}

void TimelineVisualizerWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    
    // Recreate drawing objects with new dimensions
    qDebug() << "Widget resized to:" << size();
    createDrawingObjects();
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
    if (!chevronDrawer) {
        qDebug() << "Chevron drawer is null!";
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

    // Draw the 3 labels - 1 and 3 below the V, 2 at the top
    painter.setPen(QPen(QColor(0, 100, 255), 2)); // Blue text, thicker
    
    // Label 1: below the chevron, left of the V
    painter.drawText(QPoint(chevronX, tipY + 15), m_chevronLabel1);
    
    // Label 2: at the top center (unchanged position)
    painter.drawText(QPoint(chevronX + chevronWidth / 2, chevronY), m_chevronLabel2);
    
    // Label 3: below the chevron, right of the V
    painter.drawText(QPoint(chevronX + chevronWidth, tipY + 15), m_chevronLabel3);
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
    m_visualizerWidget->setTimeInterval(TimeInterval::FifteenMinutes);
    
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