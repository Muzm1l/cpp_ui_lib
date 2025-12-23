#include "graphcontainer.h"
#include "btwinteractiveoverlay.h"
#include "debugutils.h"
#include <QDebug>
#include <QTimer>
#include <stdexcept>

GraphContainer::GraphContainer(QWidget *parent, bool showTimelineView, std::map<QString, QColor> seriesColorsMap, QTimer *timer, int containerWidth, int containerHeight, GraphContainerSyncState *syncState)
    : QWidget{parent}, 
    m_showTimelineView(showTimelineView), 
    m_timer(timer), 
    m_ownsTimer(false), 
    m_timelineWidth(64), 
    m_graphViewSize(226, 300), 
    m_seriesColorsMap(seriesColorsMap), 
    currentDataOption(GraphType::BDW), 
    m_updatingTimeInterval(false),
    m_sharedCursorTime(QDateTime()),
    m_hasSharedCursorTime(false),
    m_isInFollowMode(true),
    m_syncState(syncState),
    m_hasLastSyncedTimeScope(false)
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

        DEBUG_OUT() << "GraphContainer: Timer setup completed since none was provided - interval:" << m_timer->interval() << "ms";
    }

    // Create main horizontal layout with 1px spacing and no margins
    m_mainLayout = new QHBoxLayout(this);
    m_mainLayout->setSpacing(1);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);

    // Create left vertical layout with no margins
    m_waterfallLayout = new QVBoxLayout();
    m_waterfallLayout->setContentsMargins(0, 0, 0, 0);

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
    m_waterfallLayout->addWidget(m_comboBox);
    m_waterfallLayout->addWidget(m_zoomPanel);

    // Create all waterfall graph instances upfront
    createAllWaterfallGraphs();
    
    // Show the initial graph type
    setCurrentDataOption(currentDataOption);


    // Create TimelineSelectionView with timer
    // Calculate combined height of combobox and zoompanel for clear button height
    int zoompanelHeight = m_zoomPanel->maximumHeight();
    int clearButtonHeight = comboboxHeight + zoompanelHeight;
    
    m_timelineSelectionView = new TimeSelectionVisualizer(this, m_timer, clearButtonHeight);
    // Set size policy to expand vertically
    m_timelineSelectionView->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_mainLayout->addWidget(m_timelineSelectionView);
    m_timelineSelectionView->setCurrentTime(QTime::currentTime());
    m_timelineSelectionView->setTimeLineLength(TimeInterval::FifteenMinutes);
    m_timelineSelectionView->setVisible(m_showTimelineView);

    // Create TimelineView (conditionally based on showTimelineView) with timer and sync state
    if (m_showTimelineView)
    {
        DEBUG_OUT() << "GraphContainer constructor: Creating TimelineView with showTimelineView = true";
        m_timelineView = new TimelineView(this, m_timer, m_syncState);
        // Set size policy to expand vertically
        m_timelineView->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        m_mainLayout->addWidget(m_timelineView);
        DEBUG_OUT() << "GraphContainer constructor: TimelineView created and added to layout";
    }
    else
    {
        DEBUG_OUT() << "GraphContainer constructor: Not creating TimelineView, showTimelineView = false";
        m_timelineView = nullptr;
    }

    // Add waterfall layout to main layout last (rightmost) with stretch factor
    m_mainLayout->addLayout(m_waterfallLayout, 1); // Give stretch factor of 1 to waterfall layout

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

    // Connect timer to our tick handler with UniqueConnection to prevent duplicates
    // This ensures the animation continues even after timeline view customization
    connect(m_timer, &QTimer::timeout, this, &GraphContainer::onTimerTick, Qt::UniqueConnection);

    // Ensure timer is started (in case it was stopped during customization)
    if (!m_timer->isActive())
    {
        m_timer->start();
    }

    DEBUG_OUT() << "GraphContainer: Timer setup completed - interval:" << m_timer->interval() << "ms";
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
        // Update mode from sync state if available and mode has changed
        if (m_syncState)
        {
            TimelineViewMode newMode = m_syncState->isGraphContainerInFollowMode 
                ? TimelineViewMode::FOLLOW_MODE 
                : TimelineViewMode::FROZEN_MODE;
            
            // Only update mode if it has actually changed to avoid resetting slider unnecessarily
            if (m_timelineView->getTimelineViewMode() != newMode)
            {
                m_timelineView->setTimelineViewMode(newMode);
            }
            
            // Update crosshair timestamp from shared sync state
            // This ensures crosshair appears in timeline view even when cursor changes in another container
            if (m_syncState->hasCursorTime && m_syncState->cursorTime.isValid())
            {
                m_timelineView->updateCrosshairTimestampFromTime(m_syncState->cursorTime);
            }
            else
            {
                m_timelineView->clearCrosshairTimestamp();
            }
        }
        
        // TimelineView will decide whether to update slider based on its current mode
        m_timelineView->setCurrentTime(currentTime);
    }
//--------syed----------------
    // Update graph only when there's new data to show (within 1 minute of current time)
    // Performance optimization: Only redraw when necessary, not on every timer tick
    if (m_currentWaterfallGraph && m_timelineView)
    {
        auto timeRange = m_currentWaterfallGraph->getTimeRange();
        if (timeRange.first.isValid() && timeRange.second.isValid())
        {
            QDateTime currentDateTime = QDateTime::currentDateTime();
            qint64 timeDiffMs = timeRange.second.msecsTo(currentDateTime);
            
            // If showing recent data (within 1 minute), check for new data
            if (timeDiffMs >= 0 && timeDiffMs < 60000)
            {
                // Only redraw if there's actually new data to show
                if (m_currentWaterfallGraph->getDataSource())
                {
                    QDateTime latestDataTime = m_currentWaterfallGraph->getDataSource()->getLatestTime();
                    if (latestDataTime.isValid() && latestDataTime > timeRange.second)
                    {
                        // Update time range to include new data
                        qint64 intervalMs = timeRange.first.msecsTo(timeRange.second);
                        QDateTime newTimeMin = latestDataTime.addMSecs(-intervalMs);
                        QDateTime newTimeMax = latestDataTime;
                        
                        m_currentWaterfallGraph->setTimeRange(newTimeMin, newTimeMax);
                        
                        // Update timeline view to match
                        TimeSelectionSpan newWindow(newTimeMin, newTimeMax);
                        m_timelineView->setVisibleTimeWindow(newWindow);
                        
                        // Redraw graph to show updated data
                        redrawWaterfallGraph();
                    }
                    // NOTE: Removed unconditional redraw - only redraw when new data arrives
                    // This prevents unnecessary CPU usage from constant full redraws
                }
            }
        }
    } //------------syed-----------------------------------

    // DEBUG_OUT() << "GraphContainer: Timer tick - updated current time to" << currentTime.toString();
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
    DEBUG_OUT() << "GraphContainer::setShowTimelineView called with:" << showTimelineView;
    m_showTimelineView = showTimelineView;
    if (m_timelineView)
    {
        DEBUG_OUT() << "GraphContainer: Setting existing TimelineView visibility to:" << showTimelineView;
        m_timelineView->setVisible(showTimelineView);
        DEBUG_OUT() << "GraphContainer: TimelineView visibility after setting:" << m_timelineView->isVisible();
        DEBUG_OUT() << "GraphContainer: TimelineView size:" << m_timelineView->size();
    }
    else
    {
        DEBUG_OUT() << "GraphContainer: Creating new TimelineView with visibility:" << showTimelineView;
        m_timelineView = new TimelineView(this, m_timer, m_syncState);
        // Set size policy to expand vertically
        m_timelineView->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        // Insert at position 0 (leftmost) to match the reversed layout order
        m_mainLayout->insertWidget(0, m_timelineView);
        m_timelineView->setVisible(showTimelineView);
        DEBUG_OUT() << "GraphContainer: New TimelineView visibility after setting:" << m_timelineView->isVisible();
        DEBUG_OUT() << "GraphContainer: New TimelineView size:" << m_timelineView->size();

        // Re-establish event connections to include the new TimelineView
        setupEventConnections();
    }

    if (m_timelineSelectionView)
    {
        m_timelineSelectionView->setVisible(showTimelineView);
        DEBUG_OUT() << "GraphContainer: TimelineSelectionView visibility set to:" << showTimelineView;
    }

    // Update container size when timeline view visibility changes
    updateTotalContainerSize();
}

bool GraphContainer::getShowTimelineView()
{
    return m_showTimelineView;
}

TimelineView *GraphContainer::getTimelineView() const
{
    return m_timelineView;
}

void GraphContainer::setShowTimeSelectionVisualizer(bool show)
{
    if (m_timelineSelectionView)
    {
        m_timelineSelectionView->setVisible(show);
    }
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

    // Update all waterfall graphs to have the same size policy
    for (auto &pair : m_waterfallGraphs)
    {
        if (pair.second)
        {
            pair.second->setMinimumSize(m_graphViewSize);
            pair.second->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            pair.second->updateGeometry();
        }
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
    totalWidth += 32; // Timeline selection view width

    // Add timeline view width if enabled
    if (m_showTimelineView)
    {
        totalWidth += m_timelineWidth;
    }

    // Add spacing between components (1px each)
    totalWidth += 2; // 2 spacings: between graph and timeline selection, and between timeline selection and timeline view

    // For vertical stretching, we only set a minimum height, not a fixed height
    // The actual height will be determined by the parent layout
    return QSize(totalWidth, totalHeight);
}

int GraphContainer::getComboBoxAndZoomPanelHeight() const
{
    if (!m_comboBox || !m_zoomPanel)
    {
        return 0;
    }
    
    // Get combo box height
    int comboboxHeight = m_comboBox->sizeHint().height();
    
    // Zoom panel height matches combo box height
    int zoompanelHeight = m_zoomPanel->maximumHeight();
    
    // Return combined height (they are stacked vertically, so add them)
    // Also account for any spacing between them in the layout
    // The layout spacing is 0, so just add the heights
    return comboboxHeight + zoompanelHeight;
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
    // Only set minimum size to allow vertical expansion
    setMinimumSize(totalSize);
    // Remove maximum size constraint to allow stretching
    setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
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
        // Force full redraw which should handle cache invalidation
        m_currentWaterfallGraph->forceFullRedraw();
    }
}

void GraphContainer::redrawWaterfallGraph(GraphType graphType)
{
    // Redraw a specific graph type, even if it's not currently displayed
    auto it = m_waterfallGraphs.find(graphType);
    if (it != m_waterfallGraphs.end() && it->second)
    {
        // Force full redraw which should handle cache invalidation
        it->second->forceFullRedraw();
    }
}

WaterfallGraph* GraphContainer::getCurrentWaterfallGraph() const
{
    return m_currentWaterfallGraph;
}

WaterfallGraph* GraphContainer::getWaterfallGraph(GraphType graphType) const
{
    auto it = m_waterfallGraphs.find(graphType);
    if (it != m_waterfallGraphs.end())
    {
        return it->second;
    }
    return nullptr;
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

    DEBUG_OUT() << "Added data option:" << title;
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

        DEBUG_OUT() << "Removed data option:" << title;
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

    DEBUG_OUT() << "Cleared all data options";
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

    // Reset zoom panel state when graph changes - clear any previous customization
    // This ensures the new graph's range is displayed, not the previous graph's zoom state
    if (m_zoomPanel)
    {
        m_zoomPanel->resetUserModifiedFlag();
        m_zoomPanel->resetIndicatorToFullRange();
        DEBUG_OUT() << "GraphContainer: Reset zoom panel state for new graph:" << title;
    }

    // Initialize zoom panel limits for the new data source
    initializeZoomPanelLimits();

    DEBUG_OUT() << "Set current data option to:" << title;
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
    
    // Define the desired order of graph types in the combo box
    std::vector<GraphType> desiredOrder = {
        GraphType::BTW,  // 1. BTW
        GraphType::BDW,  // 2. BDW
        GraphType::BRW,  // 3. BRW
        GraphType::RTW,  // 4. RTW
        GraphType::FTW,  // 5. FTW
        GraphType::FDW,  // 6. FDW
        GraphType::LTW   // 7. LTW
    };
    
    // Add items in the desired order, only if they exist in dataOptions
    for (GraphType graphType : desiredOrder)
    {
        if (dataOptions.find(graphType) != dataOptions.end())
        {
            m_comboBox->addItem(graphTypeToString(graphType));
        }
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
        
        // Connect marker timestamp and value changes
        connect(pair.second, &WaterfallGraph::markerTimestampValueChanged,
                this, &GraphContainer::markerTimestampValueChanged);
        
        // Connect crosshair position changes to update zoompanel label
        pair.second->setCrosshairPositionChangedCallback([this](qreal xPosition) {
            if (m_zoomPanel)
            {
                if (xPosition < 0)
                {
                    // Crosshair hidden
                    m_zoomPanel->clearCrosshairLabel();
                }
                else
                {
                    // Update zoompanel label with crosshair X position
                    m_zoomPanel->updateCrosshairLabel(xPosition);
                }
            }
        });
    }

    // Connect ZoomPanel value changes
    connect(m_zoomPanel, &ZoomPanel::valueChanged,
            this, &GraphContainer::onZoomValueChanged);

    // Connect TimelineView interval changes (if timeline view exists)
    if (m_timelineView)
    {
        connect(m_timelineView, &TimelineView::TimeIntervalChanged,
                this, &GraphContainer::onTimeIntervalChanged);
        
        connect(m_timelineView, &TimelineView::TimeScopeChanged,
                this, &GraphContainer::onTimeScopeChanged);
        
        connect(m_timelineView, &TimelineView::GraphContainerInFollowModeChanged,
                this, &GraphContainer::onGraphContainerInFollowModeChanged);
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

    DEBUG_OUT() << "GraphContainer: All event connections established";
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
    
    // Connect marker timestamp and value changes
    connect(m_currentWaterfallGraph, &WaterfallGraph::markerTimestampValueChanged,
            this, &GraphContainer::markerTimestampValueChanged);

    DEBUG_OUT() << "GraphContainer: Event connections established for waterfall graph";
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
                m_waterfallLayout->addWidget(graph, 1);
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
        
        // Connect marker timestamp and value changes
        connect(pair.second, &WaterfallGraph::markerTimestampValueChanged,
                this, &GraphContainer::markerTimestampValueChanged);
    }
    
    DEBUG_OUT() << "GraphContainer: Created all waterfall graph instances";
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

    graph->setCursorTimeChangedCallback([this](const QDateTime &time, qreal /* yPosition */) {
        handleCursorTimeChanged(time);
        
        // Update timelineview with crosshair timestamp
        // Use timestamp-based calculation instead of Y coordinate to ensure consistency
        // This prevents the cursor timestamp from showing at wrong position when layout becomes unstable
        if (m_timelineView && time.isValid())
        {
            m_timelineView->updateCrosshairTimestampFromTime(time);
        }
        else if (m_timelineView)
        {
            m_timelineView->clearCrosshairTimestamp();
        }
    });
    
    // Set shared sync state for cursor layer
    if (m_syncState)
    {
        graph->setCursorSyncState(m_syncState);
    }
    
    applyCursorTimeToGraph(graph);
    
    // Set up crosshair position callback to update zoompanel label
    graph->setCrosshairPositionChangedCallback([this](qreal xPosition) {
        if (m_zoomPanel)
        {
            if (xPosition < 0)
            {
                // Crosshair hidden
                m_zoomPanel->clearCrosshairLabel();
            }
            else
            {
                // Update zoompanel label with crosshair X position
                m_zoomPanel->updateCrosshairLabel(xPosition);
            }
        }
    });

    // Connect DeleteInteractiveMarkers signal to BTWGraph
    if (auto btwGraph = qobject_cast<BTWGraph*>(graph)) {
        connect(this, &GraphContainer::DeleteInteractiveMarkers,
                btwGraph, &BTWGraph::deleteInteractiveMarkers);
        DEBUG_OUT() << "GraphContainer: Connected DeleteInteractiveMarkers signal to BTWGraph";
        
        // Connect BTW marker signals
        connect(btwGraph, &BTWGraph::manualMarkerPlaced,
                this, &GraphContainer::onBTWManualMarkerPlaced);
        connect(btwGraph, &BTWGraph::manualMarkerClicked,
                this, &GraphContainer::onBTWManualMarkerClicked);
        
        // Connect BTW horizontal line signals
        connect(btwGraph, &BTWGraph::horizontalLinePlaced,
                this, &GraphContainer::onBTWHorizontalLinePlaced);
        connect(btwGraph, &BTWGraph::horizontalLineRemoved,
                this, &GraphContainer::onBTWHorizontalLineRemoved);
        
        // Connect comprehensive marker click signal (forwards timestamp, range, and bearing rate)
        connect(btwGraph, &BTWGraph::markerClickedWithData,
                this, &GraphContainer::markerClickedWithData);
        
        // Connect BTW marker sync signals (for syncing across containers)
        connect(btwGraph, &BTWGraph::markerSyncDataChanged,
                this, &GraphContainer::BTWMarkerSyncDataChanged);
        connect(btwGraph, &BTWGraph::markerSyncDeleted,
                this, &GraphContainer::BTWMarkerSyncDeleted);
        
        // Connect BTW shaded region sync signals
        connect(btwGraph, &BTWGraph::shadedRegionAdded,
                this, &GraphContainer::ShadedRegionSyncAdded);
        connect(btwGraph, &BTWGraph::shadedRegionRemoved,
                this, &GraphContainer::ShadedRegionSyncRemoved);
        connect(btwGraph, &BTWGraph::shadedRegionsCleared,
                this, &GraphContainer::ShadedRegionsSyncCleared);
        
        DEBUG_OUT() << "GraphContainer: Connected BTW marker and shaded region sync signals";
    }
    
    // Connect RTW R marker signal
    if (auto rtwGraph = qobject_cast<RTWGraph*>(graph)) {
        connect(rtwGraph, &RTWGraph::rMarkerTimestampCaptured,
                this, &GraphContainer::onRTWRMarkerTimestampCaptured);
        DEBUG_OUT() << "GraphContainer: Connected RTW R marker timestamp signal";
        
        // Connect RTW symbol signal
        connect(rtwGraph, &RTWGraph::rtwSymbolTimestampCaptured,
                this, &GraphContainer::onRTWSymbolTimestampCaptured);
        DEBUG_OUT() << "GraphContainer: Connected RTW symbol timestamp signal";
    }
}

void GraphContainer::initializeWaterfallGraph(GraphType graphType)
{
    // Remove current graph from layout if it exists
    if (m_currentWaterfallGraph)
    {
        m_waterfallLayout->removeWidget(m_currentWaterfallGraph);
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
        m_waterfallLayout->addWidget(targetGraph, 1);
        targetGraph->setVisible(true);
        applyCursorTimeToGraph(targetGraph);
        
        DEBUG_OUT() << "GraphContainer: Switched to waterfall graph type:" << graphTypeToString(graphType);
    }
    else
    {
        qWarning() << "GraphContainer: Cannot initialize waterfall graph - graph type not found:" << graphTypeToString(graphType);
    }
}

void GraphContainer::handleCursorTimeChanged(const QDateTime &time)
{
    m_sharedCursorTime = time;
    m_hasSharedCursorTime = time.isValid();

    for (auto &pair : m_waterfallGraphs)
    {
        applyCursorTimeToGraph(pair.second);
    }

    if (m_cursorTimeChangedCallback)
    {
        m_cursorTimeChangedCallback(this, time);
    }
}

void GraphContainer::applyCursorTimeToGraph(WaterfallGraph *graph)
{
    if (!graph)
    {
        return;
    }

    if (m_hasSharedCursorTime)
    {
        graph->setTimeAxisCursor(m_sharedCursorTime);
    }
    else
    {
        graph->clearTimeAxisCursor();
    }
}

void GraphContainer::subscribeToIntervalChange(QObject *subscriber, const char *slot)
{
    if (subscriber && slot)
    {
        // Use the old Qt syntax for connecting to string-based slots
        connect(this, SIGNAL(IntervalChanged(TimeInterval)), subscriber, slot);
        DEBUG_OUT() << "GraphContainer: External subscriber connected to interval change signal";
    }
    else
    {
        qWarning() << "GraphContainer: Invalid subscriber or slot provided to subscribeToIntervalChange";
    }
}

void GraphContainer::onTimeIntervalChanged(TimeInterval interval)
{
    DEBUG_OUT() << "GraphContainer: Handling time interval change to" << timeIntervalToString(interval);

    // Update the time interval locally
    updateTimeInterval(interval);

    // Emit the signal so GraphLayout can propagate to all other containers
    // This won't cause loops because setTimeInterval() (used by GraphLayout) doesn't emit signals
    emit IntervalChanged(interval);
}

void GraphContainer::onGraphContainerInFollowModeChanged(bool isInFollowMode)
{
    m_isInFollowMode = isInFollowMode;
    
    // Update sync state so other containers can be synchronized
    if (m_syncState)
    {
        m_syncState->isGraphContainerInFollowMode = isInFollowMode;
    }
    
    DEBUG_OUT() << "GraphContainer: Graph container in follow mode changed to" << isInFollowMode;
}

void GraphContainer::updateTimeInterval(TimeInterval interval)
{
    // Set flag to prevent TimeScopeChanged from interfering
    m_updatingTimeInterval = true;

    // Update ALL waterfall graphs' time interval (not just current one)
    // This ensures all graphs have the correct interval when switching between them
    // setTimeInterval() updates the interval, recalculates time ranges, and triggers a redraw
    for (auto &pair : m_waterfallGraphs)
    {
        if (pair.second)
        {
            pair.second->setTimeInterval(interval);
        }
    }

    // Explicitly redraw the current waterfall graph with full redraw
    // Time interval change requires full redraw since visible data changes significantly
    if (m_currentWaterfallGraph)
    {
        m_currentWaterfallGraph->forceFullRedraw();
    }

    // Update the time selection visualizer time interval
    if (m_timelineSelectionView)
    {
        m_timelineSelectionView->setTimeLineLength(interval);
    }

    // Update the timeline view time interval
    if (m_timelineView)
    {
        m_timelineView->setTimeLineLength(interval);
    }

    // Ensure timer is still running after interval change to keep animation active
    if (m_timer && !m_timer->isActive())
    {
        m_timer->start();
    }

    // Reset flag after a short delay to allow signals to propagate
    // Use QTimer::singleShot to reset the flag after the current event loop
    QTimer::singleShot(0, this, [this]() {
        m_updatingTimeInterval = false;
    });
}

void GraphContainer::setTimeInterval(TimeInterval interval)
{
    // Update the interval without emitting signals (for centralized sync from GraphLayout)
    updateTimeInterval(interval);
    
    // Note: This method does NOT emit IntervalChanged signal to avoid event loops
    // when GraphLayout is synchronizing intervals across containers
}

void GraphContainer::onSelectionCreated(const TimeSelectionSpan &selection)
{
    if (m_timelineSelectionView)
    {
        m_timelineSelectionView->addTimeSelection(selection);
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
    // Emit the TimeSelectionCreated signal for external components
    emit TimeSelectionCreated(selection);
}

void GraphContainer::onTimeScopeChanged(const TimeSelectionSpan &selection)
{
    // Skip processing if we're in the middle of updating the time interval
    // This prevents TimeScopeChanged (which is emitted as a side effect of interval changes)
    // from interfering with the proper time range setup during interval updates
    if (m_updatingTimeInterval)
    {
        return;
    }

    if (!m_currentWaterfallGraph)
    {
        return;
    }

    // Update the waterfall graph's time range to match the visible scope
    // setTimeRange() no longer auto-draws - it sets INCREMENTAL_UPDATE state
    m_currentWaterfallGraph->setTimeRange(selection.startTime, selection.endTime);
    
    // Trigger incremental redraw - this will use the state machine to efficiently
    // update only what changed rather than clearing and recreating everything
    m_currentWaterfallGraph->draw();

    // Update shared sync state so other containers can be synchronized
    if (m_syncState)
    {
        m_syncState->currentTimeScope = selection;
        m_syncState->hasTimeScope = true;
        m_lastSyncedTimeScope = selection;
        m_hasLastSyncedTimeScope = true;
    }

    // Emit the signal so GraphLayout hub can propagate to other containers
    emit TimeScopeChanged(selection);
}

void GraphContainer::setTimeScope(const TimeSelectionSpan &selection)
{
    // This method is called by GraphLayout hub to update time scope without triggering signals
    // Skip processing if we're in the middle of updating the time interval
    if (m_updatingTimeInterval)
    {
        return;
    }

    if (!m_currentWaterfallGraph)
    {
        return;
    }

    // Update the waterfall graph's time range to match the visible scope
    // setTimeRange() no longer auto-draws - it sets INCREMENTAL_UPDATE state
    m_currentWaterfallGraph->setTimeRange(selection.startTime, selection.endTime);
    
    // Trigger incremental redraw - this will use the state machine to efficiently
    // update only what changed rather than clearing and recreating everything
    m_currentWaterfallGraph->draw();

    // Update timeline view slider position to match the time scope
    if (m_timelineView)
    {
        m_timelineView->setTimeWindowSilent(selection);
    }

    // Update local tracking
    m_lastSyncedTimeScope = selection;
    m_hasLastSyncedTimeScope = true;
}

void GraphContainer::setMouseSelectionEnabled(bool enabled)
{
    if (m_currentWaterfallGraph)
    {
        m_currentWaterfallGraph->setMouseSelectionEnabled(enabled);
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
        DEBUG_OUT() << "GraphContainer: Test selection rectangle called";
    }
}

void GraphContainer::setCurrentTime(const QTime &time)
{
    DEBUG_OUT() << "GraphContainer: Setting current time to" << time.toString();
    if (m_timelineSelectionView)
    {
        m_timelineSelectionView->setCurrentTime(time);
    }

    if (m_timelineView)
    {
        // Update mode from sync state if available and mode has changed
        if (m_syncState)
        {
            TimelineViewMode newMode = m_syncState->isGraphContainerInFollowMode 
                ? TimelineViewMode::FOLLOW_MODE 
                : TimelineViewMode::FROZEN_MODE;
            
            // Only update mode if it has actually changed to avoid resetting slider unnecessarily
            if (m_timelineView->getTimelineViewMode() != newMode)
            {
                m_timelineView->setTimelineViewMode(newMode);
            }
        }
        
        // TimelineView will decide whether to update slider based on its current mode
        m_timelineView->setCurrentTime(time);
    }
}

void GraphContainer::setCursorTimeChangedCallback(const std::function<void(GraphContainer *, const QDateTime &)> &callback)
{
    m_cursorTimeChangedCallback = callback;
}

void GraphContainer::applySharedTimeAxisCursor(const QDateTime &time)
{
    m_sharedCursorTime = time;
    m_hasSharedCursorTime = time.isValid();

    for (auto &pair : m_waterfallGraphs)
    {
        applyCursorTimeToGraph(pair.second);
    }
}

void GraphContainer::addTimeSelection(const TimeSelectionSpan &selection)
{
    DEBUG_OUT() << "GraphContainer: Adding time selection from" << selection.startTime.toString() << "to" << selection.endTime.toString();

    if (m_timelineSelectionView)
    {
        m_timelineSelectionView->addTimeSelection(selection);
        DEBUG_OUT() << "GraphContainer: Time selection added to timeline selection view";
    }
    else
    {
        qWarning() << "GraphContainer: Timeline selection view is null - cannot add selection";
    }
}

void GraphContainer::clearTimeSelections()
{
    DEBUG_OUT() << "GraphContainer: Clearing all time selections";

    if (m_timelineSelectionView)
    {
        m_timelineSelectionView->clearTimeSelections();
        DEBUG_OUT() << "GraphContainer: All time selections cleared from timeline selection view";

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
    DEBUG_OUT() << "GraphContainer: Silently clearing all time selections (no signal emission)";

    if (m_timelineSelectionView)
    {
        m_timelineSelectionView->clearTimeSelections();
        DEBUG_OUT() << "GraphContainer: All time selections cleared from timeline selection view (silent)";

        // Do NOT emit signal to prevent cyclic dependencies
    }
    else
    {
        qWarning() << "GraphContainer: Timeline selection view is null - cannot clear selections";
    }
}

void GraphContainer::deleteInteractiveMarkers()
{
    DEBUG_OUT() << "GraphContainer: deleteInteractiveMarkers invoked";
    emit DeleteInteractiveMarkers();
}

void GraphContainer::initializeZoomPanelLimits()
{
    if (!m_zoomPanel)
    {
        DEBUG_OUT() << "GraphContainer: Cannot initialize zoom panel limits - no zoom panel";
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
        DEBUG_OUT() << "GraphContainer: Cannot initialize zoom panel limits - no data available";
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

        DEBUG_OUT() << "GraphContainer: Using stored range limit for" << graphTypeToString(currentDataOption) << "- Min:" << dataMin << "Max:" << dataMax;
    }


    // Calculate center value (linear interpolation)
    qreal centerValue = dataMin + (dataMax - dataMin) * 0.5;

    // Only update zoom panel if user hasn't customized
    // If customized, preserve all values (original and display) - don't update anything
    if (!m_zoomPanel->hasUserModifiedBounds())
    {
        // User hasn't customized - update original values to new data range
        m_zoomPanel->setOriginalRangeValues(dataMin, centerValue, dataMax);
        // Also update display values
        m_zoomPanel->setLeftLabelValue(dataMin);
        m_zoomPanel->setCenterLabelValue(centerValue);
        m_zoomPanel->setRightLabelValue(dataMax);
        
        // Update zero axis value for BDW, BRW, FDW graphs (use center sticker value)
        if (m_currentWaterfallGraph)
        {
            m_currentWaterfallGraph->setZeroAxisValue(centerValue);
        }
        
        DEBUG_OUT() << "GraphContainer: Zoom panel limits updated - Min:" << dataMin
                 << "Center:" << centerValue << "Max:" << dataMax << "- User has not customized";
    }
    else
    {
        // User has customized - don't update anything (preserve all zoom state)
        // Original values remain constant, display values remain unchanged
        DEBUG_OUT() << "GraphContainer: Zoom panel limits NOT updated - user has customized zoom - preserving state";
    }
}

void GraphContainer::onDataChanged(GraphType graphType)
{
    // Only process if this container has this data option
    if (!hasDataOption(graphType))
    {
        return;
    }

    // If this is the currently displayed graph, update UI components
    if (getCurrentDataOption() == graphType)
    {
        // Initialize or update the graph's time range from timeline view
        // This ensures the graph starts drawing immediately when data arrives
        if (m_currentWaterfallGraph && m_timelineView)
        {
            TimelineView *timelineView = m_timelineView;
            auto timeRange = m_currentWaterfallGraph->getTimeRange();
            
            // Check if graph has a valid time range
            bool hasValidTimeRange = timeRange.first.isValid() && timeRange.second.isValid() && 
                                     timeRange.first < timeRange.second;
            
            // If graph doesn't have a valid time range, initialize it from timeline view
            if (!hasValidTimeRange)
            {
                TimeSelectionSpan timelineWindow = timelineView->getVisibleTimeWindow();
                if (timelineWindow.startTime.isValid() && timelineWindow.endTime.isValid())
                {
                    // Initialize graph time range from timeline view's current window
                    m_currentWaterfallGraph->setTimeRange(timelineWindow.startTime, timelineWindow.endTime);
                    DEBUG_OUT() << "GraphContainer: Initialized graph time range from timeline view -" 
                             << timelineWindow.startTime.toString() << "to" << timelineWindow.endTime.toString();
                }
            }
            else
            {
                // Graph has a valid time range - check if we need to update it for new data
                QDateTime currentTime = QDateTime::currentDateTime();
                qint64 timeDiffMs = timeRange.second.msecsTo(currentTime);
                
                // If timeMax is within 1 minute of current time, we're showing recent data
                // Update the time range to include new data points
                if (timeDiffMs >= 0 && timeDiffMs < 60000) // Within 1 minute
                {
                    // Get the latest data time
                    if (m_currentWaterfallGraph->getDataSource())
                    {
                        QDateTime latestDataTime = m_currentWaterfallGraph->getDataSource()->getLatestTime();
                        if (latestDataTime.isValid() && latestDataTime > timeRange.second)
                        {
                            // New data is after current timeMax, update the range
                            // Keep the same interval but shift the window forward
                            qint64 intervalMs = timeRange.first.msecsTo(timeRange.second);
                            QDateTime newTimeMin = latestDataTime.addMSecs(-intervalMs);
                            QDateTime newTimeMax = latestDataTime;
                            
                            // Update the graph's time range
                            m_currentWaterfallGraph->setTimeRange(newTimeMin, newTimeMax);
                            
                            // Also update the timeline view to match
                            TimeSelectionSpan newWindow(newTimeMin, newTimeMax);
                            timelineView->setVisibleTimeWindow(newWindow);
                            
                            DEBUG_OUT() << "GraphContainer: Updated time range to show new data -" 
                                     << newTimeMin.toString() << "to" << newTimeMax.toString();
                        }
                    }
                }
            }
        }
        
        // Update zoom panel limits to reflect new data ranges
        initializeZoomPanelLimits();

        // Force full redraw of the waterfall graph when new data arrives
        if (m_currentWaterfallGraph)
        {
            m_currentWaterfallGraph->forceFullRedraw();
        }
        
        // Ensure timer is running to continue animation after data update
        if (m_timer && !m_timer->isActive())
        {
            m_timer->start();
            DEBUG_OUT() << "GraphContainer: Timer restarted after data change to continue animation";
        }

        // Update valid time range in TimeSelectionVisualizer from available data
        if (m_timelineSelectionView)
        {
            try
            {
                auto timeRange = getAvailableDataTimeRange();
                if (timeRange.first.isValid() && timeRange.second.isValid())
                {
                    QTime startTime = timeRange.first.time();
                    QTime endTime = timeRange.second.time();
                    m_timelineSelectionView->setValidSelectionRange(startTime, endTime);
                    DEBUG_OUT() << "GraphContainer: Updated TimeSelectionVisualizer valid range to" 
                             << startTime.toString() << "to" << endTime.toString();
                }
                else
                {
                    // If time range is invalid, clear the valid range
                    m_timelineSelectionView->setValidSelectionRange(QTime(), QTime());
                    DEBUG_OUT() << "GraphContainer: Cleared TimeSelectionVisualizer valid range (invalid time range)";
                }
            }
            catch (const std::runtime_error &e)
            {
                // If data is empty or not available, clear the valid range
                m_timelineSelectionView->setValidSelectionRange(QTime(), QTime());
                DEBUG_OUT() << "GraphContainer: Cleared TimeSelectionVisualizer valid range -" << e.what();
            }
        }
    }
}

void GraphContainer::onZoomValueChanged(ZoomBounds bounds)
{
    if (!m_currentWaterfallGraph)
    {
        DEBUG_OUT() << "GraphContainer: Cannot update waterfall graph - no waterfall graph";
        return;
    }

    DEBUG_OUT() << "GraphContainer: Received interpolated zoom bounds - Lower:" << bounds.lowerbound
        << "Upper:" << bounds.upperbound;

    // The bounds are already interpolated by the zoom panel, so use them directly
    // No additional conversion needed - bounds.lowerbound and bounds.upperbound
    // are already in the correct data value range

    // Set the custom Y range directly from interpolated bounds
    m_currentWaterfallGraph->setCustomYRange(bounds.lowerbound, bounds.upperbound);

    // Update the time range to ensure only relevant data points are rendered
    m_currentWaterfallGraph->updateTimeRange();

    // Update zero axis value for BDW, BRW, FDW graphs (use center sticker value from zoom panel)
    if (m_zoomPanel)
    {
        qreal centerStickerValue = m_zoomPanel->getCenterLabelValue();
        m_currentWaterfallGraph->setZeroAxisValue(centerStickerValue);
    }

    // Sync BTW markers with the new zoom range
    auto it = m_waterfallGraphs.find(GraphType::BTW);
    if (it != m_waterfallGraphs.end())
    {
        BTWGraph *btwGraph = qobject_cast<BTWGraph*>(it->second);
        if (btwGraph)
        {
            BTWInteractiveOverlay *overlay = btwGraph->getInteractiveOverlay();
            if (overlay)
            {
                overlay->syncMarkersWithTimeline();
                DEBUG_OUT() << "GraphContainer: Synced BTW markers with zoom panel";
            }
        }
    }

    DEBUG_OUT() << "GraphContainer: Custom Y range set directly from interpolated bounds and time range updated";
}

void GraphContainer::onClearTimeSelectionsButtonClicked()
{
    DEBUG_OUT() << "GraphContainer: Clear time selections button clicked";
    clearTimeSelections();
}

// Marker timestamp slot implementations
void GraphContainer::onRTWRMarkerTimestampCaptured(const QDateTime &timestamp, const QPointF &position)
{
    DEBUG_OUT() << "GraphContainer: RTW R marker timestamp captured:" << timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz");
    emit RTWRMarkerTimestampCaptured(timestamp, position);
}

void GraphContainer::onRTWSymbolTimestampCaptured(const QDateTime &timestamp, const QPointF &position, const QString &symbolName)
{
    DEBUG_OUT() << "GraphContainer: RTW symbol timestamp captured:" << timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz") 
             << "symbol:" << symbolName;
    emit RTWSymbolTimestampCaptured(timestamp, position, symbolName);
}

void GraphContainer::onBTWManualMarkerPlaced(const QDateTime &timestamp, const QPointF &position)
{
    DEBUG_OUT() << "GraphContainer: BTW manual marker placed:" << timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz");
    emit BTWManualMarkerPlaced(timestamp, position);
}

void GraphContainer::onBTWManualMarkerClicked(const QDateTime &timestamp, const QPointF &position)
{
    DEBUG_OUT() << "GraphContainer: BTW manual marker clicked:" << timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz");
    emit BTWManualMarkerClicked(timestamp, position);
}

void GraphContainer::onBTWHorizontalLinePlaced(const QUuid &lineId, const QDateTime &timestamp)
{
    DEBUG_OUT() << "GraphContainer: BTW horizontal line placed:" << lineId.toString() << "at" << timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz");
    emit BTWHorizontalLinePlaced(lineId, timestamp);
}

void GraphContainer::onBTWHorizontalLineRemoved(const QUuid &lineId)
{
    DEBUG_OUT() << "GraphContainer: BTW horizontal line removed:" << lineId.toString();
    emit BTWHorizontalLineRemoved(lineId);
}

void GraphContainer::onBTWMarkerSyncDataChanged(const BTWSyncMarkerData &markerData)
{
    // This is called when receiving sync from another container
    // Create or update the marker in our BTW graph
    
    // Get the BTW graph
    auto it = m_waterfallGraphs.find(GraphType::BTW);
    if (it != m_waterfallGraphs.end()) {
        BTWGraph *btwGraph = qobject_cast<BTWGraph*>(it->second);
        if (btwGraph) {
            // Check if marker already exists
            if (btwGraph->hasMarkerWithSyncId(markerData.id)) {
                btwGraph->updateMarkerFromSyncData(markerData);
                DEBUG_OUT() << "GraphContainer: Updated synced BTW marker:" << markerData.id.toString();
            } else {
                btwGraph->createMarkerFromSyncData(markerData);
                DEBUG_OUT() << "GraphContainer: Created synced BTW marker:" << markerData.id.toString();
            }
        }
    }
}

void GraphContainer::onBTWMarkerSyncDeleted(const QUuid &markerId)
{
    // This is called when receiving sync from another container
    // Delete the marker from our BTW graph
    
    // Get the BTW graph
    auto it = m_waterfallGraphs.find(GraphType::BTW);
    if (it != m_waterfallGraphs.end()) {
        BTWGraph *btwGraph = qobject_cast<BTWGraph*>(it->second);
        if (btwGraph) {
            btwGraph->deleteMarkerBySyncId(markerId);
            DEBUG_OUT() << "GraphContainer: Deleted synced BTW marker:" << markerId.toString();
        }
    }
}

void GraphContainer::onShadedRegionSyncAdded(const ShadedRegionSyncData &regionData)
{
    // This is called when receiving sync from another container
    // Create the shaded region in our BTW graph
    
    auto it = m_waterfallGraphs.find(GraphType::BTW);
    if (it != m_waterfallGraphs.end()) {
        BTWGraph *btwGraph = qobject_cast<BTWGraph*>(it->second);
        if (btwGraph) {
            // Check if region already exists
            if (!btwGraph->hasShadedRegionWithSyncId(regionData.syncId)) {
                btwGraph->createShadedRegionFromSyncData(regionData);
                DEBUG_OUT() << "GraphContainer: Created synced shaded region:" << regionData.syncId.toString();
            }
        }
    }
}

void GraphContainer::onShadedRegionSyncRemoved(const QUuid &syncId)
{
    // This is called when receiving sync from another container
    // Delete the shaded region from our BTW graph
    
    auto it = m_waterfallGraphs.find(GraphType::BTW);
    if (it != m_waterfallGraphs.end()) {
        BTWGraph *btwGraph = qobject_cast<BTWGraph*>(it->second);
        if (btwGraph) {
            btwGraph->deleteShadedRegionBySyncId(syncId);
            DEBUG_OUT() << "GraphContainer: Deleted synced shaded region:" << syncId.toString();
        }
    }
}

void GraphContainer::onShadedRegionsSyncCleared()
{
    // This is called when receiving sync from another container
    // Clear all shaded regions from our BTW graph
    // Note: We need a method that clears without emitting signals to avoid loops
    
    auto it = m_waterfallGraphs.find(GraphType::BTW);
    if (it != m_waterfallGraphs.end()) {
        BTWGraph *btwGraph = qobject_cast<BTWGraph*>(it->second);
        if (btwGraph) {
            // For now, we'll need to track and delete each region individually
            // to avoid emitting new clear signals
            DEBUG_OUT() << "GraphContainer: Shaded regions sync cleared received";
            // Note: Full implementation would require a clearWithoutSignal method
        }
    }
}

// Chevron label control methods implementation
void GraphContainer::setChevronLabel1(const QString &label)
{
    if (m_timelineView)
    {
        m_timelineView->setChevronLabel1(label);
        DEBUG_OUT() << "GraphContainer: Set chevron label 1 to:" << label;
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
        DEBUG_OUT() << "GraphContainer: Set chevron label 2 to:" << label;
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
        DEBUG_OUT() << "GraphContainer: Set chevron label 3 to:" << label;
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

void GraphContainer::setManoeuvres(const std::vector<Manoeuvre> *manoeuvres)
{
    // Update shared sync state if available
    if (m_syncState)
    {
        if (manoeuvres)
        {
            m_syncState->manoeuvres = *manoeuvres;
            m_syncState->hasManoeuvres = true;
        }
        else
        {
            m_syncState->manoeuvres.clear();
            m_syncState->hasManoeuvres = false;
        }
    }
    
    // Propagate to timeline view if it exists
    if (m_timelineView)
    {
        m_timelineView->setManoeuvres(manoeuvres);
        DEBUG_OUT() << "GraphContainer: Set manoeuvres to timeline view - count:" << (manoeuvres ? manoeuvres->size() : 0);
    }
    else
    {
        qWarning() << "GraphContainer: Cannot set manoeuvres - timeline view is null";
    }
}

void GraphContainer::setInProgressManoeuvre(const QDateTime &startTime)
{
    if (m_timelineView)
    {
        m_timelineView->setInProgressManoeuvre(startTime);
    }
}

void GraphContainer::clearInProgressManoeuvre()
{
    if (m_timelineView)
    {
        m_timelineView->clearInProgressManoeuvre();
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
        qreal centerValue = yMin + (yMax - yMin) * 0.5;
        // Set original values (used for calculations) and display values
        m_zoomPanel->setOriginalRangeValues(yMin, centerValue, yMax);
        m_zoomPanel->setLeftLabelValue(yMin);
        m_zoomPanel->setCenterLabelValue(centerValue);
        m_zoomPanel->setRightLabelValue(yMax);
        
        // Update zero axis value for BDW, BRW, FDW graphs (use center sticker value)
        m_currentWaterfallGraph->setZeroAxisValue(centerValue);
        
        // Reset indicator to full range to restore initial state
        m_zoomPanel->resetIndicatorToFullRange();

        DEBUG_OUT() << "GraphContainer: Applied stored range limits for" << graphTypeToString(graphType)
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
        DEBUG_OUT() << "GraphContainer: No stored range limits for" << graphTypeToString(graphType)
                 << "- Auto-update enabled";
    }
}

void GraphContainer::clearAllGraphRangeLimits()
{
    graphRangeLimits.clear();

    m_currentWaterfallGraph->setAutoUpdateYRange(true);
    DEBUG_OUT() << "GraphContainer: Cleared all range limits - Auto-update enabled";
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

// Computed property getters implementation

QDateTime GraphContainer::getCurrentDisplayTimeMin() const
{
    if (!m_currentWaterfallGraph)
    {
        throw std::runtime_error("GraphContainer::getCurrentDisplayTimeMin(): No current waterfall graph set");
    }
    return m_currentWaterfallGraph->getTimeMin();
}

QDateTime GraphContainer::getCurrentDisplayTimeMax() const
{
    if (!m_currentWaterfallGraph)
    {
        throw std::runtime_error("GraphContainer::getCurrentDisplayTimeMax(): No current waterfall graph set");
    }
    return m_currentWaterfallGraph->getTimeMax();
}

std::pair<QDateTime, QDateTime> GraphContainer::getCurrentDisplayTimeRange() const
{
    if (!m_currentWaterfallGraph)
    {
        throw std::runtime_error("GraphContainer::getCurrentDisplayTimeRange(): No current waterfall graph set");
    }
    return m_currentWaterfallGraph->getTimeRange();
}

WaterfallData *GraphContainer::getCurrentWaterfallData() const
{
    auto it = dataOptions.find(currentDataOption);
    if (it != dataOptions.end())
    {
        return it->second;
    }
    throw std::runtime_error("GraphContainer::getCurrentWaterfallData(): No WaterfallData available for current data option");
}

QDateTime GraphContainer::getAvailableDataTimeMin() const
{
    WaterfallData *data = getCurrentWaterfallData();
    if (!data)
    {
        throw std::runtime_error("GraphContainer::getAvailableDataTimeMin(): No current WaterfallData available");
    }
    if (data->isEmpty())
    {
        throw std::runtime_error("GraphContainer::getAvailableDataTimeMin(): Current WaterfallData is empty");
    }
    return data->getCombinedTimeRange().first;
}

QDateTime GraphContainer::getAvailableDataTimeMax() const
{
    WaterfallData *data = getCurrentWaterfallData();
    if (!data)
    {
        throw std::runtime_error("GraphContainer::getAvailableDataTimeMax(): No current WaterfallData available");
    }
    if (data->isEmpty())
    {
        throw std::runtime_error("GraphContainer::getAvailableDataTimeMax(): Current WaterfallData is empty");
    }
    return data->getCombinedTimeRange().second;
}

std::pair<QDateTime, QDateTime> GraphContainer::getAvailableDataTimeRange() const
{
    WaterfallData *data = getCurrentWaterfallData();
    if (!data)
    {
        throw std::runtime_error("GraphContainer::getAvailableDataTimeRange(): No current WaterfallData available");
    }
    if (data->isEmpty())
    {
        throw std::runtime_error("GraphContainer::getAvailableDataTimeRange(): Current WaterfallData is empty");
    }
    return data->getCombinedTimeRange();
}

qreal GraphContainer::getAvailableDataYMin() const
{
    WaterfallData *data = getCurrentWaterfallData();
    if (!data)
    {
        throw std::runtime_error("GraphContainer::getAvailableDataYMin(): No current WaterfallData available");
    }
    if (data->isEmpty())
    {
        throw std::runtime_error("GraphContainer::getAvailableDataYMin(): Current WaterfallData is empty");
    }
    return data->getCombinedYRange().first;
}

qreal GraphContainer::getAvailableDataYMax() const
{
    WaterfallData *data = getCurrentWaterfallData();
    if (!data)
    {
        throw std::runtime_error("GraphContainer::getAvailableDataYMax(): No current WaterfallData available");
    }
    if (data->isEmpty())
    {
        throw std::runtime_error("GraphContainer::getAvailableDataYMax(): Current WaterfallData is empty");
    }
    return data->getCombinedYRange().second;
}

std::pair<qreal, qreal> GraphContainer::getAvailableDataYRange() const
{
    WaterfallData *data = getCurrentWaterfallData();
    if (!data)
    {
        throw std::runtime_error("GraphContainer::getAvailableDataYRange(): No current WaterfallData available");
    }
    if (data->isEmpty())
    {
        throw std::runtime_error("GraphContainer::getAvailableDataYRange(): Current WaterfallData is empty");
    }
    return data->getCombinedYRange();
}