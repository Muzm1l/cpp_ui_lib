#include "graphlayout.h"
#include <QDebug>

GraphLayout::GraphLayout(QWidget *parent, LayoutType layoutType, const std::vector<QString>& dataSourceLabels)
    : QWidget{parent}
    , m_layoutType(layoutType)
{
    // Initialize data sources based on provided labels
    for (const QString& label : dataSourceLabels) {
        m_dataSources[label] = new WaterfallData(label);
    }
    
    // Initialize container labels (using data source labels as container labels)
    m_containerLabels = dataSourceLabels;
    
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

void GraphLayout::initializeContainers()
{
    // Create 4 graph containers
    m_graphContainers.push_back(new GraphContainer(this, true));
    m_graphContainers.push_back(new GraphContainer(this, true));   
    m_graphContainers.push_back(new GraphContainer(this, true));
    m_graphContainers.push_back(new GraphContainer(this, true));
    
    // Attach data sources to containers
    attachContainerDataSources();
}

void GraphLayout::attachContainerDataSources()
{
    // Go through each of the graph containers and attach each of the 
    // data sources using the key as the title and the value as the datasource
    for (auto* container : m_graphContainers) {
        for (auto& dataSource : m_dataSources) {
            container->addDataOption(dataSource.first, *dataSource.second);
        }
    }
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

// Data point methods implementation

// Methods that operate on all visible containers
void GraphLayout::setData(const std::vector<qreal>& yData, const std::vector<QDateTime>& timestamps)
{
    for (auto* container : m_graphContainers) {
        if (container && container->isVisible()) {
            container->setData(yData, timestamps);
        }
    }
}

void GraphLayout::setData(const WaterfallData& data)
{
    for (auto* container : m_graphContainers) {
        if (container && container->isVisible()) {
            container->setData(data);
        }
    }
}

void GraphLayout::clearData()
{
    for (auto* container : m_graphContainers) {
        if (container && container->isVisible()) {
            container->clearData();
        }
    }
}

void GraphLayout::addDataPoint(qreal yValue, const QDateTime& timestamp)
{
    for (auto* container : m_graphContainers) {
        if (container && container->isVisible()) {
            container->addDataPoint(yValue, timestamp);
        }
    }
}

void GraphLayout::addDataPoints(const std::vector<qreal>& yValues, const std::vector<QDateTime>& timestamps)
{
    for (auto* container : m_graphContainers) {
        if (container && container->isVisible()) {
            container->addDataPoints(yValues, timestamps);
        }
    }
}

// Methods that operate on specific container by label
void GraphLayout::setData(const QString& containerLabel, const std::vector<qreal>& yData, const std::vector<QDateTime>& timestamps)
{
    int containerIndex = getContainerIndex(containerLabel);
    if (containerIndex >= 0 && containerIndex < static_cast<int>(m_graphContainers.size())) {
        m_graphContainers[containerIndex]->setData(yData, timestamps);
    } else {
        qDebug() << "Container not found:" << containerLabel;
    }
}

void GraphLayout::setData(const QString& containerLabel, const WaterfallData& data)
{
    int containerIndex = getContainerIndex(containerLabel);
    if (containerIndex >= 0 && containerIndex < static_cast<int>(m_graphContainers.size())) {
        m_graphContainers[containerIndex]->setData(data);
    } else {
        qDebug() << "Container not found:" << containerLabel;
    }
}

void GraphLayout::clearData(const QString& containerLabel)
{
    int containerIndex = getContainerIndex(containerLabel);
    if (containerIndex >= 0 && containerIndex < static_cast<int>(m_graphContainers.size())) {
        m_graphContainers[containerIndex]->clearData();
    } else {
        qDebug() << "Container not found:" << containerLabel;
    }
}

void GraphLayout::addDataPoint(const QString& containerLabel, qreal yValue, const QDateTime& timestamp)
{
    int containerIndex = getContainerIndex(containerLabel);
    if (containerIndex >= 0 && containerIndex < static_cast<int>(m_graphContainers.size())) {
        m_graphContainers[containerIndex]->addDataPoint(yValue, timestamp);
    } else {
        qDebug() << "Container not found:" << containerLabel;
    }
}

void GraphLayout::addDataPoints(const QString& containerLabel, const std::vector<qreal>& yValues, const std::vector<QDateTime>& timestamps)
{
    int containerIndex = getContainerIndex(containerLabel);
    if (containerIndex >= 0 && containerIndex < static_cast<int>(m_graphContainers.size())) {
        m_graphContainers[containerIndex]->addDataPoints(yValues, timestamps);
    } else {
        qDebug() << "Container not found:" << containerLabel;
    }
}

// Data options management - operate on specific container by label

void GraphLayout::addDataOption(const QString& containerLabel, const QString& title, WaterfallData& dataSource)
{
    int containerIndex = getContainerIndex(containerLabel);
    if (containerIndex >= 0 && containerIndex < static_cast<int>(m_graphContainers.size())) {
        m_graphContainers[containerIndex]->addDataOption(title, dataSource);
    } else {
        qDebug() << "Container not found:" << containerLabel;
    }
}

void GraphLayout::removeDataOption(const QString& containerLabel, const QString& title)
{
    int containerIndex = getContainerIndex(containerLabel);
    if (containerIndex >= 0 && containerIndex < static_cast<int>(m_graphContainers.size())) {
        m_graphContainers[containerIndex]->removeDataOption(title);
    } else {
        qDebug() << "Container not found:" << containerLabel;
    }
}

void GraphLayout::clearDataOptions(const QString& containerLabel)
{
    int containerIndex = getContainerIndex(containerLabel);
    if (containerIndex >= 0 && containerIndex < static_cast<int>(m_graphContainers.size())) {
        m_graphContainers[containerIndex]->clearDataOptions();
    } else {
        qDebug() << "Container not found:" << containerLabel;
    }
}

void GraphLayout::setCurrentDataOption(const QString& containerLabel, const QString& title)
{
    int containerIndex = getContainerIndex(containerLabel);
    if (containerIndex >= 0 && containerIndex < static_cast<int>(m_graphContainers.size())) {
        m_graphContainers[containerIndex]->setCurrentDataOption(title);
    } else {
        qDebug() << "Container not found:" << containerLabel;
    }
}

QString GraphLayout::getCurrentDataOption(const QString& containerLabel) const
{
    int containerIndex = getContainerIndex(containerLabel);
    if (containerIndex >= 0 && containerIndex < static_cast<int>(m_graphContainers.size())) {
        return m_graphContainers[containerIndex]->getCurrentDataOption();
    }
    qDebug() << "Container not found:" << containerLabel;
    return QString();
}

std::vector<QString> GraphLayout::getAvailableDataOptions(const QString& containerLabel) const
{
    int containerIndex = getContainerIndex(containerLabel);
    if (containerIndex >= 0 && containerIndex < static_cast<int>(m_graphContainers.size())) {
        return m_graphContainers[containerIndex]->getAvailableDataOptions();
    }
    qDebug() << "Container not found:" << containerLabel;
    return std::vector<QString>();
}

WaterfallData* GraphLayout::getDataOption(const QString& containerLabel, const QString& title)
{
    int containerIndex = getContainerIndex(containerLabel);
    if (containerIndex >= 0 && containerIndex < static_cast<int>(m_graphContainers.size())) {
        return m_graphContainers[containerIndex]->getDataOption(title);
    }
    qDebug() << "Container not found:" << containerLabel;
    return nullptr;
}

bool GraphLayout::hasDataOption(const QString& containerLabel, const QString& title) const
{
    int containerIndex = getContainerIndex(containerLabel);
    if (containerIndex >= 0 && containerIndex < static_cast<int>(m_graphContainers.size())) {
        return m_graphContainers[containerIndex]->hasDataOption(title);
    }
    qDebug() << "Container not found:" << containerLabel;
    return false;
}

// Data options management - operate on all visible containers

void GraphLayout::addDataOption(const QString& title, WaterfallData& dataSource)
{
    for (auto* container : m_graphContainers) {
        if (container && container->isVisible()) {
            container->addDataOption(title, dataSource);
        }
    }
}

void GraphLayout::removeDataOption(const QString& title)
{
    for (auto* container : m_graphContainers) {
        if (container && container->isVisible()) {
            container->removeDataOption(title);
        }
    }
}

void GraphLayout::clearDataOptions()
{
    for (auto* container : m_graphContainers) {
        if (container && container->isVisible()) {
            container->clearDataOptions();
        }
    }
}

void GraphLayout::setCurrentDataOption(const QString& title)
{
    for (auto* container : m_graphContainers) {
        if (container && container->isVisible()) {
            container->setCurrentDataOption(title);
        }
    }
}

// Data point methods for specific data sources

void GraphLayout::addDataPointToDataSource(const QString& dataSourceLabel, qreal yValue, const QDateTime& timestamp)
{
    auto it = m_dataSources.find(dataSourceLabel);
    if (it != m_dataSources.end()) {
        it->second->addDataPoint(yValue, timestamp);
        qDebug() << "Added data point to" << dataSourceLabel << "y:" << yValue << "time:" << timestamp.toString();
    } else {
        qDebug() << "Data source not found:" << dataSourceLabel;
    }
}

void GraphLayout::addDataPointsToDataSource(const QString& dataSourceLabel, const std::vector<qreal>& yValues, const std::vector<QDateTime>& timestamps)
{
    auto it = m_dataSources.find(dataSourceLabel);
    if (it != m_dataSources.end()) {
        it->second->addDataPoints(yValues, timestamps);
        qDebug() << "Added" << yValues.size() << "data points to" << dataSourceLabel;
    } else {
        qDebug() << "Data source not found:" << dataSourceLabel;
    }
}

void GraphLayout::setDataToDataSource(const QString& dataSourceLabel, const std::vector<qreal>& yData, const std::vector<QDateTime>& timestamps)
{
    auto it = m_dataSources.find(dataSourceLabel);
    if (it != m_dataSources.end()) {
        it->second->setData(yData, timestamps);
        qDebug() << "Set data for" << dataSourceLabel << "size:" << yData.size();
    } else {
        qDebug() << "Data source not found:" << dataSourceLabel;
    }
}

void GraphLayout::setDataToDataSource(const QString& dataSourceLabel, const WaterfallData& data)
{
    auto it = m_dataSources.find(dataSourceLabel);
    if (it != m_dataSources.end()) {
        it->second->setData(data.getYData(), data.getTimestamps());
        qDebug() << "Set data for" << dataSourceLabel << "from WaterfallData object";
    } else {
        qDebug() << "Data source not found:" << dataSourceLabel;
    }
}

void GraphLayout::clearDataSource(const QString& dataSourceLabel)
{
    auto it = m_dataSources.find(dataSourceLabel);
    if (it != m_dataSources.end()) {
        it->second->clearData();
        qDebug() << "Cleared data for" << dataSourceLabel;
    } else {
        qDebug() << "Data source not found:" << dataSourceLabel;
    }
}

// Data source management

WaterfallData* GraphLayout::getDataSource(const QString& dataSourceLabel)
{
    auto it = m_dataSources.find(dataSourceLabel);
    return (it != m_dataSources.end()) ? it->second : nullptr;
}

bool GraphLayout::hasDataSource(const QString& dataSourceLabel) const
{
    return m_dataSources.find(dataSourceLabel) != m_dataSources.end();
}

std::vector<QString> GraphLayout::getDataSourceLabels() const
{
    std::vector<QString> labels;
    for (const auto& pair : m_dataSources) {
        labels.push_back(pair.first);
    }
    return labels;
}

// Container management

std::vector<QString> GraphLayout::getContainerLabels() const
{
    return m_containerLabels;
}

bool GraphLayout::hasContainer(const QString& containerLabel) const
{
    return std::find(m_containerLabels.begin(), m_containerLabels.end(), containerLabel) != m_containerLabels.end();
}

int GraphLayout::getContainerIndex(const QString& containerLabel) const
{
    auto it = std::find(m_containerLabels.begin(), m_containerLabels.end(), containerLabel);
    if (it != m_containerLabels.end()) {
        return std::distance(m_containerLabels.begin(), it);
    }
    return -1; // Not found
}
