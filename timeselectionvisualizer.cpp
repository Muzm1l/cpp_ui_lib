#include "timeselectionvisualizer.h"
#include "ui_timeselectionvisualizer.h"

TimeVisualizerWidget::TimeVisualizerWidget(QWidget *parent)
    : QWidget(parent)
{
    setFixedWidth(GRAPHICS_VIEW_WIDTH);
    setMinimumHeight(50); // Set a minimum height
}

void TimeVisualizerWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Fill with solid grey background
    painter.fillRect(rect(), QColor(128, 128, 128));
    
    // Optional: Draw a border to make it more visible
    painter.setPen(QPen(QColor(100, 100, 100), 1));
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
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
    
    // Set the layout
    setLayout(m_layout);
}

TimeSelectionVisualizer::~TimeSelectionVisualizer()
{
    delete ui;
}
