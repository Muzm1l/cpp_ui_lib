#include "graphcontainer.h"
#include <QDebug>

GraphContainer::GraphContainer(QWidget *parent, bool showTimelineView)
    : QWidget{parent}, m_showTimelineView(showTimelineView), m_timelineWidth(150), m_graphViewSize(80, 300), currentDataOption("")
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
    m_comboBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    
    // Connect combobox signal to data option change handler
    connect(m_comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GraphContainer::onDataOptionChanged);
    
    // Create ZoomPanel
    m_zoomPanel = new ZoomPanel(this);
    m_zoomPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_zoomPanel->setMaximumHeight(50); // Limit zoom panel height
    
    // Create WaterfallGraph
    m_waterfallGraph = new waterfallgraph(this);
    
    // Set up the data source
    m_waterfallGraph->setDataSource(waterfallData);

    // Enable mouse selection for the waterfall graph 
    m_waterfallGraph->setMouseSelectionEnabled(true);
    
    // Connect waterfall graph selection signal to our handler
    connect(m_waterfallGraph, &waterfallgraph::SelectionCreated,
            this, &GraphContainer::onSelectionCreated);
    
    
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
        
        // Connect TimelineView intervalChanged signal to our slot
        connect(m_timelineView, &TimelineView::intervalChanged, 
                this, &GraphContainer::onTimeIntervalChanged);
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
        
        // Connect TimelineView intervalChanged signal to our slot
        connect(m_timelineView, &TimelineView::intervalChanged, 
                this, &GraphContainer::onTimeIntervalChanged);
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

// Data point methods implementation

void GraphContainer::setData(const std::vector<qreal>& yData, const std::vector<QDateTime>& timestamps)
{
    waterfallData.setData(yData, timestamps);
}

void GraphContainer::setData(const WaterfallData& data)
{
    waterfallData = data;
}

void GraphContainer::clearData()
{
    waterfallData.clearData();
}

void GraphContainer::addDataPoint(qreal yValue, const QDateTime& timestamp)
{
    waterfallData.addDataPoint(yValue, timestamp);
}

void GraphContainer::addDataPoints(const std::vector<qreal>& yValues, const std::vector<QDateTime>& timestamps)
{
    waterfallData.addDataPoints(yValues, timestamps);
}

WaterfallData GraphContainer::getData() const
{
    return waterfallData;
}

std::vector<std::pair<qreal, QDateTime>> GraphContainer::getDataWithinYExtents(qreal yMin, qreal yMax) const
{
    return waterfallData.getDataWithinYExtents(yMin, yMax);
}

std::vector<std::pair<qreal, QDateTime>> GraphContainer::getDataWithinTimeRange(const QDateTime& startTime, const QDateTime& endTime) const
{
    return waterfallData.getDataWithinTimeRange(startTime, endTime);
}

qreal GraphContainer::getMinY() const
{
    return waterfallData.getMinY();
}

qreal GraphContainer::getMaxY() const
{
    return waterfallData.getMaxY();
}

std::pair<qreal, qreal> GraphContainer::getYRange() const
{
    return waterfallData.getYRange();
}

// Data options management implementation

void GraphContainer::addDataOption(const QString& title, WaterfallData& dataSource)
{
    dataOptions[title] = &dataSource;
    updateComboBoxOptions();
    
    // If this is the first option, set it as current
    if (currentDataOption.isEmpty()) {
        setCurrentDataOption(title);
    }
    
    qDebug() << "Added data option:" << title;
}

void GraphContainer::removeDataOption(const QString& title)
{
    auto it = dataOptions.find(title);
    if (it != dataOptions.end()) {
        dataOptions.erase(it);
        updateComboBoxOptions();
        
        // If we removed the current option, switch to another one or clear
        if (currentDataOption == title) {
            if (!dataOptions.empty()) {
                setCurrentDataOption(dataOptions.begin()->first);
            } else {
                currentDataOption.clear();
                m_waterfallGraph->setDataSource(waterfallData);
            }
        }
        
        qDebug() << "Removed data option:" << title;
    }
}

void GraphContainer::clearDataOptions()
{
    dataOptions.clear();
    currentDataOption.clear();
    updateComboBoxOptions();
    m_waterfallGraph->setDataSource(waterfallData);
    
    qDebug() << "Cleared all data options";
}

void GraphContainer::setCurrentDataOption(const QString& title)
{
    auto it = dataOptions.find(title);
    if (it != dataOptions.end()) {
        currentDataOption = title;
        m_waterfallGraph->setDataSource(*it->second);
        
        // Update combobox selection
        int index = m_comboBox->findText(title);
        if (index >= 0) {
            m_comboBox->setCurrentIndex(index);
        }
        
        qDebug() << "Set current data option to:" << title;
    }
}

QString GraphContainer::getCurrentDataOption() const
{
    return currentDataOption;
}

std::vector<QString> GraphContainer::getAvailableDataOptions() const
{
    std::vector<QString> options;
    for (const auto& pair : dataOptions) {
        options.push_back(pair.first);
    }
    return options;
}

WaterfallData* GraphContainer::getDataOption(const QString& title)
{
    auto it = dataOptions.find(title);
    return (it != dataOptions.end()) ? it->second : nullptr;
}

bool GraphContainer::hasDataOption(const QString& title) const
{
    return dataOptions.find(title) != dataOptions.end();
}

void GraphContainer::updateComboBoxOptions()
{
    m_comboBox->clear();
    for (const auto& pair : dataOptions) {
        m_comboBox->addItem(pair.first);
    }
}

void GraphContainer::onDataOptionChanged(int index)
{
    if (index >= 0 && index < m_comboBox->count()) {
        QString selectedTitle = m_comboBox->itemText(index);
        setCurrentDataOption(selectedTitle);
    }
}

void GraphContainer::subscribeToIntervalChange(QObject* subscriber, const char* slot)
{
    if (subscriber && slot) {
        // Use the old Qt syntax for connecting to string-based slots
        connect(this, SIGNAL(IntervalChanged(TimeInterval)), subscriber, slot);
        qDebug() << "GraphContainer: External subscriber connected to interval change signal";
    } else {
        qWarning() << "GraphContainer: Invalid subscriber or slot provided to subscribeToIntervalChange";
    }
}

void GraphContainer::onTimeIntervalChanged(TimeInterval interval)
{
    qDebug() << "GraphContainer: Time interval changed to" << timeIntervalToString(interval);
    
    // Update the waterfall graph time interval
    if (m_waterfallGraph) {
        m_waterfallGraph->setTimeInterval(interval);
    }
    
    // Update the time selection visualizer time interval
    if (m_timelineSelectionView) {
        m_timelineSelectionView->setTimeLineLength(interval);
        qDebug() << "TimeSelectionVisualizer updated with interval:" << timeIntervalToString(interval);
    }
    
    // Emit the signal to notify other components
    emit IntervalChanged(interval);
}

void GraphContainer::onSelectionCreated(const TimeSelectionSpan& selection)
{
    qDebug() << "GraphContainer: Selection created from" << selection.startTime.toString() << "to" << selection.endTime.toString();
    
    if (m_timelineSelectionView) {
        m_timelineSelectionView->addTimeSelection(selection);
        qDebug() << "GraphContainer: Selection added to timeline selection view";
    } else {
        qWarning() << "GraphContainer: Timeline selection view is null";
    }
}

void GraphContainer::setMouseSelectionEnabled(bool enabled)
{
    if (m_waterfallGraph) {
        m_waterfallGraph->setMouseSelectionEnabled(enabled);
        qDebug() << "GraphContainer: Mouse selection" << (enabled ? "enabled" : "disabled");
    }
}

bool GraphContainer::isMouseSelectionEnabled() const
{
    if (m_waterfallGraph) {
        return m_waterfallGraph->isMouseSelectionEnabled();
    }
    return false;
}

void GraphContainer::testSelectionRectangle()
{
    if (m_waterfallGraph) {
        m_waterfallGraph->testSelectionRectangle();
        qDebug() << "GraphContainer: Test selection rectangle called";
    }
}