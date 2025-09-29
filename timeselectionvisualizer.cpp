#include "timeselectionvisualizer.h"
#include <QDebug>

TimeVisualizerWidget::TimeVisualizerWidget(QWidget *parent)
    : QWidget(parent)
    , m_timeLineLength(QTime(0, 0, 0))
    , m_currentTime(QTime(0, 0, 0))
{
    setFixedWidth(GRAPHICS_VIEW_WIDTH);
    setMinimumHeight(50); // Set a minimum height
    
    // Remove all margins and padding for snug fit
    setContentsMargins(0, 0, 0, 0);
}


void TimeVisualizerWidget::drawSelection(QPainter &painter, const TimeSelectionSpan &span)
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
    int selectionStartSeconds = span.startTime.hour() * 3600 + span.startTime.minute() * 60 + span.startTime.second();
    int selectionEndSeconds = span.endTime.hour() * 3600 + span.endTime.minute() * 60 + span.endTime.second();

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

        // Now draw the selection
        painter.fillRect(0, topY, widgetWidth, rectHeight, QColor(255, 255, 255));

        // Now draw the border
        painter.setPen(QPen(QColor(150, 150, 150), 1));
        painter.drawRect(0, topY, widgetWidth, rectHeight);
    }
}

void TimeVisualizerWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Fill with light grey background
    painter.fillRect(rect(), QColor(200, 200, 200));
    
    // Draw time selection rectangles
    if (!m_timeSelections.isEmpty() && !m_timeLineLength.isNull() && !m_currentTime.isNull()) {
        for (const TimeSelectionSpan& span : m_timeSelections) {
            drawSelection(painter, span);
        }
    }
    
    // Draw a border to make it more visible
    painter.setPen(QPen(QColor(150, 150, 150), 1));
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
}

void TimeVisualizerWidget::addTimeSelection(TimeSelectionSpan span)
{
    if (m_timeSelections.size() < MAX_TIME_SELECTIONS) {
        m_timeSelections.append(span);
        updateVisualization();
    }
}

void TimeVisualizerWidget::clearTimeSelections()
{
    m_timeSelections.clear();
    updateVisualization();
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

TimeSelectionVisualizer::TimeSelectionVisualizer(QWidget *parent, QTimer *timer)
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
    m_button->setFixedSize(BUTTON_SIZE, BUTTON_SIZE);
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
    
    qDebug() << "TimeSelectionVisualizer: Timer setup completed - interval:" << m_timer->interval() << "ms";
}

void TimeSelectionVisualizer::onTimerTick()
{
    // Update current time to the visualizer widget
    QTime currentTime = QTime::currentTime();
    
    if (m_visualizerWidget) {
        m_visualizerWidget->setCurrentTime(currentTime);
    }
    
    qDebug() << "TimeSelectionVisualizer: Timer tick - updated current time to" << currentTime.toString();
}

void TimeSelectionVisualizer::onButtonClicked()
{
    clearTimeSelections();
    qDebug() << "Time selections cleared!";
}
