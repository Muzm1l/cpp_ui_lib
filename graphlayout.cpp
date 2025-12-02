#include "graphlayout.h"
#include <QDebug>

GraphLayout::GraphLayout(QWidget *parent, LayoutType layoutType, QTimer *timer, std::map<GraphType, std::vector<QPair<QString, QColor>>> seriesLabelsMap)
    : QWidget{parent}, m_layoutType(layoutType), m_timer(timer)
{

    // If the timer is not provided, create a default 1-second timer
    if (!m_timer)
    {
        m_timer = new QTimer(this);
        m_timer->setInterval(1000); // 1 second
                                    // Connect timer to our tick handler
        connect(m_timer, &QTimer::timeout, this, &GraphLayout::onTimerTick);

        // Start the timer
        m_timer->start();

        qDebug() << "GraphLayout: Timer setup completed since none was provided - interval:" << m_timer->interval() << "ms";
    }

    // Initialize data sources based on provided labels
    initializeDataSources(seriesLabelsMap);
    
    // Initialize container labels (using data source labels as container labels)
    m_containerLabels = getAllGraphTypeStrings();

    initializeContainers();

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
    for (auto *container : m_graphContainers)
    {
        delete container;
    }
    m_graphContainers.clear();

    // Layouts will be automatically cleaned up by Qt's parent-child system
    // since they have this widget as parent
}

void GraphLayout::setLayoutType(LayoutType layoutType)
{
    m_layoutType = layoutType;

    // Disconnect all existing connections before changing layout
    disconnectAllContainerConnections();

    // Remove all widgets from both row layouts
    while (QLayoutItem *item = m_graphContainersRow1Layout->takeAt(0))
    {
        if (item->widget())
        {
            m_graphContainersRow1Layout->removeWidget(item->widget());
        }
    }
    while (QLayoutItem *item = m_graphContainersRow2Layout->takeAt(0))
    {
        if (item->widget())
        {
            m_graphContainersRow2Layout->removeWidget(item->widget());
        }
    }

    // First, make all containers visible and show their time selection visualizers
    for (auto *container : m_graphContainers)
    {
        container->setVisible(true);
        container->setShowTimeSelectionVisualizer(true); // Reset to visible by default
    }

    switch (m_layoutType)
    {
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
        // Hide timeline view for first container in top row, show for second container
        m_graphContainers[0]->setShowTimelineView(false);
        m_graphContainers[1]->setShowTimelineView(true);
        // Hide time selection visualizer for first container in top row (container 0)
        m_graphContainers[0]->setShowTimeSelectionVisualizer(false);
        // Add graph containers to row 2
        m_graphContainersRow2Layout->addWidget(m_graphContainers[2]);
        m_graphContainersRow2Layout->addWidget(m_graphContainers[3]);
        // Hide timeline view for first container in bottom row, show for second container
        m_graphContainers[2]->setShowTimelineView(false);
        m_graphContainers[3]->setShowTimelineView(true);
        // Hide time selection visualizer for first container in bottom row (container 2)
        m_graphContainers[2]->setShowTimeSelectionVisualizer(false);

        // Connect the time scope change handler of containers 1 to the event of 0 and the 3 to the event of 2
        // Note: Interval changes are now handled centrally by GraphLayout
        connect(m_graphContainers[0], &GraphContainer::TimeScopeChanged, m_graphContainers[1], &GraphContainer::onTimeScopeChanged);
        connect(m_graphContainers[2], &GraphContainer::TimeScopeChanged, m_graphContainers[3], &GraphContainer::onTimeScopeChanged);

        break;
    case LayoutType::GPW2WV:
        // Add 1 graph container to row 1
        m_graphContainersRow1Layout->addWidget(m_graphContainers[0]);
        m_graphContainers[0]->setShowTimelineView(true);
        // Add 1 graph container to row 2
        m_graphContainersRow2Layout->addWidget(m_graphContainers[2]);
        m_graphContainers[2]->setShowTimelineView(true);
        // Hide the other containers
        m_graphContainers[1]->setVisible(false);
        m_graphContainers[3]->setVisible(false);
        break;
    case LayoutType::GPW2WH:
        // Add 2 graph containers to row 1
        m_graphContainersRow1Layout->addWidget(m_graphContainers[0]);
        m_graphContainersRow1Layout->addWidget(m_graphContainers[1]);
        // Hide timeline view for first container, show for second container
        m_graphContainers[0]->setShowTimelineView(false);
        m_graphContainers[1]->setShowTimelineView(true);
        // Hide time selection visualizer for first container
        m_graphContainers[0]->setShowTimeSelectionVisualizer(false);
        // Hide the other containers
        m_graphContainers[2]->setVisible(false);
        m_graphContainers[3]->setVisible(false);

        // Connect the time scope change handler of containers 1 to the event of 0
        // Note: Interval changes are now handled centrally by GraphLayout
        connect(m_graphContainers[0], &GraphContainer::TimeScopeChanged, m_graphContainers[1], &GraphContainer::onTimeScopeChanged);
        break;
    case LayoutType::GPW4WH:
    // this 4 horizantal graphs with no GPW
        // Add 4 graph containers to row 1
        m_graphContainersRow1Layout->addWidget(m_graphContainers[0]);
        m_graphContainersRow1Layout->addWidget(m_graphContainers[1]);
        m_graphContainersRow1Layout->addWidget(m_graphContainers[2]);
        m_graphContainersRow1Layout->addWidget(m_graphContainers[3]);
        // Hide timeline view for first container, show for third container
        m_graphContainers[0]->setShowTimelineView(false);
        m_graphContainers[1]->setShowTimelineView(false);
        m_graphContainers[2]->setShowTimelineView(true);
        m_graphContainers[3]->setShowTimelineView(false);
        // Hide time selection visualizer for 1st, 2nd, and 4th containers
        m_graphContainers[0]->setShowTimeSelectionVisualizer(false);
        m_graphContainers[1]->setShowTimeSelectionVisualizer(false);
        m_graphContainers[3]->setShowTimeSelectionVisualizer(false);

        // Connect the interval change handlers of containers 0,1,3 to the event of 2 (container 2 has timeline view)
        connect(m_graphContainers[2], &GraphContainer::IntervalChanged, m_graphContainers[0], &GraphContainer::onTimeIntervalChanged);
        connect(m_graphContainers[2], &GraphContainer::IntervalChanged, m_graphContainers[1], &GraphContainer::onTimeIntervalChanged);
        connect(m_graphContainers[2], &GraphContainer::IntervalChanged, m_graphContainers[3], &GraphContainer::onTimeIntervalChanged);
        
        // Connect the time scope change handlers of containers 0,1,3 to the event of 2 (container 2 has timeline view)
        connect(m_graphContainers[2], &GraphContainer::TimeScopeChanged, m_graphContainers[0], &GraphContainer::onTimeScopeChanged);
        connect(m_graphContainers[2], &GraphContainer::TimeScopeChanged, m_graphContainers[1], &GraphContainer::onTimeScopeChanged);
        connect(m_graphContainers[2], &GraphContainer::TimeScopeChanged, m_graphContainers[3], &GraphContainer::onTimeScopeChanged);
        break;
    // Layout 2W: two graph container side by side, but take up whole screen. this is similar 2WH
    case LayoutType::NOGPW2WH:
        // Add 2 graph containers to row 1, side by side
        m_graphContainersRow1Layout->addWidget(m_graphContainers[0]);
        m_graphContainersRow1Layout->addWidget(m_graphContainers[1]);
        
        // Hide timeline view for first container, show for second container
        m_graphContainers[0]->setShowTimelineView(false);
        m_graphContainers[1]->setShowTimelineView(true);
        // Hide time selection visualizer for first container
        m_graphContainers[0]->setShowTimeSelectionVisualizer(false);
        
        // Hide the other containers
        m_graphContainers[2]->setVisible(false);
        m_graphContainers[3]->setVisible(false);

        // Note: Interval changes are now handled centrally by GraphLayout
        // Connect the interval change handler of container 1 to the event of 0
        connect(m_graphContainers[0], &GraphContainer::IntervalChanged, m_graphContainers[1], &GraphContainer::onTimeIntervalChanged);
        
        // Connect the time scope change handler of container 1 to the event of 0
        connect(m_graphContainers[0], &GraphContainer::TimeScopeChanged, m_graphContainers[1], &GraphContainer::onTimeScopeChanged);
        break;
    case LayoutType::HIDDEN:
        // Hide all containers
        for (auto *container : m_graphContainers)
        {
            container->setVisible(false);
        }
        break;
    default:
        qDebug() << "Invalid layout type selected";
        break;
    }

    // Reset container sizes before recalculating to prevent size carryover from previous layout
    for (auto *container : m_graphContainers)
    {
        if (container)
        {
            // Remove fixed size constraints to allow recalculation
            container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        }
    }
    
    // Remove fixed width constraint from GraphLayout to allow recalculation based on new layout
    setMaximumWidth(QWIDGETSIZE_MAX);
    
    // Update sizing after layout changes
    updateLayoutSizing();

    // Link horizontal containers for selection events
    linkHorizontalContainers();

    // Sync all timeline views to keep them in sync
    syncAllTimelineViews();

    // Reconnect container -> layout selection signals after disconnects
    for (auto *container : m_graphContainers)
    {
        connect(container, &GraphContainer::TimeSelectionCreated,
                this, &GraphLayout::onTimeSelectionCreated);
        connect(container, &GraphContainer::TimeSelectionsCleared,
                this, &GraphLayout::onTimeSelectionsCleared);
        connect(container, &GraphContainer::IntervalChanged,
                this, &GraphLayout::onContainerIntervalChanged);
        connect(container, &GraphContainer::TimeScopeChanged,
                this, &GraphLayout::onContainerTimeScopeChanged);
        
        // Connect marker timestamp signals
        connect(container, &GraphContainer::RTWRMarkerTimestampCaptured,
                this, &GraphLayout::RTWRMarkerTimestampCaptured);
        connect(container, &GraphContainer::BTWManualMarkerPlaced,
                this, &GraphLayout::onBTWManualMarkerPlaced);
        // Also forward the signal for external integration
        connect(container, &GraphContainer::BTWManualMarkerPlaced,
                this, &GraphLayout::BTWManualMarkerPlaced);
        connect(container, &GraphContainer::BTWManualMarkerClicked,
                this, &GraphLayout::BTWManualMarkerClicked);
    }
}

LayoutType GraphLayout::getLayoutType() const
{
    return m_layoutType;
}

void GraphLayout::setGraphViewSize(int width, int height)
{
    // Set graph view size for all containers
    for (auto *container : m_graphContainers)
    {
        container->setGraphViewSize(width, height);
    }
    updateLayoutSizing();
}

void GraphLayout::initializeDataSources(std::map<GraphType, std::vector<QPair<QString, QColor>>> seriesLabelsMap)
{
    // Initialize data sources for all graph types manually
    for (auto& pair : seriesLabelsMap) {
        GraphType graphType = pair.first;
        const auto& seriesData = pair.second;
        
        // Extract just the series labels for WaterfallData constructor
        std::vector<QString> seriesLabels;
        for (const auto& seriesPair : seriesData) {
            seriesLabels.push_back(seriesPair.first);
        }
        
        // Create WaterfallData with series labels
        m_dataSources[graphType] = new WaterfallData(graphTypeToString(graphType), seriesLabels);
        
        // Set colors for each series (this will require updating WaterfallData to support colors)
        for (const auto& seriesPair : seriesData) {
            m_seriesColorsMap[seriesPair.first] = seriesPair.second;
        }

    }
}

void GraphLayout::initializeContainers()
{
    // Create 4 graph containers with timer and shared sync state
    m_graphContainers.push_back(new GraphContainer(this, true, m_seriesColorsMap, m_timer, 0, 0, &m_syncState));
    m_graphContainers.push_back(new GraphContainer(this, true, m_seriesColorsMap, m_timer, 0, 0, &m_syncState));
    m_graphContainers.push_back(new GraphContainer(this, true, m_seriesColorsMap, m_timer, 0, 0, &m_syncState));
    m_graphContainers.push_back(new GraphContainer(this, true, m_seriesColorsMap, m_timer, 0, 0, &m_syncState));

    // Attach data sources to containers
    attachContainerDataSources();
    
    // Connect all containers' TimeSelectionCreated and TimeSelectionsCleared signals to our slots
    for (auto *container : m_graphContainers)
    {
        connect(container, &GraphContainer::TimeSelectionCreated,
                this, &GraphLayout::onTimeSelectionCreated);
        connect(container, &GraphContainer::TimeSelectionsCleared,
                this, &GraphLayout::onTimeSelectionsCleared);
        connect(container, &GraphContainer::IntervalChanged,
                this, &GraphLayout::onContainerIntervalChanged);
        connect(container, &GraphContainer::TimeScopeChanged,
                this, &GraphLayout::onContainerTimeScopeChanged);
        
        // Connect marker timestamp signals
        connect(container, &GraphContainer::RTWRMarkerTimestampCaptured,
                this, &GraphLayout::RTWRMarkerTimestampCaptured);
        connect(container, &GraphContainer::BTWManualMarkerPlaced,
                this, &GraphLayout::onBTWManualMarkerPlaced);
        // Also forward the signal for external integration
        connect(container, &GraphContainer::BTWManualMarkerPlaced,
                this, &GraphLayout::BTWManualMarkerPlaced);
        connect(container, &GraphContainer::BTWManualMarkerClicked,
                this, &GraphLayout::BTWManualMarkerClicked);
    }
    
    qDebug() << "GraphLayout: Connected all containers to time selection and time scope propagation";

    registerCursorSyncCallbacks();
}

void GraphLayout::attachContainerDataSources()
{
    // Go through each of the graph containers and attach each of the
    // data sources using the key as the title and the value as the datasource
    for (auto *container : m_graphContainers)
    {
        for (auto &dataSource : m_dataSources)
        {
            container->addDataOption(dataSource.first, *dataSource.second);
        }
    }
}

void GraphLayout::updateLayoutSizing()
{
    // Get the current geometry of this GraphLayout widget
    QSize currentSize = size();
    if (currentSize.isEmpty()) {
        // If size is not set yet, use the minimum size hint
        currentSize = minimumSizeHint();
    }
    
    int availableHeight = currentSize.height();
    
    // Calculate container heights based on layout type and available space
    int containerHeight = 0;
    int numRows = 1;
    
    switch (m_layoutType)
    {
    case LayoutType::GPW1W:
        numRows = 1;
        break;
    case LayoutType::GPW2WH:
    case LayoutType::GPW4WH:
    case LayoutType::NOGPW2WH:
        numRows = 1;
        break;
    case LayoutType::GPW2WV:
        numRows = 2;
        break;
    case LayoutType::GPW4W:
        numRows = 2;
        break;
    case LayoutType::HIDDEN:
        numRows = 0;
        break;
    }
    
    if (numRows > 0) {
        // Calculate height per row, accounting for spacing between rows
        int spacingHeight = (numRows > 1) ? (numRows - 1) : 0; // 1px spacing between rows
        containerHeight = (availableHeight - spacingHeight) / numRows;
        
        // Ensure minimum height
        containerHeight = qMax(containerHeight, 200);
    }
    
    // Set container heights for all visible containers
    for (auto *container : m_graphContainers)
    {
        if (container && container->isVisible())
        {
            container->setContainerHeight(containerHeight);
        }
    }
    
    // Calculate widths based on formula: totalWidth = N_Columns * container_width + 64
    // (64 is the timeline view width)
    int numColumns = 0;
    int containerWidth = 0;
    const int timelineViewWidth = 64; // Timeline view width
    
    switch (m_layoutType)
    {
    case LayoutType::GPW1W:
        numColumns = 1;
        break;
    case LayoutType::GPW2WH:
        numColumns = 2;
        break;
    case LayoutType::NOGPW2WH:
        numColumns = 2;
        break;
    case LayoutType::GPW4WH:
        numColumns = 4;
        break;
    case LayoutType::GPW2WV:
        numColumns = 1; // Stacked vertically, so 1 column
        break;
    case LayoutType::GPW4W:
        numColumns = 2; // 2x2 grid, so 2 columns
        break;
    case LayoutType::HIDDEN:
        numColumns = 0;
        break;
    }
    
    if (numColumns > 0) {
        // Calculate container width from available space
        // Formula: totalWidth = N_Columns * container_width + timelineViewWidth
        // So: container_width = (totalWidth - timelineViewWidth) / N_Columns
        int availableWidth = currentSize.width();
        
        // If the current width seems too large (likely from a previous layout with more columns),
        // try using parent widget's available space or container's graph view size as a base
        if (!m_graphContainers.empty() && m_graphContainers[0])
        {
            QSize graphViewSize = m_graphContainers[0]->getGraphViewSize();
            int baseContainerWidth = graphViewSize.width();
            
            // Check if current width calculation would result in an unreasonably wide container
            // (more than 2x the base width suggests we're using the wrong base width)
            int calculatedWidth = (availableWidth - timelineViewWidth) / numColumns;
            if (calculatedWidth > baseContainerWidth * 2)
            {
                // Try to use parent widget's available space if available
                QWidget *parent = parentWidget();
                if (parent)
                {
                    int parentWidth = parent->width();
                    if (parentWidth > 0)
                    {
                        int parentBasedWidth = (parentWidth - timelineViewWidth) / numColumns;
                        // Use parent-based width if it's more reasonable
                        if (parentBasedWidth <= baseContainerWidth * 2 && parentBasedWidth >= baseContainerWidth)
                        {
                            containerWidth = parentBasedWidth;
                        }
                        else
                        {
                            // Fall back to base container width from graph view size
                            containerWidth = baseContainerWidth;
                        }
                    }
                    else
                    {
                        // Use the base container width from graph view size
                        containerWidth = baseContainerWidth;
                    }
                }
                else
                {
                    // Use the base container width from graph view size
                    containerWidth = baseContainerWidth;
                }
            }
            else
            {
                containerWidth = calculatedWidth;
            }
        }
        else
        {
            containerWidth = (availableWidth - timelineViewWidth) / numColumns;
        }
        
        // Ensure minimum width
        containerWidth = qMax(containerWidth, 200);
        
        // Set container widths based on layout type
        switch (m_layoutType)
        {
        case LayoutType::GPW1W:
            if (m_graphContainers[0] && m_graphContainers[0]->isVisible()) {
                m_graphContainers[0]->setContainerWidth(containerWidth + timelineViewWidth);
            }
            break;
        case LayoutType::GPW2WH:
            for (int i = 0; i < 2; ++i) {
                if (m_graphContainers[i] && m_graphContainers[i]->isVisible()) {
                    if (i == 0) {
                        m_graphContainers[i]->setContainerWidth(containerWidth);
                    } else {
                        m_graphContainers[i]->setContainerWidth(containerWidth + timelineViewWidth);
                    }
                }
            }
            break;
        case LayoutType::NOGPW2WH:
            for (int i = 0; i < 2; ++i) {
                if (m_graphContainers[i] && m_graphContainers[i]->isVisible()) {
                    if (i == 0) {
                        m_graphContainers[i]->setContainerWidth(containerWidth);
                    } else {
                        m_graphContainers[i]->setContainerWidth(containerWidth + timelineViewWidth);
                    }
                }
            }
            break;
        case LayoutType::GPW4WH:
            for (int i = 0; i < 4; ++i) {
                if (m_graphContainers[i] && m_graphContainers[i]->isVisible()) {
                    if (i == 2) {
                        // Third container (index 2) has timeline view, so gets extra width
                        m_graphContainers[i]->setContainerWidth(containerWidth + timelineViewWidth);
                    } else {
                        m_graphContainers[i]->setContainerWidth(containerWidth);
                    }
                }
            }
            break;
        case LayoutType::GPW2WV:
            // For vertical stacking, both containers should have the same width
            // Containers 0 and 2 are used in GPW2WV layout
            if (m_graphContainers[0] && m_graphContainers[0]->isVisible()) {
                m_graphContainers[0]->setContainerWidth(containerWidth + timelineViewWidth);
            }
            if (m_graphContainers[2] && m_graphContainers[2]->isVisible()) {
                m_graphContainers[2]->setContainerWidth(containerWidth + timelineViewWidth);
            }
            break;
        case LayoutType::GPW4W:
            // For 2x2 grid, second container in each row has timeline view, so gets extra width
            for (int i = 0; i < 4; ++i) {
                if (m_graphContainers[i] && m_graphContainers[i]->isVisible()) {
                    if (i == 1 || i == 3) {
                        // Second container in each row has timeline view, so gets extra width
                        m_graphContainers[i]->setContainerWidth(containerWidth + timelineViewWidth);
                    } else {
                        // First container in each row gets standard width (no timeline view)
                        m_graphContainers[i]->setContainerWidth(containerWidth);
                    }
                }
            }
            break;
        case LayoutType::HIDDEN:
            break;
        }
        
        // Calculate and set total width based on layout type
        int totalWidth = 0;
        switch (m_layoutType)
        {
        case LayoutType::GPW1W:
            totalWidth = containerWidth + timelineViewWidth;
            break;
        case LayoutType::GPW2WH:
            // Container 0: no timeline view, Container 1: has timeline view
            totalWidth = containerWidth + (containerWidth + timelineViewWidth);
            break;
        case LayoutType::NOGPW2WH:
            // Container 0: no timeline view, Container 1: has timeline view
            totalWidth = containerWidth + (containerWidth + timelineViewWidth);
            break;
        case LayoutType::GPW4WH:
            // Container 2 (3rd container) has timeline view, so gets extra width
            totalWidth = containerWidth + containerWidth + (containerWidth + timelineViewWidth) + containerWidth;
            break;
        case LayoutType::GPW2WV:
            totalWidth = containerWidth + timelineViewWidth; // Both containers have same width with timeline view
            break;
        case LayoutType::GPW4W:
            // 2x2 grid: second container in each row has timeline view, so gets extra width
            totalWidth = containerWidth + (containerWidth + timelineViewWidth); // Row 1: standard + (standard + timelineViewWidth)
            break;
        case LayoutType::HIDDEN:
            totalWidth = 0;
            break;
        }
        setFixedWidth(totalWidth);
    }
    
    updateGeometry();
}

void GraphLayout::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    
    // Update container heights when the layout is resized
    updateLayoutSizing();
}

// Data point methods implementation
// Data options management - operate on specific container by label

void GraphLayout::addDataOption(const QString &containerLabel, const GraphType &graphType, WaterfallData &dataSource)
{
    int containerIndex = getContainerIndex(containerLabel);
    if (containerIndex >= 0 && containerIndex < static_cast<int>(m_graphContainers.size()))
    {
        m_graphContainers[containerIndex]->addDataOption(graphType, dataSource);
    }
    else
    {
        qDebug() << "Container not found:" << containerLabel;
    }
}

void GraphLayout::removeDataOption(const QString &containerLabel, const GraphType &graphType)
{
    int containerIndex = getContainerIndex(containerLabel);
    if (containerIndex >= 0 && containerIndex < static_cast<int>(m_graphContainers.size()))
    {
        m_graphContainers[containerIndex]->removeDataOption(graphType);
    }
    else
    {
        qDebug() << "Container not found:" << containerLabel;
    }
}

void GraphLayout::clearDataOptions(const QString &containerLabel)
{
    int containerIndex = getContainerIndex(containerLabel);
    if (containerIndex >= 0 && containerIndex < static_cast<int>(m_graphContainers.size()))
    {
        m_graphContainers[containerIndex]->clearDataOptions();
    }
    else
    {
        qDebug() << "Container not found:" << containerLabel;
    }
}

void GraphLayout::setCurrentDataOption(const QString &containerLabel, const GraphType &graphType)
{
    int containerIndex = getContainerIndex(containerLabel);
    if (containerIndex >= 0 && containerIndex < static_cast<int>(m_graphContainers.size()))
    {
        m_graphContainers[containerIndex]->setCurrentDataOption(graphType);
    }
    else
    {
        qDebug() << "Container not found:" << containerLabel;
    }
}

GraphType GraphLayout::getCurrentDataOption(const QString &containerLabel) const
{
    int containerIndex = getContainerIndex(containerLabel);
    if (containerIndex >= 0 && containerIndex < static_cast<int>(m_graphContainers.size()))
    {
        return m_graphContainers[containerIndex]->getCurrentDataOption();
    }
    qDebug() << "Container not found:" << containerLabel;
    return GraphType::BDW;
}

std::vector<GraphType> GraphLayout::getAvailableDataOptions(const QString &containerLabel) const
{
    int containerIndex = getContainerIndex(containerLabel);
    if (containerIndex >= 0 && containerIndex < static_cast<int>(m_graphContainers.size()))
    {
        return m_graphContainers[containerIndex]->getAvailableDataOptions();
    }
    qDebug() << "Container not found:" << containerLabel;
    return std::vector<GraphType>();
}

WaterfallData *GraphLayout::getDataOption(const QString &containerLabel, const GraphType &graphType)
{
    int containerIndex = getContainerIndex(containerLabel);
    if (containerIndex >= 0 && containerIndex < static_cast<int>(m_graphContainers.size()))
    {
        return m_graphContainers[containerIndex]->getDataOption(graphType);
    }
    qDebug() << "Container not found:" << containerLabel;
    return nullptr;
}

bool GraphLayout::hasDataOption(const QString &containerLabel, const GraphType &graphType) const
{
    int containerIndex = getContainerIndex(containerLabel);
    if (containerIndex >= 0 && containerIndex < static_cast<int>(m_graphContainers.size()))
    {
        return m_graphContainers[containerIndex]->hasDataOption(graphType);
    }
    qDebug() << "Container not found:" << containerLabel;
    return false;
}

// Data options management - operate on all visible containers

void GraphLayout::addDataOption(const GraphType &graphType, WaterfallData &dataSource)
{
    for (auto *container : m_graphContainers)
    {
        if (container)
        {
            container->addDataOption(graphType, dataSource);
        }
    }
}

void GraphLayout::removeDataOption(const GraphType &graphType)
{
    for (auto *container : m_graphContainers)
    {
        if (container)
        {
            container->removeDataOption(graphType);
        }
    }
}

void GraphLayout::clearDataOptions()
{
    for (auto *container : m_graphContainers)
    {
        if (container)
        {
            container->clearDataOptions();
        }
    }
}

void GraphLayout::setCurrentDataOption(const GraphType &graphType)
{
    for (auto *container : m_graphContainers)
    {
        if (container)
        {
            container->setCurrentDataOption(graphType);
        }
    }
}

// Data point methods for specific data sources

void GraphLayout::addDataPointToDataSource(const GraphType &graphType, const QString &seriesLabel, qreal yValue, const QDateTime &timestamp)
{
    QString dataSourceLabel = graphTypeToString(graphType);
    auto it = m_dataSources.find(graphType);
    if (it != m_dataSources.end())
    {
        it->second->addDataPointToSeries(seriesLabel, yValue, timestamp);
        qDebug() << "Added data point to" << dataSourceLabel << "series" << seriesLabel << "y:" << yValue << "time:" << timestamp.toString();

        // Notify all containers that have this data source to update their UI
        for (auto *container : m_graphContainers)
        {
            if (container)
            {
                container->onDataChanged(graphType);
            }
        }
    }
    else
    {
        qDebug() << "Data source not found:" << dataSourceLabel;
    }
}

void GraphLayout::addDataPointsToDataSource(const GraphType &graphType, const QString &seriesLabel, const std::vector<qreal> &yValues, const std::vector<QDateTime> &timestamps)
{
    QString dataSourceLabel = graphTypeToString(graphType);
    auto it = m_dataSources.find(graphType);
    if (it != m_dataSources.end())
    {
        it->second->addDataPointsToSeries(seriesLabel, yValues, timestamps);
        qDebug() << "Added" << yValues.size() << "data points to" << dataSourceLabel << "series" << seriesLabel;

        // Notify all containers that have this data source to update their UI
        for (auto *container : m_graphContainers)
        {
            if (container)
            {
                container->onDataChanged(graphType);
            }
        }
    }
    else
    {
        qDebug() << "Data source not found:" << dataSourceLabel;
    }
}

void GraphLayout::setDataToDataSource(const GraphType &graphType, const QString &seriesLabel, const std::vector<qreal> &yData, const std::vector<QDateTime> &timestamps)
{
    QString dataSourceLabel = graphTypeToString(graphType);
    auto it = m_dataSources.find(graphType);
    if (it != m_dataSources.end())
    {
        it->second->setDataSeries(seriesLabel, yData, timestamps);
        qDebug() << "Set data for" << dataSourceLabel << "series" << seriesLabel << "size:" << yData.size();

        // Notify all containers that have this data source to update their UI
        for (auto *container : m_graphContainers)
        {
            if (container)
            {
                container->onDataChanged(graphType);
            }
        }
    }
    else
    {
        qDebug() << "Data source not found:" << dataSourceLabel;
    }
}

void GraphLayout::setDataToDataSource(const GraphType &graphType, const QString &seriesLabel, const WaterfallData &data)
{
    QString dataSourceLabel = graphTypeToString(graphType);
    auto it = m_dataSources.find(graphType);
    if (it != m_dataSources.end())
    {
        // Get the specific series data from the WaterfallData object
        auto seriesData = data.getAllDataSeries(seriesLabel);
        std::vector<qreal> yData;
        std::vector<QDateTime> timestamps;
        
        for (const auto& pair : seriesData) {
            yData.push_back(pair.first);
            timestamps.push_back(pair.second);
        }
        
        it->second->setDataSeries(seriesLabel, yData, timestamps);
        qDebug() << "Set data for" << dataSourceLabel << "series" << seriesLabel << "from WaterfallData object";

        // Notify all containers that have this data source to update their UI
        for (auto *container : m_graphContainers)
        {
            if (container)
            {
                container->onDataChanged(graphType);
            }
        }
    }
    else
    {
        qDebug() << "Data source not found:" << dataSourceLabel;
    }
}

void GraphLayout::clearDataSource(const GraphType &graphType, const QString &seriesLabel)
{
    QString dataSourceLabel = graphTypeToString(graphType);
    auto it = m_dataSources.find(graphType);
    if (it != m_dataSources.end())
    {
        it->second->clearDataSeries(seriesLabel);
        qDebug() << "Cleared data for" << dataSourceLabel << "series" << seriesLabel;

        // Notify all containers that have this data source to update their UI
        for (auto *container : m_graphContainers)
        {
            if (container)
            {
                container->onDataChanged(graphType);
            }
        }
    }
    else
    {
        qDebug() << "Data source not found:" << dataSourceLabel;
    }
}

// Data source management

WaterfallData *GraphLayout::getDataSource(const GraphType &graphType)
{
    auto it = m_dataSources.find(graphType);
    return (it != m_dataSources.end()) ? it->second : nullptr;
}

bool GraphLayout::hasDataSource(const GraphType &graphType) const
{
    return m_dataSources.find(graphType) != m_dataSources.end();
}

std::vector<GraphType> GraphLayout::getDataSourceLabels() const
{
    std::vector<GraphType> labels;
    for (const auto &pair : m_dataSources)
    {
        labels.push_back(pair.first);
    }
    return labels;
}

// Series-specific data source management

bool GraphLayout::hasSeriesInDataSource(const GraphType &graphType, const QString &seriesLabel) const
{
    auto it = m_dataSources.find(graphType);
    if (it != m_dataSources.end())
    {
        return it->second->hasDataSeries(seriesLabel);
    }
    return false;
}

std::vector<QString> GraphLayout::getSeriesLabelsInDataSource(const GraphType &graphType) const
{
    auto it = m_dataSources.find(graphType);
    if (it != m_dataSources.end())
    {
        return it->second->getDataSeriesLabels();
    }
    return std::vector<QString>();
}

void GraphLayout::addSeriesToDataSource(const GraphType &graphType, const QString &seriesLabel)
{
    auto it = m_dataSources.find(graphType);
    if (it != m_dataSources.end())
    {
        // Create empty vectors for the new series
        std::vector<qreal> emptyYData;
        std::vector<QDateTime> emptyTimestamps;
        it->second->addDataSeries(seriesLabel, emptyYData, emptyTimestamps);
        qDebug() << "Added series" << seriesLabel << "to data source" << graphTypeToString(graphType);
    }
    else
    {
        qDebug() << "Data source not found:" << graphTypeToString(graphType);
    }
}

void GraphLayout::removeSeriesFromDataSource(const GraphType &graphType, const QString &seriesLabel)
{
    auto it = m_dataSources.find(graphType);
    if (it != m_dataSources.end())
    {
        it->second->clearDataSeries(seriesLabel);
        qDebug() << "Cleared series" << seriesLabel << "from data source" << graphTypeToString(graphType);
    }
    else
    {
        qDebug() << "Data source not found:" << graphTypeToString(graphType);
    }
}

// Container management

std::vector<QString> GraphLayout::getContainerLabels() const
{
    return m_containerLabels;
}

bool GraphLayout::hasContainer(const GraphType &graphType) const
{
    return std::find(m_containerLabels.begin(), m_containerLabels.end(), graphTypeToString(graphType)) != m_containerLabels.end();
}

int GraphLayout::getContainerIndex(const QString &containerLabel) const
{
    auto it = std::find(m_containerLabels.begin(), m_containerLabels.end(), containerLabel);
    if (it != m_containerLabels.end())
    {
        return std::distance(m_containerLabels.begin(), it);
    }
    return -1; // Not found
}

void GraphLayout::disconnectAllContainerConnections()
{
    qDebug() << "GraphLayout: Disconnecting external container connections";

    // Disconnect external connections to prevent duplicate connections
    // while preserving internal connections like TimelineView -> GraphContainer
    for (auto *container : m_graphContainers)
    {
        if (container)
        {
            // Disconnect IntervalChanged, TimeScopeChanged, TimeSelectionCreated, and TimeSelectionsCleared signals to preserve internal functionality
            container->disconnect(SIGNAL(IntervalChanged(TimeInterval)));
            container->disconnect(SIGNAL(TimeScopeChanged(TimeSelectionSpan)));
            container->disconnect(SIGNAL(TimeSelectionCreated(TimeSelectionSpan)));
            container->disconnect(SIGNAL(TimeSelectionsCleared()));
            qDebug() << "GraphLayout: Disconnected external signals from container";
        }
    }
}

void GraphLayout::setCurrentTime(const QTime &time)
{
    for (auto *container : m_graphContainers)
    {
        if (container)
        {
            container->setCurrentTime(time);
        }
    }
}

void GraphLayout::deleteInteractiveMarkers()
{
    qDebug() << "GraphLayout: deleteInteractiveMarkers invoked";
    for (auto *container : m_graphContainers)
    {
        if (container)
        {
            container->deleteInteractiveMarkers();
        }
    }
}

void GraphLayout::linkHorizontalContainers()
{
    qDebug() << "GraphLayout: Linking horizontal containers for layout type:" << static_cast<int>(m_layoutType);

    // Disconnect all existing connections first to avoid duplicates
    disconnectAllContainerConnections();

    // Note: Interval synchronization is now handled centrally by GraphLayout
    // via onContainerIntervalChanged, which uses setTimeInterval API to avoid signal loops.
    // Only TimeScopeChanged needs direct container-to-container connections for now.

    switch (m_layoutType)
    {
    case LayoutType::GPW4W:
        // Link row 1: container 0 -> container 1 (time scope change)
        connect(m_graphContainers[0], &GraphContainer::TimeScopeChanged,
                m_graphContainers[1], &GraphContainer::onTimeScopeChanged);

        // Link row 2: container 2 -> container 3 (time scope change)
        connect(m_graphContainers[2], &GraphContainer::TimeScopeChanged,
                m_graphContainers[3], &GraphContainer::onTimeScopeChanged);

        qDebug() << "GraphLayout: Linked containers for GPW4W layout";
        break;

    case LayoutType::GPW2WH:
        // Link horizontal: container 0 -> container 1 (time scope change)
        connect(m_graphContainers[0], &GraphContainer::TimeScopeChanged,
                m_graphContainers[1], &GraphContainer::onTimeScopeChanged);

        qDebug() << "GraphLayout: Linked containers for GPW2WH layout";
        break;

    case LayoutType::GPW4WH:
        // Link horizontal: container 2 (has timeline view) -> containers 0, 1, 3 (interval change)
        connect(m_graphContainers[2], &GraphContainer::IntervalChanged,
                m_graphContainers[0], &GraphContainer::onTimeIntervalChanged);
        connect(m_graphContainers[2], &GraphContainer::IntervalChanged,
                m_graphContainers[1], &GraphContainer::onTimeIntervalChanged);
        connect(m_graphContainers[2], &GraphContainer::IntervalChanged,
                m_graphContainers[3], &GraphContainer::onTimeIntervalChanged);
        
        // Link horizontal: container 2 (has timeline view) -> containers 0, 1, 3 (time scope change)
        connect(m_graphContainers[2], &GraphContainer::TimeScopeChanged,
                m_graphContainers[0], &GraphContainer::onTimeScopeChanged);
        connect(m_graphContainers[2], &GraphContainer::TimeScopeChanged,
                m_graphContainers[1], &GraphContainer::onTimeScopeChanged);
        connect(m_graphContainers[2], &GraphContainer::TimeScopeChanged,
                m_graphContainers[3], &GraphContainer::onTimeScopeChanged);

        qDebug() << "GraphLayout: Linked containers for GPW4WH layout";
        break;

    case LayoutType::GPW1W:
    case LayoutType::GPW2WV:
    case LayoutType::HIDDEN:
        // No horizontal linking needed for these layouts
        qDebug() << "GraphLayout: No horizontal linking needed for layout type:" << static_cast<int>(m_layoutType);
        break;

    default:
        qWarning() << "GraphLayout: Unknown layout type for horizontal linking:" << static_cast<int>(m_layoutType);
        break;
    }
}

void GraphLayout::syncAllTimelineViews()
{
    qDebug() << "GraphLayout: Syncing all timeline views for layout type:" << static_cast<int>(m_layoutType);
    
    // This function ensures all timeline views in the layout are properly synchronized:
    // 1. Timeline views are connected to each other for interval and scope changes
    // 2. Timeline views are connected to all visible containers (including those without timeline views)
    // 3. Internal connections (TimelineView -> its own container) are preserved
    // 4. Works for all layout types: GPW1W, GPW2WV, GPW4W, GPW2WH, GPW4WH, NOGPW2WH
    
    // Collect all visible TimelineView instances with their containers
    // This includes timeline views from all visible containers, regardless of layout
    std::vector<std::pair<GraphContainer*, TimelineView*>> timelineViewPairs;
    for (auto *container : m_graphContainers)
    {
        if (container && container->isVisible() && container->getShowTimelineView())
        {
            TimelineView *timelineView = container->getTimelineView();
            if (timelineView)
            {
                timelineViewPairs.push_back({container, timelineView});
                qDebug() << "GraphLayout: Found timeline view in container";
            }
            else
            {
                qWarning() << "GraphLayout: Container has showTimelineView=true but timelineView is null";
            }
        }
    }
    
    if (timelineViewPairs.size() <= 1)
    {
        if (timelineViewPairs.size() == 1)
        {
            qDebug() << "GraphLayout: Only 1 timeline view found, ensuring internal connections are set up";
            // Even with 1 timeline view, ensure internal connections are properly set up
            const auto &pair = timelineViewPairs[0];
            if (pair.first && pair.second)
            {
                connect(pair.second, &TimelineView::TimeIntervalChanged,
                        pair.first, &GraphContainer::onTimeIntervalChanged, Qt::UniqueConnection);
                connect(pair.second, &TimelineView::TimeScopeChanged,
                        pair.first, &GraphContainer::onTimeScopeChanged, Qt::UniqueConnection);
            }
        }
        else
        {
            qDebug() << "GraphLayout: No timeline views found";
        }
        return;
    }
    
    qDebug() << "GraphLayout: Found" << timelineViewPairs.size() << "timeline views to sync";
    
    // Disconnect only the specific external sync connections to avoid duplicates
    // We must be specific to preserve internal connections (like timer, visualizer widget, etc.)
    for (size_t i = 0; i < timelineViewPairs.size(); ++i)
    {
        TimelineView *sourceTimelineView = timelineViewPairs[i].second;
        if (!sourceTimelineView)
            continue;
        
        // Disconnect TimeIntervalChanged connections to other timeline views
        for (size_t j = 0; j < timelineViewPairs.size(); ++j)
        {
            if (i != j && timelineViewPairs[j].second)
            {
                disconnect(sourceTimelineView, &TimelineView::TimeIntervalChanged,
                          timelineViewPairs[j].second, &TimelineView::setTimeLineLength);
            }
        }
        
        // Disconnect TimeScopeChanged connections to other timeline views
        for (size_t j = 0; j < timelineViewPairs.size(); ++j)
        {
            if (i != j && timelineViewPairs[j].second)
            {
                disconnect(sourceTimelineView, &TimelineView::TimeScopeChanged,
                          timelineViewPairs[j].second, &TimelineView::setVisibleTimeWindow);
            }
        }
        
        // Disconnect TimeScopeChanged connections to containers (external sync only)
        for (auto *container : m_graphContainers)
        {
            if (container && container->isVisible())
            {
                // Only disconnect if this is NOT the timeline view's own container
                // (we want to preserve the internal connection)
                bool isOwnContainer = false;
                for (const auto &pair : timelineViewPairs)
                {
                    if (pair.first == container && pair.second == sourceTimelineView)
                    {
                        isOwnContainer = true;
                        break;
                    }
                }
                
                if (!isOwnContainer)
                {
                    disconnect(sourceTimelineView, &TimelineView::TimeScopeChanged,
                              container, &GraphContainer::onTimeScopeChanged);
                }
            }
        }
    }
    
    // Connect all timeline views to each other for interval changes
    // When one timeline view's interval changes, update all others directly
    for (size_t i = 0; i < timelineViewPairs.size(); ++i)
    {
        for (size_t j = 0; j < timelineViewPairs.size(); ++j)
        {
            if (i != j && timelineViewPairs[i].second && timelineViewPairs[j].second)
            {
                // Connect TimeIntervalChanged signal to setTimeLineLength
                // This ensures all timeline views stay in sync when interval changes
                // Use Qt::UniqueConnection to prevent duplicate connections
                connect(timelineViewPairs[i].second, &TimelineView::TimeIntervalChanged,
                        timelineViewPairs[j].second, &TimelineView::setTimeLineLength, Qt::UniqueConnection);
            }
        }
    }
    
    // Ensure each TimelineView is connected to its own container (internal connections)
    // These should already exist, but we ensure they're there
    for (const auto &pair : timelineViewPairs)
    {
        if (pair.first && pair.second)
        {
            // Ensure the internal connection: TimelineView -> its own container
            // Use QOverload to ensure we connect to the right slot signature
            connect(pair.second, &TimelineView::TimeIntervalChanged,
                    pair.first, &GraphContainer::onTimeIntervalChanged, Qt::UniqueConnection);
            connect(pair.second, &TimelineView::TimeScopeChanged,
                    pair.first, &GraphContainer::onTimeScopeChanged, Qt::UniqueConnection);
        }
    }
    
    // Connect timeline views for scope changes - sync slider positions directly
    // When one timeline view's scope changes, update all other timeline views' sliders
    // and update ALL containers (including those without timeline views)
    for (size_t i = 0; i < timelineViewPairs.size(); ++i)
    {
        TimelineView *sourceTimelineView = timelineViewPairs[i].second;
        if (!sourceTimelineView)
            continue;
        
        // Connect source timeline view's TimeScopeChanged to all other timeline views
        for (size_t j = 0; j < timelineViewPairs.size(); ++j)
        {
            if (i != j && timelineViewPairs[j].second)
            {
                // Connect to setVisibleTimeWindow to sync slider positions
                // This ensures all timeline views' sliders stay in sync
                connect(sourceTimelineView, &TimelineView::TimeScopeChanged,
                        timelineViewPairs[j].second, &TimelineView::setVisibleTimeWindow);
            }
        }
        
        // Connect source timeline view's TimeScopeChanged to ALL visible containers
        // This ensures ALL graphs (including those without timeline views) stay in sync
        for (auto *container : m_graphContainers)
        {
            if (container && container->isVisible())
            {
                // Only connect to containers that are NOT the source timeline view's own container
                // (the internal connection is already handled above)
                bool isOwnContainer = false;
                for (const auto &pair : timelineViewPairs)
                {
                    if (pair.first == container && pair.second == sourceTimelineView)
                    {
                        isOwnContainer = true;
                        break;
                    }
                }
                
                if (!isOwnContainer)
                {
                    // Connect to the container's onTimeScopeChanged to update the graph
                    // This ensures all graphs stay in sync with the timeline views
                    connect(sourceTimelineView, &TimelineView::TimeScopeChanged,
                            container, &GraphContainer::onTimeScopeChanged);
                }
            }
        }
    }
    
    qDebug() << "GraphLayout: Timeline views synced successfully";
}

void GraphLayout::onTimerTick()
{
    setCurrentTime(QTime::currentTime());
}

void GraphLayout::onTimeSelectionCreated(const TimeSelectionSpan &selection)
{
    qDebug() << "GraphLayout: Time selection created from" << selection.startTime.toString() << "to" << selection.endTime.toString();

    // Add the selection to the sync state
    m_syncState.timeSelections.push_back(selection);
    
    // Identify the source container to avoid duplicating selection there
    GraphContainer *source = qobject_cast<GraphContainer *>(sender());
    
    // Propagate the selection to all other visible containers
    for (auto *container : m_graphContainers)
    {
        if (container && container != source)
        {
            container->addTimeSelection(selection);
            qDebug() << "GraphLayout: Selection added to container";
        }
    }
    
    // Emit the signal for external components
    emit TimeSelectionCreated(selection);
}


void GraphLayout::onContainerIntervalChanged(TimeInterval interval)
{
    qDebug() << "GraphLayout: Container interval changed to" << timeIntervalToString(interval);
    
    // Update sync state
    m_syncState.currentInterval = interval;
    m_syncState.hasInterval = true;
    
    // Identify the source container to avoid updating it again
    GraphContainer *source = qobject_cast<GraphContainer *>(sender());
    
    // Set the interval on all other containers using the API (no signal emission)
    for (auto *container : m_graphContainers)
    {
        if (container && container != source)
        {
            container->setTimeInterval(interval);
            qDebug() << "GraphLayout: Interval set on container via API";
        }
    }
}

void GraphLayout::onContainerTimeScopeChanged(const TimeSelectionSpan &selection)
{
    qDebug() << "GraphLayout: Container time scope changed from" << selection.startTime.toString() << "to" << selection.endTime.toString();
    
    // Update sync state
    m_syncState.currentTimeScope = selection;
    m_syncState.hasTimeScope = true;
    
    // Identify the source container to avoid updating it again
    GraphContainer *source = qobject_cast<GraphContainer *>(sender());
    
    // Act as hub: propagate time scope to all other containers
    // This ensures all waterfall graphs stay synchronized
    for (auto *container : m_graphContainers)
    {
        if (container && container != source)
        {
            // Use setTimeScope to update without triggering signals (prevents feedback loops)
            container->setTimeScope(selection);
            qDebug() << "GraphLayout: Time scope propagated to container";
        }
    }
    
    qDebug() << "GraphLayout: Time scope synchronized across all containers";
}

// void GraphLayout::onCursorTimeChanged(const QDateTime &time)
// {
//     m_syncState.cursorTime = time;
//     m_syncState.hasCursorTime = true;
// }

// ------------------------------------------------------------

void GraphLayout::propagateTimeSelectionToAllContainers(const TimeSelectionSpan &selection)
{
    qDebug() << "GraphLayout: Propagating time selection to all containers";
    
    // Add the selection to all visible containers
    for (auto *container : m_graphContainers)
    {
        if (container && container->isVisible())
        {
            container->addTimeSelection(selection);
            qDebug() << "GraphLayout: Selection added to container";
        }
    }

    // Emit the signal for external consumers
    emit TimeSelectionCreated(selection);
}

void GraphLayout::registerCursorSyncCallbacks()
{
    for (auto *container : m_graphContainers)
    {
        if (!container)
        {
            continue;
        }

        container->setCursorTimeChangedCallback([this](GraphContainer *source, const QDateTime &time) {
            onContainerCursorTimeChanged(source, time);
        });
    }
}

void GraphLayout::onContainerCursorTimeChanged(GraphContainer *source, const QDateTime &time)
{
    // Update all containers' graphs with the shared cursor time
    for (auto *container : m_graphContainers)
    {
        if (!container || container == source)
        {
            continue;
        }

        container->applySharedTimeAxisCursor(time);
    }
    
    // Update all timeline views with the cursor timestamp label
    // This shows the timestamp when the horizontal cursor line intersects the timeline view
    // Skip the source container since it updates its own timeline view locally
    for (auto *container : m_graphContainers)
    {
        if (!container || container == source || !container->isVisible())
        {
            continue;
        }
        
        if (container->getShowTimelineView())
        {
            TimelineView *timelineView = container->getTimelineView();
            if (timelineView)
            {
                if (time.isValid())
                {
                    timelineView->updateCrosshairTimestampFromTime(time);
                }
                else
                {
                    timelineView->clearCrosshairTimestamp();
                }
            }
        }
    }
}

void GraphLayout::onTimeSelectionsCleared()
{
    qDebug() << "GraphLayout: Time selections cleared by one container - clearing in all containers";
    
    // Clear the sync state
    m_syncState.timeSelections.clear();
    
    // Identify the source container to avoid cyclic re-emission
    GraphContainer *source = qobject_cast<GraphContainer *>(sender());
    
    // Clear selections in all other visible containers silently
    for (auto *container : m_graphContainers)
    {
        if (container && container != source)
        {
            container->clearTimeSelectionsSilent();
        }
    }

    // Emit the signal for external consumers
    emit TimeSelectionsCleared();
}

void GraphLayout::onBTWManualMarkerPlaced(const QDateTime &timestamp, const QPointF &position)
{
    qDebug() << "GraphLayout: BTW manual marker placed at timestamp" << timestamp.toString() << "position" << position;
    
    // Find the BTW graph to get the range value from the X position
    qreal range = 0.0;
    bool foundRange = false;
    
    for (auto *container : m_graphContainers)
    {
        if (!container)
            continue;
            
        // Check if this container has BTW as current option
        if (container->getCurrentDataOption() == GraphType::BTW)
        {
            WaterfallGraph *graph = container->getCurrentWaterfallGraph();
            if (graph)
            {
                // Convert X position to range value
                range = graph->mapScreenXToRange(position.x());
                foundRange = true;
                qDebug() << "GraphLayout: Calculated range" << range << "from X position" << position.x();
                break;
            }
        }
    }
    
    // If we couldn't find the range, try to get it from BTW data source
    if (!foundRange)
    {
        WaterfallData *btwDataSource = getDataSource(GraphType::BTW);
        if (btwDataSource && !btwDataSource->isEmpty())
        {
            // Try to find a data point near this timestamp
            std::vector<QString> seriesLabels = btwDataSource->getDataSeriesLabels();
            for (const QString &seriesLabel : seriesLabels)
            {
                const std::vector<QDateTime> &timestamps = btwDataSource->getTimestampsSeries(seriesLabel);
                const std::vector<qreal> &yData = btwDataSource->getYDataSeries(seriesLabel);
                
                for (size_t i = 0; i < timestamps.size(); ++i)
                {
                    qint64 timeDiff = qAbs(timestamps[i].msecsTo(timestamp));
                    if (timeDiff < 1000) // Within 1 second
                    {
                        range = yData[i];
                        foundRange = true;
                        qDebug() << "GraphLayout: Found range" << range << "from data at timestamp";
                        break;
                    }
                }
                if (foundRange) break;
            }
        }
    }
    
    // If still no range found, use a default value
    if (!foundRange)
    {
        range = 50.0; // Default range value
        qDebug() << "GraphLayout: Using default range" << range << "for BTW marker";
    }
    
    // Add magenta circle (BTW symbol) to all graphs at this timestamp
    // The range parameter is not needed - we'll find the data point at this timestamp in each graph
    addBTWSymbolToAllGraphs(timestamp, 0.0); // Range parameter is ignored, we find it from data points
}

// Chevron label control methods implementation - operate on all visible containers
void GraphLayout::setChevronLabel1(const QString &label)
{
    for (auto *container : m_graphContainers)
    {
        if (container)
        {
            container->setChevronLabel1(label);
        }
    }
    qDebug() << "GraphLayout: Set chevron label 1 to:" << label << "for all visible containers";
}

void GraphLayout::setChevronLabel2(const QString &label)
{
    for (auto *container : m_graphContainers)
    {
        if (container)
        {
            container->setChevronLabel2(label);
        }
    }
    qDebug() << "GraphLayout: Set chevron label 2 to:" << label << "for all visible containers";
}

void GraphLayout::setChevronLabel3(const QString &label)
{
    for (auto *container : m_graphContainers)
    {
        if (container)
        {
            container->setChevronLabel3(label);
        }
    }
    qDebug() << "GraphLayout: Set chevron label 3 to:" << label << "for all visible containers";
}

QString GraphLayout::getChevronLabel1() const
{
    // Return the label from the first visible container
    for (auto *container : m_graphContainers)
    {
        if (container)
        {
            return container->getChevronLabel1();
        }
    }
    qWarning() << "GraphLayout: No visible containers found to get chevron label";
    return QString();
}

QString GraphLayout::getChevronLabel2() const
{
    // Return the label from the first visible container
    for (auto *container : m_graphContainers)
    {
        if (container)
        {
            return container->getChevronLabel2();
        }
    }
    qWarning() << "GraphLayout: No visible containers found to get chevron label";
    return QString();
}

QString GraphLayout::getChevronLabel3() const
{
    // Return the label from the first visible container
    for (auto *container : m_graphContainers)
    {
        if (container)
        {
            return container->getChevronLabel3();
        }
    }
    qWarning() << "GraphLayout: No visible containers found to get chevron label";
    return QString();
}

// Chevron label control methods implementation - operate on specific container by label
void GraphLayout::setChevronLabel1(const QString &containerLabel, const QString &label)
{
    int containerIndex = getContainerIndex(containerLabel);
    if (containerIndex >= 0 && containerIndex < static_cast<int>(m_graphContainers.size()))
    {
        m_graphContainers[containerIndex]->setChevronLabel1(label);
        qDebug() << "GraphLayout: Set chevron label 1 to:" << label << "for container:" << containerLabel;
    }
    else
    {
        qDebug() << "GraphLayout: Container not found:" << containerLabel;
    }
}

void GraphLayout::setChevronLabel2(const QString &containerLabel, const QString &label)
{
    int containerIndex = getContainerIndex(containerLabel);
    if (containerIndex >= 0 && containerIndex < static_cast<int>(m_graphContainers.size()))
    {
        m_graphContainers[containerIndex]->setChevronLabel2(label);
        qDebug() << "GraphLayout: Set chevron label 2 to:" << label << "for container:" << containerLabel;
    }
    else
    {
        qDebug() << "GraphLayout: Container not found:" << containerLabel;
    }
}

void GraphLayout::setChevronLabel3(const QString &containerLabel, const QString &label)
{
    int containerIndex = getContainerIndex(containerLabel);
    if (containerIndex >= 0 && containerIndex < static_cast<int>(m_graphContainers.size()))
    {
        m_graphContainers[containerIndex]->setChevronLabel3(label);
        qDebug() << "GraphLayout: Set chevron label 3 to:" << label << "for container:" << containerLabel;
    }
    else
    {
        qDebug() << "GraphLayout: Container not found:" << containerLabel;
    }
}

QString GraphLayout::getChevronLabel1(const QString &containerLabel) const
{
    int containerIndex = getContainerIndex(containerLabel);
    if (containerIndex >= 0 && containerIndex < static_cast<int>(m_graphContainers.size()))
    {
        return m_graphContainers[containerIndex]->getChevronLabel1();
    }
    else
    {
        qDebug() << "GraphLayout: Container not found:" << containerLabel;
        return QString();
    }
}

QString GraphLayout::getChevronLabel2(const QString &containerLabel) const
{
    int containerIndex = getContainerIndex(containerLabel);
    if (containerIndex >= 0 && containerIndex < static_cast<int>(m_graphContainers.size()))
    {
        return m_graphContainers[containerIndex]->getChevronLabel2();
    }
    else
    {
        qDebug() << "GraphLayout: Container not found:" << containerLabel;
        return QString();
    }
}

QString GraphLayout::getChevronLabel3(const QString &containerLabel) const
{
    int containerIndex = getContainerIndex(containerLabel);
    if (containerIndex >= 0 && containerIndex < static_cast<int>(m_graphContainers.size()))
    {
        return m_graphContainers[containerIndex]->getChevronLabel3();
    }
    else
    {
        qDebug() << "GraphLayout: Container not found:" << containerLabel;
        return QString();
    }
}

void GraphLayout::setHardRangeLimits(const GraphType graphType, qreal yMin, qreal yMax)
{
    for (auto *container : m_graphContainers)
    {
        if (container)
        {
            container->setGraphRangeLimits(graphType, yMin, yMax);
        }
    }
}

void GraphLayout::removeHardRangeLimits(const GraphType graphType)
{

    // loop through all containers
    for (auto *container : m_graphContainers)
    {
        if (container)
        {
            container->removeGraphRangeLimits(graphType);
        }
    }
}

void GraphLayout::clearAllHardRangeLimits()
{
    for (auto *container : m_graphContainers)
    {
        container->clearAllGraphRangeLimits();
    }
}

void GraphLayout::clearAllGraphs()
{
    qDebug() << "GraphLayout: clearAllGraphs() - clearing all data, markers, and symbols from all graphs";
    
    // Clear all data sources
    for (auto &pair : m_dataSources)
    {
        WaterfallData *dataSource = pair.second;
        if (dataSource)
        {
            // Clear all data series
            dataSource->clearAllDataSeries();
            
            // Clear all markers and symbols
            dataSource->clearRTWSymbols();
            dataSource->clearBTWSymbols();
            dataSource->clearBTWMarkers();
            dataSource->clearRTWRMarkers();
            
            qDebug() << "GraphLayout: Cleared data for graph type:" << static_cast<int>(pair.first);
        }
    }
    
    // Trigger redraws on all containers
    for (auto *container : m_graphContainers)
    {
        if (container)
        {
            container->redrawWaterfallGraph();
            qDebug() << "GraphLayout: Triggered redraw for container";
        }
    }
    
    qDebug() << "GraphLayout: clearAllGraphs() completed";
}

// Marker and symbol management methods implementation

void GraphLayout::addRTWSymbol(const GraphType &graphType, const QString &symbolName, const QDateTime &timestamp, qreal range)
{
    auto it = m_dataSources.find(graphType);
    if (it != m_dataSources.end() && it->second)
    {
        it->second->addRTWSymbol(symbolName, timestamp, range);
        redrawGraph(graphType);
        qDebug() << "GraphLayout: Added RTW symbol" << symbolName << "to graph type" << static_cast<int>(graphType);
    }
    else
    {
        qDebug() << "GraphLayout: Cannot add RTW symbol - data source not found for graph type" << static_cast<int>(graphType);
    }
}

void GraphLayout::addBTWSymbol(const GraphType &graphType, const QString &symbolName, const QDateTime &timestamp, qreal range)
{
    auto it = m_dataSources.find(graphType);
    if (it != m_dataSources.end() && it->second)
    {
        it->second->addBTWSymbol(symbolName, timestamp, range);
        redrawGraph(graphType);
        qDebug() << "GraphLayout: Added BTW symbol" << symbolName << "to graph type" << static_cast<int>(graphType);
    }
    else
    {
        qDebug() << "GraphLayout: Cannot add BTW symbol - data source not found for graph type" << static_cast<int>(graphType);
    }
}

void GraphLayout::addBTWMarker(const GraphType &graphType, const QDateTime &timestamp, qreal range, qreal delta)
{
    auto it = m_dataSources.find(graphType);
    if (it != m_dataSources.end() && it->second)
    {
        it->second->addBTWMarker(timestamp, range, delta);
        redrawGraph(graphType);
        qDebug() << "GraphLayout: Added BTW marker to graph type" << static_cast<int>(graphType);
        
        // Add magenta circle (BTW symbol) to all other graphs at this timestamp
        addBTWSymbolToAllGraphs(timestamp, range);
    }
    else
    {
        qDebug() << "GraphLayout: Cannot add BTW marker - data source not found for graph type" << static_cast<int>(graphType);
    }
}

void GraphLayout::addRTWRMarker(const GraphType &graphType, const QDateTime &timestamp, qreal range)
{
    auto it = m_dataSources.find(graphType);
    if (it != m_dataSources.end() && it->second)
    {
        it->second->addRTWRMarker(timestamp, range);
        redrawGraph(graphType);
        qDebug() << "GraphLayout: Added RTW R marker to graph type" << static_cast<int>(graphType);
    }
    else
    {
        qDebug() << "GraphLayout: Cannot add RTW R marker - data source not found for graph type" << static_cast<int>(graphType);
    }
}

bool GraphLayout::removeBTWMarker(const GraphType &graphType, const QDateTime &timestamp, qreal range, qreal toleranceMs, qreal rangeTolerance)
{
    auto it = m_dataSources.find(graphType);
    if (it != m_dataSources.end() && it->second)
    {
        bool removed = it->second->removeBTWMarker(timestamp, range, toleranceMs, rangeTolerance);
        if (removed)
        {
            redrawGraph(graphType);
            qDebug() << "GraphLayout: Removed BTW marker from graph type" << static_cast<int>(graphType);
        }
        return removed;
    }
    else
    {
        qDebug() << "GraphLayout: Cannot remove BTW marker - data source not found for graph type" << static_cast<int>(graphType);
        return false;
    }
}

bool GraphLayout::removeRTWRMarker(const GraphType &graphType, const QDateTime &timestamp, qreal range, qreal toleranceMs, qreal rangeTolerance)
{
    auto it = m_dataSources.find(graphType);
    if (it != m_dataSources.end() && it->second)
    {
        bool removed = it->second->removeRTWRMarker(timestamp, range, toleranceMs, rangeTolerance);
        if (removed)
        {
            redrawGraph(graphType);
            qDebug() << "GraphLayout: Removed RTW R marker from graph type" << static_cast<int>(graphType);
        }
        return removed;
    }
    else
    {
        qDebug() << "GraphLayout: Cannot remove RTW R marker - data source not found for graph type" << static_cast<int>(graphType);
        return false;
    }
}

void GraphLayout::clearRTWSymbols(const GraphType &graphType)
{
    auto it = m_dataSources.find(graphType);
    if (it != m_dataSources.end() && it->second)
    {
        it->second->clearRTWSymbols();
        redrawGraph(graphType);
        qDebug() << "GraphLayout: Cleared RTW symbols for graph type" << static_cast<int>(graphType);
    }
    else
    {
        qDebug() << "GraphLayout: Cannot clear RTW symbols - data source not found for graph type" << static_cast<int>(graphType);
    }
}

void GraphLayout::clearBTWSymbols(const GraphType &graphType)
{
    auto it = m_dataSources.find(graphType);
    if (it != m_dataSources.end() && it->second)
    {
        it->second->clearBTWSymbols();
        redrawGraph(graphType);
        qDebug() << "GraphLayout: Cleared BTW symbols for graph type" << static_cast<int>(graphType);
    }
    else
    {
        qDebug() << "GraphLayout: Cannot clear BTW symbols - data source not found for graph type" << static_cast<int>(graphType);
    }
}

void GraphLayout::clearBTWMarkers(const GraphType &graphType)
{
    auto it = m_dataSources.find(graphType);
    if (it != m_dataSources.end() && it->second)
    {
        it->second->clearBTWMarkers();
        redrawGraph(graphType);
        qDebug() << "GraphLayout: Cleared BTW markers for graph type" << static_cast<int>(graphType);
    }
    else
    {
        qDebug() << "GraphLayout: Cannot clear BTW markers - data source not found for graph type" << static_cast<int>(graphType);
    }
}

void GraphLayout::clearRTWRMarkers(const GraphType &graphType)
{
    auto it = m_dataSources.find(graphType);
    if (it != m_dataSources.end() && it->second)
    {
        it->second->clearRTWRMarkers();
        redrawGraph(graphType);
        qDebug() << "GraphLayout: Cleared RTW R markers for graph type" << static_cast<int>(graphType);
    }
    else
    {
        qDebug() << "GraphLayout: Cannot clear RTW R markers - data source not found for graph type" << static_cast<int>(graphType);
    }
}

void GraphLayout::redrawGraph(const GraphType &graphType)
{
    // Redraw all containers that have this graph type available (not just currently displayed)
    // This ensures symbols/markers appear even if the graph type isn't currently visible
    for (auto *container : m_graphContainers)
    {
        if (container)
        {
            // Redraw the specific graph type in this container
            container->redrawWaterfallGraph(graphType);
            qDebug() << "GraphLayout: Redrew graph type" << static_cast<int>(graphType) << "in container";
        }
    }
}

void GraphLayout::redrawAllGraphs()
{
    for (auto *container : m_graphContainers)
    {
        if (container)
        {
            container->redrawWaterfallGraph();
        }
    }
    qDebug() << "GraphLayout: Redrew all graphs";
}

void GraphLayout::addBTWSymbolToAllGraphs(const QDateTime &timestamp, qreal /* unusedRange */)
{
    qDebug() << "GraphLayout: Adding BTW symbol (magenta circle) to all graphs at timestamp" << timestamp.toString();
    
    // Get all graph types
    std::vector<GraphType> allGraphTypes = getDataSourceLabels();
    
    for (GraphType graphType : allGraphTypes)
    {
        // Skip BTW graphs (they already have the marker)
        if (graphType == GraphType::BTW)
        {
            continue;
        }
        
        // Get data source for this graph type
        WaterfallData *dataSource = getDataSource(graphType);
        if (!dataSource || dataSource->isEmpty())
        {
            qDebug() << "GraphLayout: Skipping graph type" << static_cast<int>(graphType) << "- no data source or empty";
            continue;
        }
        
        // Find the data point at this timestamp in this graph's data
        // We need to find the range (Y value) of the data point at this timestamp
        qreal dataPointRange = 0.0;
        bool foundDataPoint = false;
        
        // Get all series labels for this data source
        std::vector<QString> seriesLabels = dataSource->getDataSeriesLabels();
        
        // Try to find a data point at the given timestamp (within tolerance)
        const qint64 timeToleranceMs = 1000; // 1 second tolerance
        qint64 closestTimeDiff = timeToleranceMs;
        
        for (const QString &seriesLabel : seriesLabels)
        {
            const std::vector<QDateTime> &timestamps = dataSource->getTimestampsSeries(seriesLabel);
            const std::vector<qreal> &yData = dataSource->getYDataSeries(seriesLabel);
            
            // Find the closest data point to the target timestamp
            for (size_t i = 0; i < timestamps.size(); ++i)
            {
                qint64 timeDiff = qAbs(timestamps[i].msecsTo(timestamp));
                if (timeDiff < closestTimeDiff)
                {
                    closestTimeDiff = timeDiff;
                    dataPointRange = yData[i];
                    foundDataPoint = true;
                }
            }
        }
        
        if (!foundDataPoint)
        {
            qDebug() << "GraphLayout: No data point found at timestamp" << timestamp.toString() << "in graph type" << static_cast<int>(graphType) << "- skipping";
            continue;
        }
        
        qDebug() << "GraphLayout: Found data point at timestamp" << timestamp.toString() << "in graph type" << static_cast<int>(graphType) << "with range" << dataPointRange;
        
        // Check if symbol already exists at this timestamp (deduplication)
        std::vector<BTWSymbolData> existingSymbols = dataSource->getBTWSymbols();
        bool symbolExists = false;
        for (const auto& existingSymbol : existingSymbols)
        {
            // Check if symbol exists at the same timestamp (within 100ms tolerance)
            qint64 timeDiff = qAbs(existingSymbol.timestamp.msecsTo(timestamp));
            if (timeDiff < 100 && existingSymbol.symbolName == "MagentaCircle")
            {
                symbolExists = true;
                break;
            }
        }
        
        if (symbolExists)
        {
            qDebug() << "GraphLayout: BTW symbol already exists in" << static_cast<int>(graphType) << "at this timestamp, skipping";
            continue;
        }
        
        // Add magenta circle symbol to this graph's data source
        // Use the range value from the data point at this timestamp (not the BTW marker's range)
        dataSource->addBTWSymbol("MagentaCircle", timestamp, dataPointRange);
        qDebug() << "GraphLayout: Added BTW symbol to graph type" << static_cast<int>(graphType) << "at timestamp" << timestamp.toString() << "with range" << dataPointRange << "(from data point)";
        
        // Verify the symbol was added
        size_t symbolCount = dataSource->getBTWSymbolsCount();
        qDebug() << "GraphLayout: Verified - graph type" << static_cast<int>(graphType) << "now has" << symbolCount << "BTW symbols";
        
        // Trigger redraw of this graph - redraw all containers that might show this graph type
        redrawGraph(graphType);
    }
    
    // Redraw all graphs once to ensure symbols appear in all containers
    redrawAllGraphs();
    
    qDebug() << "GraphLayout: Finished adding BTW symbols to all graphs";
}

bool GraphLayout::hasHardRangeLimits(const GraphType graphType) const
{
    for (auto *container : m_graphContainers)
    {
        if (container)
        {
            return container->hasGraphRangeLimits(graphType);
        }
    }
    return false;
}

std::pair<qreal, qreal> GraphLayout::getHardRangeLimits(const GraphType graphType) const
{
    for (auto *container : m_graphContainers)
    {
        if (container)
        {
            return container->getGraphRangeLimits(graphType);
        }
    }
    return std::make_pair(0.0, 0.0);
}