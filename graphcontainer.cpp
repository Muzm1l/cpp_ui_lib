#include "graphcontainer.h"

GraphContainer::GraphContainer(QWidget *parent, bool showTimelineView)
    : QWidget{parent}, m_showTimelineView(showTimelineView), m_timelineWidth(150), m_graphViewSize(80, 300)
{
    // Create main horizontal layout with 1px spacing and no margins
    m_mainLayout = new QHBoxLayout(this);
    m_mainLayout->setSpacing(1);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // Create left vertical layout with no margins
    m_leftLayout = new QVBoxLayout();
    m_leftLayout->setContentsMargins(0, 0, 0, 0);
    
    // Create ComboBox
    m_comboBox = new QComboBox(this);
    m_comboBox->addItem("Option 1");
    m_comboBox->addItem("Option 2");
    m_comboBox->addItem("Option 3");
    m_comboBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    
    // Create ZoomPanel
    m_zoomPanel = new ZoomPanel(this);
    m_zoomPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_zoomPanel->setMaximumHeight(50); // Limit zoom panel height
    
    // Create WaterfallGraph
    m_waterfallGraph = new waterfallgraph(this);
    
    
    // Add ComboBox, ZoomPanel, and WaterfallGraph to left layout
    m_leftLayout->addWidget(m_comboBox);
    m_leftLayout->addWidget(m_zoomPanel);
    m_leftLayout->addWidget(m_waterfallGraph); // No stretch factor - let it use preferred size
    
    // Add left layout to main layout with stretch factor
    m_mainLayout->addLayout(m_leftLayout, 1); // Give stretch factor of 1 to left layout
    
    // Create TimelineSelectionView
    m_timelineSelectionView = new TimeSelectionVisualizer(this);
    m_mainLayout->addWidget(m_timelineSelectionView);
    
    // Create TimelineView (conditionally based on showTimelineView)
    if (m_showTimelineView) {
        m_timelineView = new TimelineView(this);
        m_mainLayout->addWidget(m_timelineView);
    } else {
        m_timelineView = nullptr;
    }
    
    // Set layout
    setLayout(m_mainLayout);
    
    // Initialize container size
    updateTotalContainerSize();
}

void GraphContainer::setShowTimelineView(bool showTimelineView)
{
    m_showTimelineView = showTimelineView;
    if (m_timelineView) {
        m_timelineView->setVisible(showTimelineView);
    } else {
        m_timelineView = new TimelineView(this);
        m_mainLayout->addWidget(m_timelineView);
    }
    
    // Update container size when timeline view visibility changes
    updateTotalContainerSize();
}

bool GraphContainer::getShowTimelineView()
{
    return m_showTimelineView;
}


int GraphContainer::getTimelineWidth() const
{
    return m_timelineWidth;
}

void GraphContainer::setGraphViewSize(int width, int height)
{
    m_graphViewSize = QSize(width, height);
    
    // Set the waterfall graph size
    if (m_waterfallGraph) {
        m_waterfallGraph->setMinimumSize(m_graphViewSize);
        m_waterfallGraph->setMaximumSize(m_graphViewSize);
        m_waterfallGraph->updateGeometry();
    }
    
    // Update the total container size
    updateTotalContainerSize();
}

QSize GraphContainer::getGraphViewSize() const
{
    return m_graphViewSize;
}

QSize GraphContainer::getTotalContainerSize() const
{
    // Calculate total container size based on graph view size and timeline components
    int totalWidth = m_graphViewSize.width();
    int totalHeight = m_graphViewSize.height();
    
    // Add timeline selection view width (fixed width)
    totalWidth += 50; // Timeline selection view width
    
    // Add timeline view width if enabled
    if (m_showTimelineView) {
        totalWidth += m_timelineWidth;
    }
    
    // Add spacing between components (1px each)
    totalWidth += 2; // 2 spacings: between graph and timeline selection, and between timeline selection and timeline view
    
    return QSize(totalWidth, totalHeight);
}

void GraphContainer::updateTotalContainerSize()
{
    QSize totalSize = getTotalContainerSize();
    setMinimumSize(totalSize);
    setMaximumSize(totalSize);
    updateGeometry();
}