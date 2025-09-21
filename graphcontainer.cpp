#include "graphcontainer.h"

GraphContainer::GraphContainer(QWidget *parent, bool showTimelineView)
    : QWidget{parent}, m_showTimelineView(showTimelineView)
{
    // Create main horizontal layout with 1px spacing
    m_mainLayout = new QHBoxLayout(this);
    m_mainLayout->setSpacing(1);
    
    // Create left vertical layout
    m_leftLayout = new QVBoxLayout();
    
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
}
