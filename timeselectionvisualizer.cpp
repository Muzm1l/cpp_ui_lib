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


void TimeVisualizerWidget::drawSelection(QPainter &painter, const TimeSelectionSpan &span)
{
    // First get the draw area
    QRect drawArea = rect();

    // Now get the time span
    int startSeconds = span.startTime.hour() * 3600 + span.startTime.minute() * 60 + span.startTime.second();
    int endSeconds = span.endTime.hour() * 3600 + span.endTime.minute() * 60 + span.endTime.second();

    // Now get the pixels per second
    int pixelsPerSecond = drawArea.height() / (endSeconds - startSeconds);
    
    // Now based on the current time identify the start and end y positions
    int startY = static_cast<int>((m_currentTime.hour() * 3600 + m_currentTime.minute() * 60 + m_currentTime.second() - endSeconds) * pixelsPerSecond);
    int endY = static_cast<int>((m_currentTime.hour() * 3600 + m_currentTime.minute() * 60 + m_currentTime.second() - startSeconds) * pixelsPerSecond);

    // Now draw the selection
    painter.fillRect(0, startY, drawArea.width(), endY - startY, QColor(255, 255, 255));

    // Now draw the border
    painter.setPen(QPen(QColor(150, 150, 150), 1));
    painter.drawRect(0, startY, drawArea.width(), endY - startY);
}

void TimeVisualizerWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Fill with light grey background
    painter.fillRect(rect(), QColor(200, 200, 200));
    
    // Draw time selection rectangles
    if (!m_timeSelections.isEmpty() && !m_timeLineLength.isNull() && !m_currentTime.isNull()) {
        qDebug() << "Drawing time selections";
            
        for (const TimeSelectionSpan& span : m_timeSelections) {
            qDebug() << "Drawing time selection: " << span.startTime << " to " << span.endTime;
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
