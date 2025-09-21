#include "graphlayout.h"
#include <QDebug>

GraphLayout::GraphLayout(QWidget *parent, LayoutType layoutType)
    : QWidget{parent}
    , m_layoutType(layoutType)
{
    // Create 4 graph containers
    m_graphContainers.push_back(new GraphContainer(this, true));
    m_graphContainers.push_back(new GraphContainer(this, true));   
    m_graphContainers.push_back(new GraphContainer(this, true));
    m_graphContainers.push_back(new GraphContainer(this, true));

    // Create main layout with 1px spacing and no margins
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(1);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    
    m_graphContainersRow1Layout = new QHBoxLayout();
    m_graphContainersRow1Layout->setSpacing(1);
    m_graphContainersRow1Layout->setContentsMargins(0, 0, 0, 0);
    
    m_graphContainersRow2Layout = new QHBoxLayout();
    m_graphContainersRow2Layout->setSpacing(1);
    m_graphContainersRow2Layout->setContentsMargins(0, 0, 0, 0);

    // Add graph containers to main layout
    m_mainLayout->addLayout(m_graphContainersRow1Layout);
    m_mainLayout->addLayout(m_graphContainersRow2Layout);

    // Set layout
    setLayout(m_mainLayout);

    // Initialize the graph containers layout
    setLayoutType(layoutType);
}

GraphLayout::~GraphLayout()
{
    // Clean up graph containers
    for (auto* container : m_graphContainers) {
        delete container;
    }
    m_graphContainers.clear();
    
    // Layouts will be automatically cleaned up by Qt's parent-child system
    // since they have this widget as parent
}

void GraphLayout::setLayoutType(LayoutType layoutType)
{
    m_layoutType = layoutType;


    // Remove all widgets from both row layouts
    while (QLayoutItem* item = m_graphContainersRow1Layout->takeAt(0)) {
        if (item->widget()) {
            m_graphContainersRow1Layout->removeWidget(item->widget());
        }
    }
    while (QLayoutItem* item = m_graphContainersRow2Layout->takeAt(0)) {
        if (item->widget()) {
            m_graphContainersRow2Layout->removeWidget(item->widget());
        }
    }

    // First, make all containers visible
    for (auto* container : m_graphContainers) {
        container->setVisible(true);
    }

    switch (m_layoutType) {
        case LayoutType::GPW1W:
            // Add graph containers to row 1
            m_graphContainersRow1Layout->addWidget(m_graphContainers[0]);
            m_graphContainers[0]->setShowTimelineView(true);
            // Hide the other containers
            m_graphContainers[1]->setVisible(false);
            m_graphContainers[2]->setVisible(false);
            m_graphContainers[3]->setVisible(false);
            break;
        case LayoutType::GPW4W:
            // Add graph containers to row 1
            m_graphContainersRow1Layout->addWidget(m_graphContainers[0]);
            m_graphContainersRow1Layout->addWidget(m_graphContainers[1]);
            // Hide timeline view for the second graph container
            m_graphContainers[0]->setShowTimelineView(true);
            m_graphContainers[1]->setShowTimelineView(false);
            // Add graph containers to row 2
            m_graphContainersRow2Layout->addWidget(m_graphContainers[2]);
            m_graphContainersRow2Layout->addWidget(m_graphContainers[3]);
            // Hide timeline view for fourth graph containers
            m_graphContainers[2]->setShowTimelineView(true);
            m_graphContainers[3]->setShowTimelineView(false);
            break;
        case LayoutType::GPW2WV:
            // Add 1 graph container to row 1
            m_graphContainersRow1Layout->addWidget(m_graphContainers[0]);
            m_graphContainers[0]->setShowTimelineView(true);
            // Add 1 graph container to row 2
            m_graphContainersRow2Layout->addWidget(m_graphContainers[1]);
            m_graphContainers[1]->setShowTimelineView(true);
            // Hide the other containers
            m_graphContainers[2]->setVisible(false);
            m_graphContainers[3]->setVisible(false);
            break;
        case LayoutType::GPW2WH:
            // Add 2 graph containers to row 1
            m_graphContainersRow1Layout->addWidget(m_graphContainers[0]);
            m_graphContainersRow1Layout->addWidget(m_graphContainers[1]);
            // Hide timeline view for the second graph container
            m_graphContainers[0]->setShowTimelineView(true);
            m_graphContainers[1]->setShowTimelineView(false);
            // Hide the other containers
            m_graphContainers[2]->setVisible(false);
            m_graphContainers[3]->setVisible(false);
            break;
        case LayoutType::GPW4WH:
            // Add 4 graph containers to row 1
            m_graphContainersRow1Layout->addWidget(m_graphContainers[0]);
            m_graphContainersRow1Layout->addWidget(m_graphContainers[1]);
            m_graphContainersRow1Layout->addWidget(m_graphContainers[2]);
            m_graphContainersRow1Layout->addWidget(m_graphContainers[3]);
            // Only show the timeline for the first graph container
            m_graphContainers[0]->setShowTimelineView(true);
            m_graphContainers[1]->setShowTimelineView(false);
            m_graphContainers[2]->setShowTimelineView(false);
            m_graphContainers[3]->setShowTimelineView(false);
            break;
        case LayoutType::HIDDEN:
            // Hide all containers
            for (auto* container : m_graphContainers) {
                container->setVisible(false);
            }
            break;
        default:
            qDebug() << "Invalid layout type selected";
            break;
    }
    
    // Update sizing after layout changes
    updateLayoutSizing();
}

LayoutType GraphLayout::getLayoutType() const
{
    return m_layoutType;
}


void GraphLayout::setGraphViewSize(int width, int height)
{
    // Set graph view size for all containers
    for (auto* container : m_graphContainers) {
        container->setGraphViewSize(width, height);
    }
    updateLayoutSizing();
}

void GraphLayout::updateLayoutSizing()
{
    // Calculate total dimensions based on visible containers
    int totalWidth = 0;
    int totalHeight = 0;
    
    switch (m_layoutType) {
        case LayoutType::GPW1W: {
            // Single container
            if (m_graphContainers[0]->isVisible()) {
                QSize containerSize = m_graphContainers[0]->getTotalContainerSize();
                totalWidth = containerSize.width();
                totalHeight = containerSize.height();
            }
            break;
        }
        case LayoutType::GPW2WH: {
            // Two containers horizontally
            int maxHeight = 0;
            for (int i = 0; i < 2; ++i) {
                if (m_graphContainers[i]->isVisible()) {
                    QSize containerSize = m_graphContainers[i]->getTotalContainerSize();
                    totalWidth += containerSize.width();
                    maxHeight = qMax(maxHeight, containerSize.height());
                }
            }
            totalHeight = maxHeight;
            if (totalWidth > 0) {
                totalWidth += 1; // Add spacing between containers
            }
            break;
        }
        case LayoutType::GPW4WH: {
            // Four containers horizontally
            int maxHeight = 0;
            for (int i = 0; i < 4; ++i) {
                if (m_graphContainers[i]->isVisible()) {
                    QSize containerSize = m_graphContainers[i]->getTotalContainerSize();
                    totalWidth += containerSize.width();
                    maxHeight = qMax(maxHeight, containerSize.height());
                }
            }
            totalHeight = maxHeight;
            if (totalWidth > 0) {
                totalWidth += 3; // Add spacing between 4 containers (3 gaps)
            }
            break;
        }
        case LayoutType::GPW2WV: {
            // Two containers vertically
            int maxWidth = 0;
            for (int i = 0; i < 2; ++i) {
                if (m_graphContainers[i]->isVisible()) {
                    QSize containerSize = m_graphContainers[i]->getTotalContainerSize();
                    maxWidth = qMax(maxWidth, containerSize.width());
                    totalHeight += containerSize.height();
                }
            }
            totalWidth = maxWidth;
            if (totalHeight > 0) {
                totalHeight += 1; // Add spacing between containers
            }
            break;
        }
        case LayoutType::GPW4W: {
            // Four containers in 2x2 grid
            int maxWidthRow1 = 0, maxWidthRow2 = 0;
            int maxHeightRow1 = 0, maxHeightRow2 = 0;
            
            // Row 1: containers 0, 1
            for (int i = 0; i < 2; ++i) {
                if (m_graphContainers[i]->isVisible()) {
                    QSize containerSize = m_graphContainers[i]->getTotalContainerSize();
                    maxWidthRow1 += containerSize.width();
                    maxHeightRow1 = qMax(maxHeightRow1, containerSize.height());
                }
            }
            if (maxWidthRow1 > 0) {
                maxWidthRow1 += 1; // Add spacing between containers in row
            }
            
            // Row 2: containers 2, 3
            for (int i = 2; i < 4; ++i) {
                if (m_graphContainers[i]->isVisible()) {
                    QSize containerSize = m_graphContainers[i]->getTotalContainerSize();
                    maxWidthRow2 += containerSize.width();
                    maxHeightRow2 = qMax(maxHeightRow2, containerSize.height());
                }
            }
            if (maxWidthRow2 > 0) {
                maxWidthRow2 += 1; // Add spacing between containers in row
            }
            
            totalWidth = qMax(maxWidthRow1, maxWidthRow2);
            totalHeight = maxHeightRow1 + maxHeightRow2;
            if (totalHeight > 0) {
                totalHeight += 1; // Add spacing between rows
            }
            break;
        }
        case LayoutType::HIDDEN:
            totalWidth = 0;
            totalHeight = 0;
            break;
    }
    
    setMinimumSize(totalWidth, totalHeight);
    setMaximumSize(totalWidth, totalHeight);
    updateGeometry();
}