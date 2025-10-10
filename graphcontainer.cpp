#include "graphcontainer.h"
#include <QDebug>

GraphContainer::GraphContainer(QWidget *parent, bool showTimelineView, std::map<QString, QColor> seriesColorsMap, QTimer *timer, int containerWidth, int containerHeight)
    : QWidget{parent}, m_showTimelineView(showTimelineView), m_timer(timer), m_ownsTimer(false), m_timelineWidth(80), m_graphViewSize(226, 300), currentDataOption(GraphType::BDW), m_seriesColorsMap(seriesColorsMap)
{
    // Set size policy to expand both horizontally and vertically
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Set container geometry if provided
    if (containerWidth > 0 && containerHeight > 0)
    {
        setFixedSize(containerWidth, containerHeight);
    }

    // If the timer is not provided, create a default 1-second timer
    if (!m_timer)
    {
        m_timer = new QTimer(this);
        m_timer->setInterval(1000); // 1 second

        // Connect timer to our tick handler
        connect(m_timer, &QTimer::timeout, this, &GraphContainer::onTimerTick);

        // Start the timer
        m_timer->start();

        qDebug() << "GraphContainer: Timer setup completed since none was provided - interval:" << m_timer->interval() << "ms";
    }

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

    // Create ZoomPanel
    m_zoomPanel = new ZoomPanel(this);
    m_zoomPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    // Set zoompanel height to match combobox height
    int comboboxHeight = m_comboBox->sizeHint().height();
    m_zoomPanel->setMaximumHeight(comboboxHeight);
    m_zoomPanel->setMinimumHeight(comboboxHeight);

    // Add ComboBox and ZoomPanel to left layout first
    m_leftLayout->addWidget(m_comboBox);
    m_leftLayout->addWidget(m_zoomPanel);

    // Create all waterfall graph instances upfront
    createAllWaterfallGraphs();
    
    // Show the initial graph type
    setCurrentDataOption(currentDataOption);

    // Add left layout to main layout with stretch factor
    m_mainLayout->addLayout(m_leftLayout, 1); // Give stretch factor of 1 to left layout

    // Create TimelineSelectionView with timer
    // Calculate combined height of combobox and zoompanel for clear button height
    int zoompanelHeight = m_zoomPanel->maximumHeight();
    int clearButtonHeight = comboboxHeight + zoompanelHeight;
    
    m_timelineSelectionView = new TimeSelectionVisualizer(this, m_timer, clearButtonHeight);
    m_mainLayout->addWidget(m_timelineSelectionView);
    m_timelineSelectionView->setCurrentTime(QTime::currentTime());
    m_timelineSelectionView->setTimeLineLength(TimeInterval::FifteenMinutes);

    // Create TimelineView (conditionally based on showTimelineView) with timer
    if (m_showTimelineView)
    {
        qDebug() << "GraphContainer constructor: Creating TimelineView with showTimelineView = true";
        m_timelineView = new TimelineView(this, m_timer);
        m_mainLayout->addWidget(m_timelineView);
        qDebug() << "GraphContainer constructor: TimelineView created and added to layout";
    }
    else
    {
        qDebug() << "GraphContainer constructor: Not creating TimelineView, showTimelineView = false";
        m_timelineView = nullptr;
    }

    // Set layout
    setLayout(m_mainLayout);

    // Setup all event connections
    setupEventConnections();

    // Initialize container size
    updateTotalContainerSize();
}

void GraphContainer::setupTimer()
{
    // If no timer provided, create a default 1-second timer
    if (!m_timer)
    {
        m_timer = new QTimer(this);
        m_ownsTimer = true;
        m_timer->setInterval(1000); // 1 second
    }

    // Connect timer to our tick handler
    connect(m_timer, &QTimer::timeout, this, &GraphContainer::onTimerTick);

    // Start the timer
    m_timer->start();

    qDebug() << "GraphContainer: Timer setup completed - interval:" << m_timer->interval() << "ms";
}

void GraphContainer::onTimerTick()
{
    // Update current time to all objects in the container
    QTime currentTime = QTime::currentTime();

    if (m_timelineSelectionView)
    {
        m_timelineSelectionView->setCurrentTime(currentTime);
    }

    if (m_timelineView)
    {
        m_timelineView->setCurrentTime(currentTime);
    }

    // qDebug() << "GraphContainer: Timer tick - updated current time to" << currentTime.toString();
}

GraphContainer::~GraphContainer()
{
    // Stop the timer if we own it
    if (m_timer && m_ownsTimer)
    {
        m_timer->stop();
        // Timer will be automatically deleted by Qt's parent-child system
    }
}

void GraphContainer::setShowTimelineView(bool showTimelineView)
{
    qDebug() << "GraphContainer::setShowTimelineView called with:" << showTimelineView;
    m_showTimelineView = showTimelineView;
    if (m_timelineView)
    {
        qDebug() << "GraphContainer: Setting existing TimelineView visibility to:" << showTimelineView;
        m_timelineView->setVisible(showTimelineView);
        qDebug() << "GraphContainer: TimelineView visibility after setting:" << m_timelineView->isVisible();
        qDebug() << "GraphContainer: TimelineView size:" << m_timelineView->size();
    }
    else
    {
        qDebug() << "GraphContainer: Creating new TimelineView with visibility:" << showTimelineView;
        m_timelineView = new TimelineView(this, m_timer);
        m_mainLayout->addWidget(m_timelineView);
        m_timelineView->setVisible(showTimelineView);
        qDebug() << "GraphContainer: New TimelineView visibility after setting:" << m_timelineView->isVisible();
        qDebug() << "GraphContainer: New TimelineView size:" << m_timelineView->size();

        // Re-establish event connections to include the new TimelineView
        setupEventConnections();
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

    // Set the waterfall graph minimum size but allow expansion
    if (m_currentWaterfallGraph)
    {
        m_currentWaterfallGraph->setMinimumSize(m_graphViewSize);
        m_currentWaterfallGraph->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_currentWaterfallGraph->updateGeometry();
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
    if (m_showTimelineView)
    {
        totalWidth += m_timelineWidth;
    }

    // Add spacing between components (1px each)
    totalWidth += 2; // 2 spacings: between graph and timeline selection, and between timeline selection and timeline view

    return QSize(totalWidth, totalHeight);
}

// Container geometry methods
void GraphContainer::setContainerHeight(int height)
{
    if (height > 0)
    {
        QSize currentSize = size();
        setFixedSize(currentSize.width(), height);
    }
    else
    {
        // Remove height constraint by setting size policy back to expanding
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }
}

void GraphContainer::setContainerWidth(int width)
{
    if (width > 0)
    {
        QSize currentSize = size();
        setFixedSize(width, currentSize.height());
    }
    else
    {
        // Remove width constraint by setting size policy back to expanding
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }
}

void GraphContainer::setContainerSize(int width, int height)
{
    if (width > 0 && height > 0)
    {
        setFixedSize(width, height);
    }
    else if (width > 0)
    {
        setContainerWidth(width);
    }
    else if (height > 0)
    {
        setContainerHeight(height);
    }
    else
    {
        // Remove size constraints by setting size policy back to expanding
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }
}

int GraphContainer::getContainerHeight() const
{
    return height();
}

int GraphContainer::getContainerWidth() const
{
    return width();
}

QSize GraphContainer::getContainerSize() const
{
    return size();
}

void GraphContainer::updateTotalContainerSize()
{
    QSize totalSize = getTotalContainerSize();
    setMinimumSize(totalSize);
    setMaximumSize(totalSize);
    updateGeometry();
}

// Data point methods implementation

WaterfallData GraphContainer::getData() const
{
    return waterfallData;
}

std::pair<qreal, qreal> GraphContainer::getYRange() const
{
    return waterfallData.getYRange();
}

void GraphContainer::redrawWaterfallGraph()
{
    if (m_currentWaterfallGraph)
    {
        m_currentWaterfallGraph->draw();
        qDebug() << "GraphContainer: Triggered waterfall graph redraw";
    }
}

// Data options management implementation

void GraphContainer::addDataOption(const GraphType graphType, WaterfallData &dataSource)
{
    QString title = graphTypeToString(graphType);
    dataOptions[graphType] = &dataSource;
    updateComboBoxOptions();

    // Update the data source for the corresponding graph
    auto it = m_waterfallGraphs.find(graphType);
    if (it != m_waterfallGraphs.end())
    {
        it->second->setDataSource(dataSource);
    }

    // If this is the first option, set it as current
    if (dataOptions.size() == 1)
    {
        setCurrentDataOption(graphType);
    }

    qDebug() << "Added data option:" << title;
}

void GraphContainer::removeDataOption(const GraphType graphType)
{
    QString title = graphTypeToString(graphType);
    auto it = dataOptions.find(graphType);
    if (it != dataOptions.end())
    {
        dataOptions.erase(it);
        updateComboBoxOptions();

        // If we removed the current option, switch to another one or clear
        if (currentDataOption == graphType)
        {
            if (!dataOptions.empty())
            {
                setCurrentDataOption(dataOptions.begin()->first);
            }
            else
            {
                currentDataOption = GraphType::BDW;
                // Switch to the default waterfall graph type (visibility toggle)
                initializeWaterfallGraph(currentDataOption);
                // Initialize zoom panel limits for the default data source
                initializeZoomPanelLimits();
            }
        }

        qDebug() << "Removed data option:" << title;
    }
}

void GraphContainer::clearDataOptions()
{
    dataOptions.clear();
    currentDataOption = GraphType::BDW;
    updateComboBoxOptions();

    // Switch to the default waterfall graph type (visibility toggle)
    initializeWaterfallGraph(currentDataOption);

    // Initialize zoom panel limits for the default data source
    initializeZoomPanelLimits();

    qDebug() << "Cleared all data options";
}

void GraphContainer::setCurrentDataOption(const GraphType graphType)
{
    QString title = graphTypeToString(graphType);
    currentDataOption = graphType;

    // Switch to the waterfall graph of this type (visibility toggle)
    initializeWaterfallGraph(graphType);

    // Update combobox selection
    int index = m_comboBox->findText(graphTypeToString(graphType));
    if (index >= 0)
    {
        m_comboBox->setCurrentIndex(index);
    }

    // Initialize zoom panel limits for the new data source
    initializeZoomPanelLimits();

    qDebug() << "Set current data option to:" << title;
}

GraphType GraphContainer::getCurrentDataOption() const
{
    return currentDataOption;
}

std::vector<GraphType> GraphContainer::getAvailableDataOptions() const
{
    std::vector<GraphType> options;
    for (const auto &pair : dataOptions)
    {
        options.push_back(pair.first);
    }
    return options;
}

WaterfallData *GraphContainer::getDataOption(const GraphType graphType)
{
    auto it = dataOptions.find(graphType);
    return (it != dataOptions.end()) ? it->second : nullptr;
}

bool GraphContainer::hasDataOption(const GraphType graphType) const
{
    return dataOptions.find(graphType) != dataOptions.end();
}

void GraphContainer::updateComboBoxOptions()
{
    m_comboBox->clear();
    for (const auto &pair : dataOptions)
    {
        m_comboBox->addItem(graphTypeToString(pair.first));
    }
}

void GraphContainer::onDataOptionChanged(QString title)
{
    GraphType graphTypeEnum = stringToGraphType(title);
    if (graphTypeEnum != currentDataOption)
    {
        setCurrentDataOption(graphTypeEnum);
    }
}

void GraphContainer::setupEventConnections()
{
    // Connect ComboBox data source selection
    connect(m_comboBox, &QComboBox::currentTextChanged,
            this, &GraphContainer::onDataOptionChanged);

    // Connect WaterfallGraph selection events for all graphs
    for (auto &pair : m_waterfallGraphs)
    {
        connect(pair.second, &WaterfallGraph::SelectionCreated,
                this, &GraphContainer::onSelectionCreated);
    }

    // Connect ZoomPanel value changes
    connect(m_zoomPanel, &ZoomPanel::valueChanged,
            this, &GraphContainer::onZoomValueChanged);

    // Connect TimelineView interval changes (if timeline view exists)
    if (m_timelineView)
    {
        connect(m_timelineView, &TimelineView::TimeIntervalChanged,
                this, &GraphContainer::onTimeIntervalChanged);
    }

    // Connect TimeSelectionVisualizer clear button events
    if (m_timelineSelectionView)
    {
        connect(m_timelineSelectionView, &TimeSelectionVisualizer::timeSelectionsCleared,
                this, &GraphContainer::onClearTimeSelectionsButtonClicked);
        
        // Connect TimeSelectionVisualizer time selection made events
        connect(m_timelineSelectionView, &TimeSelectionVisualizer::timeSelectionMade,
                this, &GraphContainer::onTimeSelectionMade);
    }

    qDebug() << "GraphContainer: All event connections established";
}

void GraphContainer::setupEventConnectionsForWaterfallGraph()
{
    if (!m_currentWaterfallGraph)
    {
        qWarning() << "GraphContainer: Cannot setup event connections - no waterfall graph";
        return;
    }

    // Connect WaterfallGraph selection events
    connect(m_currentWaterfallGraph, &WaterfallGraph::SelectionCreated,
            this, &GraphContainer::onSelectionCreated);

    qDebug() << "GraphContainer: Event connections established for waterfall graph";
}

WaterfallGraph *GraphContainer::createWaterfallGraph(GraphType graphType)
{
    switch (graphType)
    {
    case GraphType::BDW:
        return new BDWGraph(this);
    case GraphType::BRW:
        return new BRWGraph(this);
    case GraphType::BTW:
        return new BTWGraph(this);
    case GraphType::FDW:
        return new FDWGraph(this);
    case GraphType::FTW:
        return new FTWGraph(this);
    case GraphType::LTW:
        return new LTWGraph(this);
    case GraphType::RTW:
        return new RTWGraph(this);
    default:
        qWarning() << "Unknown graph type, defaulting to BDWGraph";
        return new BDWGraph(this);
    }
}

void GraphContainer::createAllWaterfallGraphs()
{
    // Create all graph types upfront
    std::vector<GraphType> allTypes = getAllGraphTypes();
    
    for (GraphType graphType : allTypes)
    {
        if (m_waterfallGraphs.find(graphType) == m_waterfallGraphs.end())
        {
            WaterfallGraph *graph = createWaterfallGraph(graphType);
            m_waterfallGraphs[graphType] = graph;
            
            // Initially hide all graphs
            graph->setVisible(false);
            
            // Set up the graph properties
            setupWaterfallGraphProperties(graph, graphType);
            
            // Add to layout (only add one initially, the rest will be added as needed)
            if (graphType == currentDataOption)
            {
                m_leftLayout->addWidget(graph, 1);
                m_currentWaterfallGraph = graph;
            }
            else
            {
                // For other graphs, we'll add them to the layout but initially hidden
                graph->setParent(this);
                graph->hide();
            }
        }
    }
    
    // Connect events for all graphs
    for (auto &pair : m_waterfallGraphs)
    {
        connect(pair.second, &WaterfallGraph::SelectionCreated,
                this, &GraphContainer::onSelectionCreated);
    }
    
    qDebug() << "GraphContainer: Created all waterfall graph instances";
}

void GraphContainer::setupWaterfallGraphProperties(WaterfallGraph *graph, GraphType graphType)
{
    if (!graph)
    {
        qWarning() << "GraphContainer: Cannot setup properties - graph is null";
        return;
    }
    
    // Set up the data source
    WaterfallData *dataSource = nullptr;
    auto it = dataOptions.find(graphType);
    if (it != dataOptions.end())
    {
        dataSource = it->second;
    }
    else
    {
        dataSource = &waterfallData;
    }
    
    if (dataSource)
    {
        graph->setDataSource(*dataSource);
    }
    
    // Set the auto update Y range for the waterfall graph if it has stored range limits
    if (hasGraphRangeLimits(graphType))
    {
        graph->setAutoUpdateYRange(false);
        graph->setCustomYRange(getGraphRangeLimits(graphType).first, getGraphRangeLimits(graphType).second);
    }
    else
    {
        graph->setAutoUpdateYRange(true);
    }
    
    // Enable mouse selection for the waterfall graph
    graph->setMouseSelectionEnabled(false);
    
    // Enable range limiting for the waterfall graph
    graph->setRangeLimitingEnabled(true);
    
    // Set the waterfall graph size policy to expand
    graph->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Set minimum size but allow expansion
    graph->setMinimumSize(m_graphViewSize);
    graph->updateGeometry();
    
    // Set up series colors from the color map
    for (const auto& colorPair : m_seriesColorsMap) {
        graph->setSeriesColor(colorPair.first, colorPair.second);
    }
}

void GraphContainer::initializeWaterfallGraph(GraphType graphType)
{
    // Remove current graph from layout if it exists
    if (m_currentWaterfallGraph)
    {
        m_leftLayout->removeWidget(m_currentWaterfallGraph);
        m_currentWaterfallGraph->hide();
        m_currentWaterfallGraph->setParent(this);
    }
    
    // Find and show the target graph
    auto it = m_waterfallGraphs.find(graphType);
    if (it != m_waterfallGraphs.end())
    {
        WaterfallGraph *targetGraph = it->second;
        
        // Update data source if needed
        WaterfallData *dataSource = nullptr;
        auto dataIt = dataOptions.find(graphType);
        if (dataIt != dataOptions.end())
        {
            dataSource = dataIt->second;
        }
        else
        {
            dataSource = &waterfallData;
        }
        
        if (dataSource)
        {
            targetGraph->setDataSource(*dataSource);
        }
        
        // Set the auto update Y range for the waterfall graph if it has stored range limits
        if (hasGraphRangeLimits(graphType))
        {
            targetGraph->setAutoUpdateYRange(false);
            auto rangeLimits = getGraphRangeLimits(graphType);
            targetGraph->setCustomYRange(rangeLimits.first, rangeLimits.second);
        }
        else
        {
            targetGraph->setAutoUpdateYRange(true);
        }
        
        // Set current waterfall graph reference
        m_currentWaterfallGraph = targetGraph;
        
        // Add the target graph to layout and show it
        m_leftLayout->addWidget(targetGraph, 1);
        targetGraph->setVisible(true);
        
        qDebug() << "GraphContainer: Switched to waterfall graph type:" << graphTypeToString(graphType);
    }
    else
    {
        qWarning() << "GraphContainer: Cannot initialize waterfall graph - graph type not found:" << graphTypeToString(graphType);
    }
}

void GraphContainer::subscribeToIntervalChange(QObject *subscriber, const char *slot)
{
    if (subscriber && slot)
    {
        // Use the old Qt syntax for connecting to string-based slots
        connect(this, SIGNAL(IntervalChanged(TimeInterval)), subscriber, slot);
        qDebug() << "GraphContainer: External subscriber connected to interval change signal";
    }
    else
    {
        qWarning() << "GraphContainer: Invalid subscriber or slot provided to subscribeToIntervalChange";
    }
}

void GraphContainer::onTimeIntervalChanged(TimeInterval interval)
{
    qDebug() << "GraphContainer: Handling emitted signal - Time interval changed to" << timeIntervalToString(interval);

    // Update the time interval for the waterfall graph and time selection visualizer
    updateTimeInterval(interval);

    // Emit the signal to notify other components
    emit IntervalChanged(interval);
}

void GraphContainer::updateTimeInterval(TimeInterval interval)
{
    qDebug() << "GraphContainer: Updating time interval to" << timeIntervalToString(interval);

    // Update the waterfall graph time interval
    if (m_currentWaterfallGraph)
    {
        m_currentWaterfallGraph->setTimeInterval(interval);
    }

    // Update the time selection visualizer time interval
    if (m_timelineSelectionView)
    {
        m_timelineSelectionView->setTimeLineLength(interval);
        qDebug() << "TimeSelectionVisualizer updated with interval:" << timeIntervalToString(interval);
    }

    // Update the timeline view time interval
    if (m_timelineView)
    {
        m_timelineView->setTimeLineLength(interval);
        qDebug() << "TimelineView updated with interval:" << timeIntervalToString(interval);
    }
}

void GraphContainer::onSelectionCreated(const TimeSelectionSpan &selection)
{
    qDebug() << "GraphContainer: Selection created from" << selection.startTime.toString() << "to" << selection.endTime.toString();

    if (m_timelineSelectionView)
    {
        m_timelineSelectionView->addTimeSelection(selection);
        qDebug() << "GraphContainer: Selection added to timeline selection view";
    }
    else
    {
        qWarning() << "GraphContainer: Timeline selection view is null";
    }

    // Emit the new signal for external components
    emit TimeSelectionCreated(selection);
}

void GraphContainer::onTimeSelectionMade(const TimeSelectionSpan &selection)
{
    qDebug() << "GraphContainer: Time selection made from" << selection.startTime.toString() << "to" << selection.endTime.toString();

    // Emit the TimeSelectionCreated signal for external components
    emit TimeSelectionCreated(selection);
}

void GraphContainer::setMouseSelectionEnabled(bool enabled)
{
    if (m_currentWaterfallGraph)
    {
        m_currentWaterfallGraph->setMouseSelectionEnabled(enabled);
        qDebug() << "GraphContainer: Mouse selection" << (enabled ? "enabled" : "disabled");
    }
}

bool GraphContainer::isMouseSelectionEnabled() const
{
    if (m_currentWaterfallGraph)
    {
        return m_currentWaterfallGraph->isMouseSelectionEnabled();
    }
    return false;
}

void GraphContainer::testSelectionRectangle()
{
    if (m_currentWaterfallGraph)
    {
        m_currentWaterfallGraph->testSelectionRectangle();
        qDebug() << "GraphContainer: Test selection rectangle called";
    }
}

void GraphContainer::setCurrentTime(const QTime &time)
{
    qDebug() << "GraphContainer: Setting current time to" << time.toString();
    if (m_timelineSelectionView)
    {
        m_timelineSelectionView->setCurrentTime(time);
    }

    if (m_timelineView)
    {
        m_timelineView->setCurrentTime(time);
    }
}

void GraphContainer::addTimeSelection(const TimeSelectionSpan &selection)
{
    qDebug() << "GraphContainer: Adding time selection from" << selection.startTime.toString() << "to" << selection.endTime.toString();

    if (m_timelineSelectionView)
    {
        m_timelineSelectionView->addTimeSelection(selection);
        qDebug() << "GraphContainer: Time selection added to timeline selection view";
    }
    else
    {
        qWarning() << "GraphContainer: Timeline selection view is null - cannot add selection";
    }
}

void GraphContainer::clearTimeSelections()
{
    qDebug() << "GraphContainer: Clearing all time selections";

    if (m_timelineSelectionView)
    {
        m_timelineSelectionView->clearTimeSelections();
        qDebug() << "GraphContainer: All time selections cleared from timeline selection view";

        // Emit signal to notify external components
        emit TimeSelectionsCleared();
    }
    else
    {
        qWarning() << "GraphContainer: Timeline selection view is null - cannot clear selections";
    }
}

void GraphContainer::clearTimeSelectionsSilent()
{
    qDebug() << "GraphContainer: Silently clearing all time selections (no signal emission)";

    if (m_timelineSelectionView)
    {
        m_timelineSelectionView->clearTimeSelections();
        qDebug() << "GraphContainer: All time selections cleared from timeline selection view (silent)";

        // Do NOT emit signal to prevent cyclic dependencies
    }
    else
    {
        qWarning() << "GraphContainer: Timeline selection view is null - cannot clear selections";
    }
}

void GraphContainer::initializeZoomPanelLimits()
{
    if (!m_zoomPanel)
    {
        qDebug() << "GraphContainer: Cannot initialize zoom panel limits - no zoom panel";
        return;
    }

    // Get the current data source (either selected option or default waterfallData)
    WaterfallData *currentDataSource = nullptr;
    auto it = dataOptions.find(currentDataOption);
    if (it != dataOptions.end())
    {
        currentDataSource = it->second;
    }

    if (!currentDataSource || currentDataSource->isEmpty())
    {
        qDebug() << "GraphContainer: Cannot initialize zoom panel limits - no data available";
        return;
    }

    // Get the Y range from the current data source
    auto yRange = currentDataSource->getCombinedYRange();
    qreal dataMin = yRange.first;
    qreal dataMax = yRange.second;

    // Check if theres a stored range limit for the current data option
    if (hasGraphRangeLimits(currentDataOption))
    {
        // Use the stored range limit
        auto rangeLimit = getGraphRangeLimits(currentDataOption);
        dataMin = rangeLimit.first;
        dataMax = rangeLimit.second;

        qDebug() << "GraphContainer: Using stored range limit for" << graphTypeToString(currentDataOption) << "- Min:" << dataMin << "Max:" << dataMax;
    }


    // Calculate center value (linear interpolation)
    qreal centerValue = dataMin + (dataMax - dataMin) * 0.5;

    // Set the zoom panel label values
    m_zoomPanel->setLeftLabelValue(dataMin);
    m_zoomPanel->setCenterLabelValue(centerValue);
    m_zoomPanel->setRightLabelValue(dataMax);

    qDebug() << "GraphContainer: Zoom panel limits initialized - Min:" << dataMin
             << "Center:" << centerValue << "Max:" << dataMax << "- Zoom reset to 50%";
}

void GraphContainer::onZoomValueChanged(ZoomBounds bounds)
{
    if (!m_currentWaterfallGraph)
    {
        qDebug() << "GraphContainer: Cannot update waterfall graph - no waterfall graph";
        return;
    }

    qDebug() << "GraphContainer: Received interpolated zoom bounds - Lower:" << bounds.lowerbound
        << "Upper:" << bounds.upperbound;

    // The bounds are already interpolated by the zoom panel, so use them directly
    // No additional conversion needed - bounds.lowerbound and bounds.upperbound
    // are already in the correct data value range

    // Set the custom Y range directly from interpolated bounds
    m_currentWaterfallGraph->setCustomYRange(bounds.lowerbound, bounds.upperbound);

    // Update the time range to ensure only relevant data points are rendered
    m_currentWaterfallGraph->updateTimeRange();

    qDebug() << "GraphContainer: Custom Y range set directly from interpolated bounds and time range updated";
}

void GraphContainer::onClearTimeSelectionsButtonClicked()
{
    qDebug() << "GraphContainer: Clear time selections button clicked";
    clearTimeSelections();
}

// Chevron label control methods implementation
void GraphContainer::setChevronLabel1(const QString &label)
{
    if (m_timelineView)
    {
        m_timelineView->setChevronLabel1(label);
        qDebug() << "GraphContainer: Set chevron label 1 to:" << label;
    }
    else
    {
        qWarning() << "GraphContainer: Cannot set chevron label - timeline view is null";
    }
}

void GraphContainer::setChevronLabel2(const QString &label)
{
    if (m_timelineView)
    {
        m_timelineView->setChevronLabel2(label);
        qDebug() << "GraphContainer: Set chevron label 2 to:" << label;
    }
    else
    {
        qWarning() << "GraphContainer: Cannot set chevron label - timeline view is null";
    }
}

void GraphContainer::setChevronLabel3(const QString &label)
{
    if (m_timelineView)
    {
        m_timelineView->setChevronLabel3(label);
        qDebug() << "GraphContainer: Set chevron label 3 to:" << label;
    }
    else
    {
        qWarning() << "GraphContainer: Cannot set chevron label - timeline view is null";
    }
}

QString GraphContainer::getChevronLabel1() const
{
    if (m_timelineView)
    {
        return m_timelineView->getChevronLabel1();
    }
    else
    {
        qWarning() << "GraphContainer: Cannot get chevron label - timeline view is null";
        return QString();
    }
}

QString GraphContainer::getChevronLabel2() const
{
    if (m_timelineView)
    {
        return m_timelineView->getChevronLabel2();
    }
    else
    {
        qWarning() << "GraphContainer: Cannot get chevron label - timeline view is null";
        return QString();
    }
}

QString GraphContainer::getChevronLabel3() const
{
    if (m_timelineView)
    {
        return m_timelineView->getChevronLabel3();
    }
    else
    {
        qWarning() << "GraphContainer: Cannot get chevron label - timeline view is null";
        return QString();
    }
}

// Range limits management methods implementation
void GraphContainer::setGraphRangeLimits(const GraphType graphType, qreal yMin, qreal yMax)
{
    graphRangeLimits[graphType] = std::make_pair(yMin, yMax);

    // Check if the current graph type has stored range limits
    if (graphType == currentDataOption)
    {
        // Disable auto-update Y range and set the stored limits
        m_currentWaterfallGraph->setAutoUpdateYRange(false);
        m_currentWaterfallGraph->setCustomYRange(yMin, yMax);

        // Update the zoom panel limits
        m_zoomPanel->setLeftLabelValue(yMin);
        m_zoomPanel->setCenterLabelValue(yMin + (yMax - yMin) * 0.5);
        m_zoomPanel->setRightLabelValue(yMax);

        qDebug() << "GraphContainer: Applied stored range limits for" << graphTypeToString(graphType)
                 << "- Min:" << yMin << "Max:" << yMax << "- Auto-update disabled";
    }
}

void GraphContainer::removeGraphRangeLimits(const GraphType graphType)
{
    graphRangeLimits.erase(graphType);

    // Check if the current graph type has stored range limits
    if (graphType == currentDataOption)
    {
        // Enable auto-update Y range for graphs without stored limits
        m_currentWaterfallGraph->setAutoUpdateYRange(true);
        qDebug() << "GraphContainer: No stored range limits for" << graphTypeToString(graphType)
                 << "- Auto-update enabled";
    }
}

void GraphContainer::clearAllGraphRangeLimits()
{
    graphRangeLimits.clear();

    m_currentWaterfallGraph->setAutoUpdateYRange(true);
    qDebug() << "GraphContainer: Cleared all range limits - Auto-update enabled";
}

bool GraphContainer::hasGraphRangeLimits(const GraphType graphType) const
{
    return graphRangeLimits.find(graphType) != graphRangeLimits.end();
}

std::pair<qreal, qreal> GraphContainer::getGraphRangeLimits(const GraphType graphType) const
{
    auto it = graphRangeLimits.find(graphType);
    return (it != graphRangeLimits.end()) ? it->second : std::make_pair(0.0, 0.0);
}