#include "timeselectionvisualizer.h"
#include "ui_timeselectionvisualizer.h"

TimeVisualizerWidget::TimeVisualizerWidget(QWidget *parent)
    : QWidget(parent)
    , m_timeLineLength(QTime(0, 0, 0))
    , m_currentTime(QTime(0, 0, 0))
{
    setFixedWidth(GRAPHICS_VIEW_WIDTH);
    setMinimumHeight(50); // Set a minimum height
}

void TimeVisualizerWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Fill with light grey background
    painter.fillRect(rect(), QColor(200, 200, 200));
    
    // Draw time selection rectangles
    if (!m_timeSelections.isEmpty() && !m_timeLineLength.isNull() && !m_currentTime.isNull()) {
        int widgetHeight = height();
        int widgetWidth = width();
        
        // Calculate the scale factor for time to pixels
        int totalSeconds = m_timeLineLength.hour() * 3600 + m_timeLineLength.minute() * 60 + m_timeLineLength.second();
        
        if (totalSeconds > 0 && widgetHeight > 0) {
            double pixelsPerSecond = static_cast<double>(widgetHeight) / totalSeconds;
            
            // Calculate the time range visible in the widget
            // currentTime is at the top (y=0), currentTime-timespan is at the bottom (y=height)
            int currentTimeSeconds = m_currentTime.hour() * 3600 + m_currentTime.minute() * 60 + m_currentTime.second();
            int timeSpanStartSeconds = currentTimeSeconds - totalSeconds;
            
            // Draw each time selection as a white rectangle
            painter.setBrush(QBrush(QColor(255, 255, 255)));
            painter.setPen(QPen(QColor(255, 255, 255), 1));
            
            for (const TimeSelectionSpan& span : m_timeSelections) {
                int selectionStartSeconds = span.startTime.hour() * 3600 + span.startTime.minute() * 60 + span.startTime.second();
                int selectionEndSeconds = span.endTime.hour() * 3600 + span.endTime.minute() * 60 + span.endTime.second();
                
                // Check if selection overlaps with visible time range
                if (selectionEndSeconds >= timeSpanStartSeconds && selectionStartSeconds <= currentTimeSeconds) {
                    // Calculate Y positions relative to currentTime (top of widget)
                    // Earlier times (smaller values) should be lower (higher Y values)
                    int startY = static_cast<int>((currentTimeSeconds - selectionEndSeconds) * pixelsPerSecond);
                    int endY = static_cast<int>((currentTimeSeconds - selectionStartSeconds) * pixelsPerSecond);
                    
                    // Clamp to widget bounds
                    startY = qMax(0, qMin(widgetHeight, startY));
                    endY = qMax(0, qMin(widgetHeight, endY));
                    
                    // Ensure the rectangle is at least 1 pixel high
                    int rectHeight = qMax(1, endY - startY);
                    
                    painter.drawRect(0, startY, widgetWidth, rectHeight);
                }
            }
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

void TimeVisualizerWidget::setCurrentTime(const QTime& currentTime)
{
    m_currentTime = currentTime;
    updateVisualization();
}

void TimeVisualizerWidget::updateVisualization()
{
    update(); // Trigger a repaint
}

TimeSelectionVisualizer::TimeSelectionVisualizer(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TimeSelectionVisualizer)
    , m_button(nullptr)
    , m_visualizerWidget(nullptr)
    , m_layout(nullptr)
{
    ui->setupUi(this);
    
    // Create vertical layout
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
    
    // Create button with grey background and white border
    m_button = new QPushButton("H", this);
    m_button->setFixedSize(BUTTON_SIZE, BUTTON_SIZE);
    m_button->setStyleSheet(
        "QPushButton {"
        "    background-color: grey;"
        "    border: 2px solid white;"
        "    color: white;"
        "    font-weight: bold;"
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
    delete ui;
}

void TimeSelectionVisualizer::onButtonClicked()
{
    clearTimeSelections();
    qDebug() << "Time selections cleared!";
}
