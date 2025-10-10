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

    // First, make all containers visible
    for (auto *container : m_graphContainers)
    {
        container->setVisible(true);
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
        // Hide timeline view for the second graph container
        m_graphContainers[0]->setShowTimelineView(true);
        m_graphContainers[1]->setShowTimelineView(false);
        // Add graph containers to row 2
        m_graphContainersRow2Layout->addWidget(m_graphContainers[2]);
        m_graphContainersRow2Layout->addWidget(m_graphContainers[3]);
        // Hide timeline view for fourth graph containers
        m_graphContainers[2]->setShowTimelineView(true);
        m_graphContainers[3]->setShowTimelineView(false);

        // Connect the interval change handler of containers 1 to the event of 0 and the 3 to the event of 2
        connect(m_graphContainers[0], &GraphContainer::IntervalChanged, m_graphContainers[1], &GraphContainer::onTimeIntervalChanged);
        connect(m_graphContainers[2], &GraphContainer::IntervalChanged, m_graphContainers[3], &GraphContainer::onTimeIntervalChanged);

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
        // Hide timeline view for the second graph container
        m_graphContainers[0]->setShowTimelineView(true);
        m_graphContainers[1]->setShowTimelineView(false);
        // Hide the other containers
        m_graphContainers[2]->setVisible(false);
        m_graphContainers[3]->setVisible(false);

        // Connect the interval change handler of containers 1 to the event of 0
        connect(m_graphContainers[0], &GraphContainer::IntervalChanged, m_graphContainers[1], &GraphContainer::onTimeIntervalChanged);
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

        // Connect the interval change handlers of containers 1,2,3 to the event of 0
        connect(m_graphContainers[0], &GraphContainer::IntervalChanged, m_graphContainers[1], &GraphContainer::onTimeIntervalChanged);
        connect(m_graphContainers[0], &GraphContainer::IntervalChanged, m_graphContainers[2], &GraphContainer::onTimeIntervalChanged);
        connect(m_graphContainers[0], &GraphContainer::IntervalChanged, m_graphContainers[3], &GraphContainer::onTimeIntervalChanged);
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

    // Update sizing after layout changes
    updateLayoutSizing();

    // Link horizontal containers for selection events
    linkHorizontalContainers();
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
    // Create 4 graph containers with timer
    m_graphContainers.push_back(new GraphContainer(this, true, m_seriesColorsMap, m_timer));
    m_graphContainers.push_back(new GraphContainer(this, true, m_seriesColorsMap, m_timer));
    m_graphContainers.push_back(new GraphContainer(this, true, m_seriesColorsMap, m_timer));
    m_graphContainers.push_back(new GraphContainer(this, true, m_seriesColorsMap, m_timer));

    // Attach data sources to containers
    attachContainerDataSources();
    
    // Connect all containers' TimeSelectionCreated signals to our slot
    for (auto *container : m_graphContainers)
    {
        connect(container, &GraphContainer::TimeSelectionCreated,
                this, &GraphLayout::onTimeSelectionCreated);
    }
    
    qDebug() << "GraphLayout: Connected all containers to time selection propagation";
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
    
    // Calculate widths based on formula: totalWidth = N_Columns * container_width + 80
    int numColumns = 0;
    int containerWidth = 0;
    
    switch (m_layoutType)
    {
    case LayoutType::GPW1W:
        numColumns = 1;
        break;
    case LayoutType::GPW2WH:
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
        // Formula: totalWidth = N_Columns * container_width + 80
        // So: container_width = (totalWidth - 80) / N_Columns
        int availableWidth = currentSize.width();
        containerWidth = (availableWidth - 80) / numColumns;
        
        // Ensure minimum width
        containerWidth = qMax(containerWidth, 200);
        
        // Set container widths based on layout type
        switch (m_layoutType)
        {
        case LayoutType::GPW1W:
            if (m_graphContainers[0] && m_graphContainers[0]->isVisible()) {
                m_graphContainers[0]->setContainerWidth(containerWidth + 80);
            }
            break;
        case LayoutType::GPW2WH:
            for (int i = 0; i < 2; ++i) {
                if (m_graphContainers[i] && m_graphContainers[i]->isVisible()) {
                    if (i == 0) {
                        m_graphContainers[i]->setContainerWidth(containerWidth + 80);
                    } else {
                        m_graphContainers[i]->setContainerWidth(containerWidth);
                    }
                }
            }
            break;
        case LayoutType::GPW4WH:
            for (int i = 0; i < 4; ++i) {
                if (m_graphContainers[i] && m_graphContainers[i]->isVisible()) {
                    if (i == 0) {
                        m_graphContainers[i]->setContainerWidth(containerWidth + 80);
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
                m_graphContainers[0]->setContainerWidth(containerWidth + 80);
            }
            if (m_graphContainers[2] && m_graphContainers[2]->isVisible()) {
                m_graphContainers[2]->setContainerWidth(containerWidth + 80);
            }
            break;
        case LayoutType::GPW4W:
            // For 2x2 grid, first container in each row gets extra width
            for (int i = 0; i < 4; ++i) {
                if (m_graphContainers[i] && m_graphContainers[i]->isVisible()) {
                    if (i == 0 || i == 2) {
                        // First container in each row gets extra 80px for timeline view
                        m_graphContainers[i]->setContainerWidth(containerWidth + 80);
                    } else {
                        // Second container in each row gets standard width
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
            totalWidth = containerWidth + 80;
            break;
        case LayoutType::GPW2WH:
            totalWidth = (containerWidth + 80) + containerWidth;
            break;
        case LayoutType::GPW4WH:
            totalWidth = (containerWidth + 80) + containerWidth + containerWidth + containerWidth;
            break;
        case LayoutType::GPW2WV:
            totalWidth = containerWidth + 80; // Both containers have same width with timeline view
            break;
        case LayoutType::GPW4W:
            // 2x2 grid: first container in each row gets +80px
            totalWidth = (containerWidth + 80) + containerWidth; // Row 1
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

        // Notify all containers that have this data source to update their zoom panels and redraw graphs
        for (auto *container : m_graphContainers)
        {
            if (container && container->hasDataOption(graphType))
            {
                container->initializeZoomPanelLimits();

                // Force redraw of the waterfall graph if it's using this data source
                if (container->getCurrentDataOption() == graphType)
                {
                    container->redrawWaterfallGraph();
                }

                qDebug() << "Updated zoom panel limits for container with data source:" << dataSourceLabel;
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

        // Notify all containers that have this data source to update their zoom panels and redraw graphs
        for (auto *container : m_graphContainers)
        {
            if (container && container->hasDataOption(graphType))
            {
                container->initializeZoomPanelLimits();

                // Force redraw of the waterfall graph if it's using this data source
                if (container->getCurrentDataOption() == graphType)
                {
                    container->redrawWaterfallGraph();
                }

                qDebug() << "Updated zoom panel limits for container with data source:" << dataSourceLabel;
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

        // Notify all containers that have this data source to update their zoom panels and redraw graphs
        for (auto *container : m_graphContainers)
        {
            if (container && container->hasDataOption(graphType))
            {
                container->initializeZoomPanelLimits();

                // Force redraw of the waterfall graph if it's using this data source
                if (container->getCurrentDataOption() == graphType)
                {
                    container->redrawWaterfallGraph();
                }

                qDebug() << "Updated zoom panel limits for container with data source:" << dataSourceLabel;
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

        // Notify all containers that have this data source to update their zoom panels and redraw graphs
        for (auto *container : m_graphContainers)
        {
            if (container && container->hasDataOption(graphType))
            {
                container->initializeZoomPanelLimits();

                // Force redraw of the waterfall graph if it's using this data source
                if (container->getCurrentDataOption() == graphType)
                {
                    container->redrawWaterfallGraph();
                }

                qDebug() << "Updated zoom panel limits for container with data source:" << dataSourceLabel;
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
            // Disconnect IntervalChanged, TimeSelectionCreated, and TimeSelectionsCleared signals to preserve internal functionality
            container->disconnect(SIGNAL(IntervalChanged(TimeInterval)));
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

void GraphLayout::linkHorizontalContainers()
{
    qDebug() << "GraphLayout: Linking horizontal containers for layout type:" << static_cast<int>(m_layoutType);

    // Disconnect all existing connections first to avoid duplicates
    disconnectAllContainerConnections();

    switch (m_layoutType)
    {
    case LayoutType::GPW4W:

        // Row 1
        // Link row 1: container 0 -> container 1
        connect(m_graphContainers[0], &GraphContainer::TimeSelectionCreated,
                m_graphContainers[1], &GraphContainer::addTimeSelection);

        // Link row 1: container 0 (clear selection) -> container 1 (clear selection)
        connect(m_graphContainers[0], &GraphContainer::TimeSelectionsCleared,
                m_graphContainers[1], &GraphContainer::clearTimeSelectionsSilent);

        // Cross link row 1: container 1 -> container 0 (clear selection)
        connect(m_graphContainers[1], &GraphContainer::TimeSelectionsCleared,
                m_graphContainers[0], &GraphContainer::clearTimeSelectionsSilent);

        // Link row 1: container 0 -> container 1 (interval change)
        connect(m_graphContainers[0], &GraphContainer::IntervalChanged,
                m_graphContainers[1], &GraphContainer::onTimeIntervalChanged);

        // Row 2
        // Link row 2: container 2 -> container 3
        connect(m_graphContainers[2], &GraphContainer::TimeSelectionCreated,
                m_graphContainers[3], &GraphContainer::addTimeSelection);

        // Link row 2: container 2 (clear selection) -> container 3 (clear selection)
        connect(m_graphContainers[2], &GraphContainer::TimeSelectionsCleared,
                m_graphContainers[3], &GraphContainer::clearTimeSelectionsSilent);

        // Cross link row 2: container 3 -> container 2 (clear selection)
        connect(m_graphContainers[3], &GraphContainer::TimeSelectionsCleared,
                m_graphContainers[2], &GraphContainer::clearTimeSelectionsSilent);

        // Link row 2: container 2 -> container 3 (interval change)
        connect(m_graphContainers[2], &GraphContainer::IntervalChanged,
                m_graphContainers[3], &GraphContainer::onTimeIntervalChanged);

        qDebug() << "GraphLayout: Linked containers for GPW4W layout";
        break;

    case LayoutType::GPW2WH:
        // Link horizontal: container 0 -> container 1
        connect(m_graphContainers[0], &GraphContainer::TimeSelectionCreated,
                m_graphContainers[1], &GraphContainer::addTimeSelection);

        // Link horizontal: container 0 (clear selection) -> container 1 (clear selection)
        connect(m_graphContainers[0], &GraphContainer::TimeSelectionsCleared,
                m_graphContainers[1], &GraphContainer::clearTimeSelectionsSilent);

        // Cross link horizontal: container 1 -> container 0 (clear selection)
        connect(m_graphContainers[1], &GraphContainer::TimeSelectionsCleared,
                m_graphContainers[0], &GraphContainer::clearTimeSelectionsSilent);

        // Link horizontal: container 0 -> container 1 (interval change)
        connect(m_graphContainers[0], &GraphContainer::IntervalChanged,
                m_graphContainers[1], &GraphContainer::onTimeIntervalChanged);

        qDebug() << "GraphLayout: Linked containers for GPW2WH layout";
        break;

    case LayoutType::GPW4WH:
        // Link horizontal: container 0 -> containers 1, 2, 3 (selection creation)
        connect(m_graphContainers[0], &GraphContainer::TimeSelectionCreated,
                m_graphContainers[1], &GraphContainer::addTimeSelection);
        connect(m_graphContainers[0], &GraphContainer::TimeSelectionCreated,
                m_graphContainers[2], &GraphContainer::addTimeSelection);
        connect(m_graphContainers[0], &GraphContainer::TimeSelectionCreated,
                m_graphContainers[3], &GraphContainer::addTimeSelection);

        // Cross-link horizontal: clear button events (all containers <-> all containers)
        // Container 0 -> containers 1, 2, 3
        connect(m_graphContainers[0], &GraphContainer::TimeSelectionsCleared,
                m_graphContainers[1], &GraphContainer::clearTimeSelectionsSilent);
        connect(m_graphContainers[0], &GraphContainer::TimeSelectionsCleared,
                m_graphContainers[2], &GraphContainer::clearTimeSelectionsSilent);
        connect(m_graphContainers[0], &GraphContainer::TimeSelectionsCleared,
                m_graphContainers[3], &GraphContainer::clearTimeSelectionsSilent);

        // Container 1 -> containers 0, 2, 3
        connect(m_graphContainers[1], &GraphContainer::TimeSelectionsCleared,
                m_graphContainers[0], &GraphContainer::clearTimeSelectionsSilent);
        connect(m_graphContainers[1], &GraphContainer::TimeSelectionsCleared,
                m_graphContainers[2], &GraphContainer::clearTimeSelectionsSilent);
        connect(m_graphContainers[1], &GraphContainer::TimeSelectionsCleared,
                m_graphContainers[3], &GraphContainer::clearTimeSelectionsSilent);

        // Container 2 -> containers 0, 1, 3
        connect(m_graphContainers[2], &GraphContainer::TimeSelectionsCleared,
                m_graphContainers[0], &GraphContainer::clearTimeSelectionsSilent);
        connect(m_graphContainers[2], &GraphContainer::TimeSelectionsCleared,
                m_graphContainers[1], &GraphContainer::clearTimeSelectionsSilent);
        connect(m_graphContainers[2], &GraphContainer::TimeSelectionsCleared,
                m_graphContainers[3], &GraphContainer::clearTimeSelectionsSilent);

        // Container 3 -> containers 0, 1, 2
        connect(m_graphContainers[3], &GraphContainer::TimeSelectionsCleared,
                m_graphContainers[0], &GraphContainer::clearTimeSelectionsSilent);
        connect(m_graphContainers[3], &GraphContainer::TimeSelectionsCleared,
                m_graphContainers[1], &GraphContainer::clearTimeSelectionsSilent);
        connect(m_graphContainers[3], &GraphContainer::TimeSelectionsCleared,
                m_graphContainers[2], &GraphContainer::clearTimeSelectionsSilent);

        // Link horizontal: container 0 -> containers 1, 2, 3 (interval change)
        connect(m_graphContainers[0], &GraphContainer::IntervalChanged,
                m_graphContainers[1], &GraphContainer::onTimeIntervalChanged);
        connect(m_graphContainers[0], &GraphContainer::IntervalChanged,
                m_graphContainers[2], &GraphContainer::onTimeIntervalChanged);
        connect(m_graphContainers[0], &GraphContainer::IntervalChanged,
                m_graphContainers[3], &GraphContainer::onTimeIntervalChanged);

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

void GraphLayout::onTimerTick()
{
    setCurrentTime(QTime::currentTime());
}

void GraphLayout::onTimeSelectionCreated(const TimeSelectionSpan &selection)
{
    qDebug() << "GraphLayout: Time selection created from" << selection.startTime.toString() << "to" << selection.endTime.toString();
    
    // Propagate the selection to all containers
    propagateTimeSelectionToAllContainers(selection);
    
    // Emit the signal for external components
    emit TimeSelectionCreated(selection);
}

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