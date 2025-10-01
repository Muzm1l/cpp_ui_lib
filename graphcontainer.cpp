#include "graphcontainer.h"
#include <QDebug>

GraphContainer::GraphContainer(QWidget *parent, bool showTimelineView, QTimer *timer)
    : QWidget{parent}, m_showTimelineView(showTimelineView), m_timer(timer), m_ownsTimer(false), m_timelineWidth(150), m_graphViewSize(80, 300), currentDataOption(GraphType::BDW)
{

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
    m_zoomPanel->setMaximumHeight(50); // Limit zoom panel height

    // Initialize the waterfall graph
    initializeWaterfallGraph(currentDataOption);

    // Add ComboBox, ZoomPanel, and WaterfallGraph to left layout
    m_leftLayout->addWidget(m_comboBox);
    m_leftLayout->addWidget(m_zoomPanel);

    // Add left layout to main layout with stretch factor
    m_mainLayout->addLayout(m_leftLayout, 1); // Give stretch factor of 1 to left layout

    // Create TimelineSelectionView with timer
    m_timelineSelectionView = new TimeSelectionVisualizer(this, m_timer);
    m_mainLayout->addWidget(m_timelineSelectionView);
    m_timelineSelectionView->setCurrentTime(QTime::currentTime());
    m_timelineSelectionView->setTimeLineLength(TimeInterval::FifteenMinutes);

    // Create TimelineView (conditionally based on showTimelineView) with timer
    if (m_showTimelineView)
    {
        m_timelineView = new TimelineView(this, m_timer);
        m_mainLayout->addWidget(m_timelineView);
    }
    else
    {
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

    qDebug() << "GraphContainer: Timer tick - updated current time to" << currentTime.toString();
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
    m_showTimelineView = showTimelineView;
    if (m_timelineView)
    {
        m_timelineView->setVisible(showTimelineView);
    }
    else
    {
        m_timelineView = new TimelineView(this, m_timer);
        m_mainLayout->addWidget(m_timelineView);

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
    if (m_waterfallGraph)
    {
        m_waterfallGraph->setMinimumSize(m_graphViewSize);
        m_waterfallGraph->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
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
    if (m_showTimelineView)
    {
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

WaterfallData GraphContainer::getData() const
{
    return waterfallData;
}

std::vector<std::pair<qreal, QDateTime>> GraphContainer::getDataWithinYExtents(qreal yMin, qreal yMax) const
{
    return waterfallData.getDataWithinYExtents(yMin, yMax);
}

std::vector<std::pair<qreal, QDateTime>> GraphContainer::getDataWithinTimeRange(const QDateTime &startTime, const QDateTime &endTime) const
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

void GraphContainer::redrawWaterfallGraph()
{
    if (m_waterfallGraph)
    {
        m_waterfallGraph->draw();
        qDebug() << "GraphContainer: Triggered waterfall graph redraw";
    }
}

// Data options management implementation

void GraphContainer::addDataOption(const GraphType graphType, WaterfallData &dataSource)
{
    QString title = graphTypeToString(graphType);
    dataOptions[graphType] = &dataSource;
    updateComboBoxOptions();

    // If this is the first option, set it as current
    setCurrentDataOption(graphType);

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
                // Initialize waterfall graph with the default type
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

    // Initialize waterfall graph with the default type
    initializeWaterfallGraph(currentDataOption);

    // Initialize zoom panel limits for the default data source
    initializeZoomPanelLimits();

    qDebug() << "Cleared all data options";
}

void GraphContainer::setCurrentDataOption(const GraphType graphType)
{
    QString title = graphTypeToString(graphType);
    auto it = dataOptions.find(graphType);
    if (it != dataOptions.end())
    {
        currentDataOption = graphType;

        // Initialize waterfall graph with the new type
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

    // Connect WaterfallGraph selection events
    connect(m_waterfallGraph, &WaterfallGraph::SelectionCreated,
            this, &GraphContainer::onSelectionCreated);

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
    }

    qDebug() << "GraphContainer: All event connections established";
}

void GraphContainer::setupEventConnectionsForWaterfallGraph()
{
    if (!m_waterfallGraph)
    {
        qWarning() << "GraphContainer: Cannot setup event connections - no waterfall graph";
        return;
    }

    // Connect WaterfallGraph selection events
    connect(m_waterfallGraph, &WaterfallGraph::SelectionCreated,
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

void GraphContainer::initializeWaterfallGraph(GraphType graphType)
{
    // Clean up existing waterfall graph if it exists
    if (m_waterfallGraph)
    {
        m_leftLayout->removeWidget(m_waterfallGraph);
        disconnect(m_waterfallGraph, nullptr, this, nullptr); // Disconnect all connections
        delete m_waterfallGraph;
        m_waterfallGraph = nullptr;
    }

    // Create new waterfall graph instance based on graph type
    m_waterfallGraph = createWaterfallGraph(graphType);

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
        m_waterfallGraph->setDataSource(*dataSource);
    }

    // Set the auto update Y range for the waterfall graph if it has stored range limits
    if (hasGraphRangeLimits(graphType))
    {
        m_waterfallGraph->setAutoUpdateYRange(false);
    }
    else
    {
        m_waterfallGraph->setAutoUpdateYRange(true);
    }

    // Enable mouse selection for the waterfall graph
    m_waterfallGraph->setMouseSelectionEnabled(true);

    // Enable range limiting for the waterfall graph
    m_waterfallGraph->setRangeLimitingEnabled(true);

    // Set the waterfall graph size policy to expand
    m_waterfallGraph->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Set minimum size but allow expansion
    m_waterfallGraph->setMinimumSize(m_graphViewSize);
    m_waterfallGraph->updateGeometry();

    // Add the waterfall graph to the layout with stretch factor
    m_leftLayout->addWidget(m_waterfallGraph, 1);

    // Setup event connections for the new graph
    setupEventConnectionsForWaterfallGraph();
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
    if (m_waterfallGraph)
    {
        m_waterfallGraph->setTimeInterval(interval);
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

void GraphContainer::setMouseSelectionEnabled(bool enabled)
{
    if (m_waterfallGraph)
    {
        m_waterfallGraph->setMouseSelectionEnabled(enabled);
        qDebug() << "GraphContainer: Mouse selection" << (enabled ? "enabled" : "disabled");
    }
}

bool GraphContainer::isMouseSelectionEnabled() const
{
    if (m_waterfallGraph)
    {
        return m_waterfallGraph->isMouseSelectionEnabled();
    }
    return false;
}

void GraphContainer::testSelectionRectangle()
{
    if (m_waterfallGraph)
    {
        m_waterfallGraph->testSelectionRectangle();
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
    if (!m_waterfallGraph)
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
    m_waterfallGraph->setCustomYRange(bounds.lowerbound, bounds.upperbound);

    // Update the time range to ensure only relevant data points are rendered
    m_waterfallGraph->updateTimeRange();

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
        m_waterfallGraph->setAutoUpdateYRange(false);
        m_waterfallGraph->setCustomYRange(yMin, yMax);

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
        m_waterfallGraph->setAutoUpdateYRange(true);
        qDebug() << "GraphContainer: No stored range limits for" << graphTypeToString(graphType)
                 << "- Auto-update enabled";
    }
}

void GraphContainer::clearAllGraphRangeLimits()
{
    graphRangeLimits.clear();

    m_waterfallGraph->setAutoUpdateYRange(true);
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