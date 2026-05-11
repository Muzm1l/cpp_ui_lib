#include "graphlayout.h"
#include "navtimeutils.h"
#include "btwgraph.h"
#include "btwinteractiveoverlay.h"
#include "debugutils.h"
#include <QDebug>
#include <QElapsedTimer>

GraphLayout::GraphLayout(QWidget *parent, LayoutType layoutType, QTimer *timer, std::map<GraphType, std::vector<QPair<QString, QColor>>> seriesLabelsMap, const QDateTime &systemStartTimeAtInit)
    : QWidget{parent}, m_layoutType(layoutType), m_timer(timer), m_systemStartTimeAtInit(systemStartTimeAtInit)
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

        DEBUG_OUT() << "GraphLayout: Timer setup completed since none was provided - interval:" << m_timer->interval() << "ms";
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

    // Initialize manoeuvre drawing state
    m_manoeuvreDrawingInProgress = false;
    m_currentManoeuvreStartTime = QDateTime();
    m_currentManoeuvreBearing = 0;
    m_currentManoeuvreSpeed = 0;
    m_currentManoeuvreDepth = 0;

    // Install the ONE writer of GraphContainerSyncState::currentTimeScope.
    // Every other place that wrote that field has been removed: this callback
    // is the single ingress point.
    m_scopeBusWriterToken = m_scopeBus.subscribe(
        [this](const TimeScopeBus::Snapshot& s) {
            m_syncState.currentTimeScope = s.span;
            m_syncState.hasTimeScope     = true;
        });
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

        // Time-scope propagation is handled centrally by TimeScopeBus; no
        // direct container-to-container TimeScopeChanged wiring needed here.
        // Interval changes are also handled centrally by GraphLayout.

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

        // Time-scope propagation is handled centrally by TimeScopeBus.
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

        // Time-scope propagation is handled centrally by TimeScopeBus.
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

        // Interval changes are handled centrally by GraphLayout.
        connect(m_graphContainers[0], &GraphContainer::IntervalChanged, m_graphContainers[1], &GraphContainer::onTimeIntervalChanged);

        // Time-scope propagation is handled centrally by TimeScopeBus.
        break;
    case LayoutType::HIDDEN:
        // Hide all containers
        for (auto *container : m_graphContainers)
        {
            container->setVisible(false);
        }
        break;
    default:
        DEBUG_OUT() << "Invalid layout type selected";
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
    // Use Qt::UniqueConnection to prevent duplicate connections
    for (auto *container : m_graphContainers)
    {
        connect(container, &GraphContainer::TimeSelectionCreated,
                this, &GraphLayout::onTimeSelectionCreated, Qt::UniqueConnection);
        connect(container, &GraphContainer::TimeSelectionModified,
                this, &GraphLayout::onTimeSelectionModified, Qt::UniqueConnection);
        connect(container, &GraphContainer::TimeSelectionsCleared,
                this, &GraphLayout::onTimeSelectionsCleared, Qt::UniqueConnection);
        connect(container, &GraphContainer::IntervalChanged,
                this, &GraphLayout::onContainerIntervalChanged, Qt::UniqueConnection);
        // Time-scope propagation is handled by TimeScopeBus, not by container signals.

        container->attachSharedCacheStore(&m_sharedRenderCache);

        // Re-attach the time-scope bus (idempotent) so layout switches do not
        // leave a container without a subscription.
        container->attachScopeBus(&m_scopeBus);

        // Connect marker timestamp signals
        connect(container, &GraphContainer::RTWRMarkerTimestampCaptured,
                this, &GraphLayout::RTWRMarkerTimestampCaptured, Qt::UniqueConnection);
        connect(container, &GraphContainer::RTWSymbolTimestampCaptured,
                this, &GraphLayout::RTWSymbolTimestampCaptured, Qt::UniqueConnection);
        connect(container, &GraphContainer::BTWManualMarkerPlaced,
                this, &GraphLayout::onBTWManualMarkerPlaced, Qt::UniqueConnection);
        // Also forward the signal for external integration
        connect(container, &GraphContainer::BTWManualMarkerPlaced,
                this, &GraphLayout::BTWManualMarkerPlaced, Qt::UniqueConnection);
        connect(container, &GraphContainer::BTWManualMarkerClicked,
                this, &GraphLayout::BTWManualMarkerClicked, Qt::UniqueConnection);
        
        // Connect BTW horizontal line signals
        connect(container, &GraphContainer::BTWHorizontalLinePlaced,
                this, &GraphLayout::onBTWHorizontalLinePlaced, Qt::UniqueConnection);
        connect(container, &GraphContainer::BTWHorizontalLineRemoved,
                this, &GraphLayout::onBTWHorizontalLineRemoved, Qt::UniqueConnection);
        
        connect(container, &GraphContainer::markerTimestampValueChanged,
                this, &GraphLayout::markerTimestampValueChanged, Qt::UniqueConnection);
        connect(container, &GraphContainer::markerClickedWithData,
                this, &GraphLayout::markerClickedWithData, Qt::UniqueConnection);
        
        // Connect BTW marker sync signals to propagate to all containers
        connect(container, &GraphContainer::BTWMarkerSyncDataChanged,
                this, &GraphLayout::onBTWMarkerSyncDataChanged, Qt::UniqueConnection);
        connect(container, &GraphContainer::BTWMarkerSyncDeleted,
                this, &GraphLayout::onBTWMarkerSyncDeleted, Qt::UniqueConnection);
        
        // Connect shaded region sync signals to propagate to all containers
        connect(container, &GraphContainer::ShadedRegionSyncAdded,
                this, &GraphLayout::onShadedRegionSyncAdded, Qt::UniqueConnection);
        connect(container, &GraphContainer::ShadedRegionSyncRemoved,
                this, &GraphLayout::onShadedRegionSyncRemoved, Qt::UniqueConnection);
        connect(container, &GraphContainer::ShadedRegionsSyncCleared,
                this, &GraphLayout::onShadedRegionsSyncCleared, Qt::UniqueConnection);
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
        
        // Create engine (owns WaterfallData internally)
        m_engines[graphType] = new GraphEngine(graphType, seriesLabels, this);
        
        // Connect engine signals
        connect(m_engines[graphType], &GraphEngine::dataAppended,
                this, [this, graphType](const QString &seriesLabel) {
            // Notify containers
            for (auto *container : m_graphContainers) {
                if (container) {
                    container->onDataChanged(graphType);
                }
            }
        });
        
        // Set colors for each series (this will require updating WaterfallData to support colors)
        for (const auto& seriesPair : seriesData) {
            m_seriesColorsMap[seriesPair.first] = seriesPair.second;
        }

    }
}

void GraphLayout::initializeContainers()
{
    if (m_systemStartTimeAtInit.isValid())
    {
        m_syncState.applicationStartTime = m_systemStartTimeAtInit;
        m_syncState.hasApplicationStartTime = true;
    }
    else if (!m_syncState.hasApplicationStartTime)
    {
        m_syncState.applicationStartTime = QDateTime::currentDateTime();
        m_syncState.hasApplicationStartTime = true;
    }

    // Create 4 graph containers with timer and shared sync state
    m_graphContainers.push_back(new GraphContainer(this, true, m_seriesColorsMap, m_timer, 0, 0, &m_syncState));
    m_graphContainers.push_back(new GraphContainer(this, true, m_seriesColorsMap, m_timer, 0, 0, &m_syncState));
    m_graphContainers.push_back(new GraphContainer(this, true, m_seriesColorsMap, m_timer, 0, 0, &m_syncState));
    m_graphContainers.push_back(new GraphContainer(this, true, m_seriesColorsMap, m_timer, 0, 0, &m_syncState));

    // Attach the centralized time-scope bus to every container.
    // Containers publish slider/programmatic intents into the bus and apply
    // every broadcast snapshot to their own waterfall + timeline.
    for (auto *container : m_graphContainers)
    {
        if (container)
            container->attachScopeBus(&m_scopeBus);
    }

    // Attach data sources to containers
    attachContainerDataSources();
    
    // Connect all containers' TimeSelectionCreated, TimeSelectionModified, and TimeSelectionsCleared signals to our slots
    // Use Qt::UniqueConnection to prevent duplicate connections
    for (auto *container : m_graphContainers)
    {
        connect(container, &GraphContainer::TimeSelectionCreated,
                this, &GraphLayout::onTimeSelectionCreated, Qt::UniqueConnection);
        connect(container, &GraphContainer::TimeSelectionModified,
                this, &GraphLayout::onTimeSelectionModified, Qt::UniqueConnection);
        connect(container, &GraphContainer::TimeSelectionsCleared,
                this, &GraphLayout::onTimeSelectionsCleared, Qt::UniqueConnection);
        connect(container, &GraphContainer::IntervalChanged,
                this, &GraphLayout::onContainerIntervalChanged, Qt::UniqueConnection);
        // Time-scope propagation is handled by TimeScopeBus, not by container signals.

        container->attachSharedCacheStore(&m_sharedRenderCache);
        
        // Connect marker timestamp signals
        connect(container, &GraphContainer::RTWRMarkerTimestampCaptured,
                this, &GraphLayout::RTWRMarkerTimestampCaptured, Qt::UniqueConnection);
        connect(container, &GraphContainer::RTWSymbolTimestampCaptured,
                this, &GraphLayout::RTWSymbolTimestampCaptured, Qt::UniqueConnection);
        connect(container, &GraphContainer::BTWManualMarkerPlaced,
                this, &GraphLayout::onBTWManualMarkerPlaced, Qt::UniqueConnection);
        // Also forward the signal for external integration
        connect(container, &GraphContainer::BTWManualMarkerPlaced,
                this, &GraphLayout::BTWManualMarkerPlaced, Qt::UniqueConnection);
        connect(container, &GraphContainer::BTWManualMarkerClicked,
                this, &GraphLayout::BTWManualMarkerClicked, Qt::UniqueConnection);
        
        // Connect BTW horizontal line signals
        connect(container, &GraphContainer::BTWHorizontalLinePlaced,
                this, &GraphLayout::onBTWHorizontalLinePlaced, Qt::UniqueConnection);
        connect(container, &GraphContainer::BTWHorizontalLineRemoved,
                this, &GraphLayout::onBTWHorizontalLineRemoved, Qt::UniqueConnection);
        
        connect(container, &GraphContainer::markerTimestampValueChanged,
                this, &GraphLayout::markerTimestampValueChanged, Qt::UniqueConnection);
        connect(container, &GraphContainer::markerClickedWithData,
                this, &GraphLayout::markerClickedWithData, Qt::UniqueConnection);
        
        // Connect BTW marker sync signals to propagate to all containers
        connect(container, &GraphContainer::BTWMarkerSyncDataChanged,
                this, &GraphLayout::onBTWMarkerSyncDataChanged, Qt::UniqueConnection);
        connect(container, &GraphContainer::BTWMarkerSyncDeleted,
                this, &GraphLayout::onBTWMarkerSyncDeleted, Qt::UniqueConnection);
        
        // Connect shaded region sync signals to propagate to all containers
        connect(container, &GraphContainer::ShadedRegionSyncAdded,
                this, &GraphLayout::onShadedRegionSyncAdded, Qt::UniqueConnection);
        connect(container, &GraphContainer::ShadedRegionSyncRemoved,
                this, &GraphLayout::onShadedRegionSyncRemoved, Qt::UniqueConnection);
        connect(container, &GraphContainer::ShadedRegionsSyncCleared,
                this, &GraphLayout::onShadedRegionsSyncCleared, Qt::UniqueConnection);
    }

    propagateSystemStartTimeToContainers();

    registerCursorSyncCallbacks();
}

void GraphLayout::setSystemStartTime(const QDateTime &t)
{
    if (!t.isValid())
        return;
    m_syncState.applicationStartTime = t;
    m_syncState.hasApplicationStartTime = true;
    propagateSystemStartTimeToContainers();
}

QDateTime GraphLayout::systemStartTime() const
{
    if (m_syncState.hasApplicationStartTime && m_syncState.applicationStartTime.isValid())
        return m_syncState.applicationStartTime;
    return QDateTime();
}

void GraphLayout::clearSystemStartTime()
{
    m_syncState.hasApplicationStartTime = false;
    m_syncState.applicationStartTime = QDateTime();
    propagateSystemStartTimeToContainers();
}

void GraphLayout::setTimelineEndOverride(const QDateTime &t)
{
    m_syncState.setTimelineEndOverride(t);
}

void GraphLayout::clearTimelineEndOverride()
{
    m_syncState.clearTimelineEndOverride();
}

void GraphLayout::propagateSystemStartTimeToContainers()
{
    for (GraphContainer *c : m_graphContainers)
    {
        if (c)
            c->applySharedSystemStartTimeFromSync();
    }
}

void GraphLayout::attachContainerDataSources()
{
    // Go through each of the graph containers and attach each of the
    // engines using the key as the title and the engine's data as the datasource
    for (auto *container : m_graphContainers)
    {
        for (auto &enginePair : m_engines)
        {
            container->addDataOption(enginePair.first, *enginePair.second->dataMutable());
        }
    }
}

void GraphLayout::updateLayoutSizing()
{
    // Standard sizes for layout types
    // x = combined width of timelineview (64) and history selection (32) = 96
    const int x = 64 + 32; // Timeline view width + history selection width
    const int standardHeight = 900;
    
    // Get combo box and zoom panel height from first visible container
    int comboBoxAndZoomPanelHeight = 0;
    for (auto *container : m_graphContainers)
    {
        if (container && container->isVisible())
        {
            comboBoxAndZoomPanelHeight = container->getComboBoxAndZoomPanelHeight();
            break; // Use height from first visible container (all should be the same)
        }
    }
    
    // Calculate container heights based on layout type
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
        containerHeight = (standardHeight - spacingHeight) / numRows;
        
        // Ensure minimum height
        containerHeight = qMax(containerHeight, 200);
        
        // Calculate graph height by subtracting combo box and zoom panel height from container height
        // This ensures graphs are symmetrical and fill the remaining space
        int graphHeight = containerHeight - comboBoxAndZoomPanelHeight;
        
        // Ensure graph has minimum height
        graphHeight = qMax(graphHeight, 100);
        
        // Set graph view size for all containers to ensure symmetrical graphs
        // Preserve each container's existing width
        for (auto *container : m_graphContainers)
        {
            if (container && container->isVisible())
            {
                int graphWidth = container->getGraphViewSize().width();
                // If width is not set yet, use default
                if (graphWidth <= 0)
                {
                    graphWidth = 226; // Default width
                }
                container->setGraphViewSize(graphWidth, graphHeight);
            }
        }
    }
    
    // Set container heights for all visible containers
    for (auto *container : m_graphContainers)
    {
        if (container && container->isVisible())
        {
            container->setContainerHeight(containerHeight);
        }
    }
    
    // Set container widths and total width based on standard sizes
    int totalWidth = 0;
    const int timelineViewWidth = 64; // Timeline view width
    
    switch (m_layoutType)
    {
    case LayoutType::GPW1W:
        // 1W: Width: 226 + x; h = 900
        if (m_graphContainers[0] && m_graphContainers[0]->isVisible()) {
            m_graphContainers[0]->setContainerWidth(226 + x);
        }
        totalWidth = 226 + x;
        break;
        
    case LayoutType::GPW2WH:
        // GPW2W: 226 + x + 226; h = 900
        for (int i = 0; i < 2; ++i) {
            if (m_graphContainers[i] && m_graphContainers[i]->isVisible()) {
                if (i == 0) {
                    m_graphContainers[i]->setContainerWidth(226);
                } else {
                    m_graphContainers[i]->setContainerWidth(226 + x);
                }
            }
        }
        totalWidth = 226 + x + 226;
        break;
        
    case LayoutType::NOGPW2WH:
        // NOGPW2WH: 580 + 582 + x; H = 900
        for (int i = 0; i < 2; ++i) {
            if (m_graphContainers[i] && m_graphContainers[i]->isVisible()) {
                if (i == 0) {
                    m_graphContainers[i]->setContainerWidth(580);
                } else {
                    m_graphContainers[i]->setContainerWidth(582 + x);
                }
            }
        }
        totalWidth = 580 + 582 + x;
        break;
        
    case LayoutType::GPW4WH:
        // GPW4WH: 290 + 290 + x + 290 + 290 + 3 (3px spacing between 4 containers)
        for (int i = 0; i < 4; ++i) {
            if (m_graphContainers[i] && m_graphContainers[i]->isVisible()) {
                if (i == 2) {
                    // Third container (index 2) has timeline view, so gets extra width
                    m_graphContainers[i]->setContainerWidth(290 + x);
                } else {
                    m_graphContainers[i]->setContainerWidth(290);
                }
            }
        }
        totalWidth = 290 + 290 + x + 290 + 290 + 3; // Add 3px for spacing between 4 containers
        break;
        
    case LayoutType::GPW2WV:
        // GPW4WV: 226 + x; h = 900
        if (m_graphContainers[0] && m_graphContainers[0]->isVisible()) {
            m_graphContainers[0]->setContainerWidth(226 + x);
        }
        if (m_graphContainers[2] && m_graphContainers[2]->isVisible()) {
            m_graphContainers[2]->setContainerWidth(226 + x);
        }
        totalWidth = 226 + x;
        break;
        
    case LayoutType::GPW4W:
        // GPW4W: 226 + x + 226; h = 900
        for (int i = 0; i < 4; ++i) {
            if (m_graphContainers[i] && m_graphContainers[i]->isVisible()) {
                if (i == 1 || i == 3) {
                    // Second container in each row has timeline view, so gets extra width
                    m_graphContainers[i]->setContainerWidth(226 + x);
                } else {
                    // First container in each row gets standard width (no timeline view)
                    m_graphContainers[i]->setContainerWidth(226);
                }
            }
        }
        totalWidth = 226 + x + 226;
        break;
        
    case LayoutType::HIDDEN:
        totalWidth = 0;
        break;
    }
    
    // Set fixed width and height for the GraphLayout widget
    if (m_layoutType != LayoutType::HIDDEN) {
        setFixedWidth(totalWidth);
        setFixedHeight(standardHeight);
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
        DEBUG_OUT() << "Container not found:" << containerLabel;
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
        DEBUG_OUT() << "Container not found:" << containerLabel;
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
        DEBUG_OUT() << "Container not found:" << containerLabel;
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
        DEBUG_OUT() << "Container not found:" << containerLabel;
    }
}

GraphType GraphLayout::getCurrentDataOption(const QString &containerLabel) const
{
    int containerIndex = getContainerIndex(containerLabel);
    if (containerIndex >= 0 && containerIndex < static_cast<int>(m_graphContainers.size()))
    {
        return m_graphContainers[containerIndex]->getCurrentDataOption();
    }
    DEBUG_OUT() << "Container not found:" << containerLabel;
    return GraphType::BDW;
}

void GraphLayout::setContainerGraphType(int containerIndex, const GraphType &graphType)
{
    if (containerIndex >= 0 && containerIndex < static_cast<int>(m_graphContainers.size()))
    {
        m_graphContainers[containerIndex]->setCurrentDataOption(graphType);
        DEBUG_OUT() << "GraphLayout: Set container" << containerIndex << "to graph type" << static_cast<int>(graphType);
    }
    else
    {
        DEBUG_OUT() << "GraphLayout: Invalid container index:" << containerIndex;
    }
}

GraphType GraphLayout::getContainerGraphType(int containerIndex) const
{
    if (containerIndex >= 0 && containerIndex < static_cast<int>(m_graphContainers.size()))
    {
        return m_graphContainers[containerIndex]->getCurrentDataOption();
    }
    DEBUG_OUT() << "GraphLayout: Invalid container index:" << containerIndex;
    return GraphType::BDW;
}

std::vector<GraphType> GraphLayout::getAvailableDataOptions(const QString &containerLabel) const
{
    int containerIndex = getContainerIndex(containerLabel);
    if (containerIndex >= 0 && containerIndex < static_cast<int>(m_graphContainers.size()))
    {
        return m_graphContainers[containerIndex]->getAvailableDataOptions();
    }
    DEBUG_OUT() << "Container not found:" << containerLabel;
    return std::vector<GraphType>();
}

WaterfallData *GraphLayout::getDataOption(const QString &containerLabel, const GraphType &graphType)
{
    int containerIndex = getContainerIndex(containerLabel);
    if (containerIndex >= 0 && containerIndex < static_cast<int>(m_graphContainers.size()))
    {
        return m_graphContainers[containerIndex]->getDataOption(graphType);
    }
    DEBUG_OUT() << "Container not found:" << containerLabel;
    return nullptr;
}

bool GraphLayout::hasDataOption(const QString &containerLabel, const GraphType &graphType) const
{
    int containerIndex = getContainerIndex(containerLabel);
    if (containerIndex >= 0 && containerIndex < static_cast<int>(m_graphContainers.size()))
    {
        return m_graphContainers[containerIndex]->hasDataOption(graphType);
    }
    DEBUG_OUT() << "Container not found:" << containerLabel;
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

void GraphLayout::addDataPointToDataSource(const GraphType &graphType, const QString &seriesLabel, float yValue, const QDateTime &timestamp)
{
    QString dataSourceLabel = graphTypeToString(graphType);
    auto it = m_engines.find(graphType);
    if (it != m_engines.end())
    {
        m_sharedRenderCache.bumpDataEpoch();
        it->second->addDataPoint(seriesLabel, yValue, timestamp);
        DEBUG_OUT() << "Added data point to" << dataSourceLabel << "series" << seriesLabel << "y:" << yValue << "time:" << timestamp.toString();

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
        DEBUG_OUT() << "Engine not found:" << dataSourceLabel;
    }
}

void GraphLayout::addDataPointsToDataSource(const GraphType &graphType, const QString &seriesLabel, const std::vector<float> &yValues, const std::vector<QDateTime> &timestamps)
{
    QString dataSourceLabel = graphTypeToString(graphType);
    auto it = m_engines.find(graphType);
    if (it != m_engines.end())
    {
        m_sharedRenderCache.bumpDataEpoch();
        it->second->addDataPoints(seriesLabel, yValues, timestamps);
        DEBUG_OUT() << "Added" << yValues.size() << "data points to" << dataSourceLabel << "series" << seriesLabel;

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
        DEBUG_OUT() << "Engine not found:" << dataSourceLabel;
    }
}

void GraphLayout::setDataToDataSource(const GraphType &graphType, const QString &seriesLabel, const std::vector<float> &yData, const std::vector<QDateTime> &timestamps)
{
    QString dataSourceLabel = graphTypeToString(graphType);
    auto it = m_engines.find(graphType);
    if (it != m_engines.end())
    {
        m_sharedRenderCache.bumpDataEpoch();
        it->second->setDataSeries(seriesLabel, yData, timestamps);
        DEBUG_OUT() << "Set data for" << dataSourceLabel << "series" << seriesLabel << "size:" << yData.size();

        // If data is empty, ensure graphs are properly cleared
        // Check if the input data is empty or if the engine's data is empty after setting
        bool isEmpty = yData.empty() || timestamps.empty() || it->second->isEmpty();
        if (isEmpty)
        {
            DEBUG_OUT() << "Data is empty for" << dataSourceLabel << "series" << seriesLabel << "- triggering full redraw to clear graphs";
        }

        // Notify all containers that have this data source to update their UI
        for (auto *container : m_graphContainers)
        {
            if (container)
            {
                // If data is empty, force a full redraw to ensure graphs are cleared
                // This ensures existing graph graphics are removed when data becomes empty
                if (isEmpty)
                {
                    container->redrawWaterfallGraph(graphType);
                }
                else
                {
                    container->onDataChanged(graphType);
                }
            }
        }
    }
    else
    {
        DEBUG_OUT() << "Engine not found:" << dataSourceLabel;
    }
}

void GraphLayout::setDataToDataSourceInteractive(const GraphType &graphType, const QString &seriesLabel, 
                                                const std::vector<float> &yData, const std::vector<QDateTime> &timestamps)
{
    QString dataSourceLabel = graphTypeToString(graphType);
    auto it = m_engines.find(graphType);
    if (it != m_engines.end())
    {
        // Always update data in engine (data must be current)
        it->second->setDataSeries(seriesLabel, yData, timestamps);
        
        // Throttle UI updates: only update if enough time has passed since last update
        // This prevents overwhelming the UI with too many repaints
        auto &timer = m_interactiveUpdateTimers[graphType];
        if (!timer.isValid())
        {
            timer.start();
        }
        
        qint64 elapsedMs = timer.elapsed();
        if (elapsedMs >= INTERACTIVE_UPDATE_THROTTLE_MS)
        {
            // Enough time has passed, process the update
            timer.restart();
            m_pendingInteractiveUpdate[graphType] = false;
            
            DEBUG_OUT() << "Interactive drag update for" << dataSourceLabel << "series" << seriesLabel << "size:" << yData.size();

            // Optimize: Only notify containers that actually display this graph type
            // This avoids unnecessary function calls and checks
            for (auto *container : m_graphContainers)
            {
                if (container && 
                    container->hasDataOption(graphType) && 
                    container->getCurrentDataOption() == graphType)
                {
                    container->onDataChangedInteractive(graphType, seriesLabel);
                }
            }
        }
        else
        {
            // Too soon since last update, mark as pending
            // The next call or a timer will process it
            m_pendingInteractiveUpdate[graphType] = true;
        }
    }
    else
    {
        DEBUG_OUT() << "Engine not found:" << dataSourceLabel;
    }
}

void GraphLayout::endInteractiveDrag(const GraphType &graphType)
{
    DEBUG_OUT() << "Interactive drag ended for" << graphTypeToString(graphType) << "- triggering full redraw";
    
    // Flush any pending interactive updates first
    flushPendingInteractiveUpdates(graphType);
    
    // Reset throttling timer
    m_interactiveUpdateTimers[graphType].invalidate();
    
    // Trigger full update with range recalculation for all containers
    for (auto *container : m_graphContainers)
    {
        if (container)
        {
            // Use normal onDataChanged which does full update with range recalculation
            container->onDataChanged(graphType);
        }
    }
}

void GraphLayout::flushPendingInteractiveUpdates(const GraphType &graphType)
{
    if (m_pendingInteractiveUpdate[graphType])
    {
        // Process any pending update
        auto it = m_engines.find(graphType);
        if (it != m_engines.end())
        {
            // Trigger update for all containers displaying this graph
            for (auto *container : m_graphContainers)
            {
                if (container && 
                    container->hasDataOption(graphType) && 
                    container->getCurrentDataOption() == graphType)
                {
                    // Trigger a general update to ensure latest data is displayed
                    container->onDataChanged(graphType);
                }
            }
        }
        m_pendingInteractiveUpdate[graphType] = false;
    }
}

void GraphLayout::setDataToDataSource(const GraphType &graphType, const QString &seriesLabel, const WaterfallData &data)
{
    QString dataSourceLabel = graphTypeToString(graphType);
    auto it = m_engines.find(graphType);
    if (it != m_engines.end())
    {
        // Get the specific series data from the WaterfallData object
        auto seriesData = data.getAllDataSeries(seriesLabel);
        std::vector<float> yData;
        std::vector<QDateTime> timestamps;
        
        for (const auto& pair : seriesData) {
            // Convert qreal to float for storage
            yData.push_back(static_cast<float>(pair.first));
            timestamps.push_back(pair.second);
        }
        
        it->second->setDataSeries(seriesLabel, yData, timestamps);
        DEBUG_OUT() << "Set data for" << dataSourceLabel << "series" << seriesLabel << "from WaterfallData object";

        // If data is empty, ensure graphs are properly cleared
        // Check if the input data is empty or if the engine's data is empty after setting
        bool isEmpty = seriesData.empty() || yData.empty() || timestamps.empty() || it->second->isEmpty();
        if (isEmpty)
        {
            DEBUG_OUT() << "Data is empty for" << dataSourceLabel << "series" << seriesLabel << "- triggering full redraw to clear graphs";
        }

        // Notify all containers that have this data source to update their UI
        for (auto *container : m_graphContainers)
        {
            if (container)
            {
                // If data is empty, force a full redraw to ensure graphs are cleared
                // This ensures existing graph graphics are removed when data becomes empty
                if (isEmpty)
                {
                    container->redrawWaterfallGraph(graphType);
                }
                else
                {
                    container->onDataChanged(graphType);
                }
            }
        }
    }
    else
    {
        DEBUG_OUT() << "Engine not found:" << dataSourceLabel;
    }
}

void GraphLayout::clearDataSource(const GraphType &graphType, const QString &seriesLabel)
{
    QString dataSourceLabel = graphTypeToString(graphType);
    auto it = m_engines.find(graphType);
    if (it != m_engines.end())
    {
        it->second->clearDataSeries(seriesLabel);
        DEBUG_OUT() << "Cleared data for" << dataSourceLabel << "series" << seriesLabel;

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
        DEBUG_OUT() << "Engine not found:" << dataSourceLabel;
    }
}

// Data source management

WaterfallData *GraphLayout::getDataSource(const GraphType &graphType)
{
    auto it = m_engines.find(graphType);
    return (it != m_engines.end()) ? it->second->dataMutable() : nullptr;
}

bool GraphLayout::hasDataSource(const GraphType &graphType) const
{
    return m_engines.find(graphType) != m_engines.end();
}

GraphEngine* GraphLayout::getEngine(const GraphType &graphType)
{
    auto it = m_engines.find(graphType);
    return (it != m_engines.end()) ? it->second : nullptr;
}

std::vector<GraphType> GraphLayout::getDataSourceLabels() const
{
    std::vector<GraphType> labels;
    for (const auto &pair : m_engines)
    {
        labels.push_back(pair.first);
    }
    return labels;
}

// Series-specific data source management

bool GraphLayout::hasSeriesInDataSource(const GraphType &graphType, const QString &seriesLabel) const
{
    auto it = m_engines.find(graphType);
    if (it != m_engines.end())
    {
        return it->second->hasDataSeries(seriesLabel);
    }
    return false;
}

std::vector<QString> GraphLayout::getSeriesLabelsInDataSource(const GraphType &graphType) const
{
    auto it = m_engines.find(graphType);
    if (it != m_engines.end())
    {
        return it->second->getDataSeriesLabels();
    }
    return std::vector<QString>();
}

void GraphLayout::addSeriesToDataSource(const GraphType &graphType, const QString &seriesLabel)
{
    auto it = m_engines.find(graphType);
    if (it != m_engines.end())
    {
        // Create empty vectors for the new series
        std::vector<float> emptyYData;
        std::vector<QDateTime> emptyTimestamps;
        it->second->setDataSeries(seriesLabel, emptyYData, emptyTimestamps);
        DEBUG_OUT() << "Added series" << seriesLabel << "to engine" << graphTypeToString(graphType);
    }
    else
    {
        DEBUG_OUT() << "Engine not found:" << graphTypeToString(graphType);
    }
}

void GraphLayout::removeSeriesFromDataSource(const GraphType &graphType, const QString &seriesLabel)
{
    auto it = m_engines.find(graphType);
    if (it != m_engines.end())
    {
        it->second->clearDataSeries(seriesLabel);
        DEBUG_OUT() << "Cleared series" << seriesLabel << "from engine" << graphTypeToString(graphType);
    }
    else
    {
        DEBUG_OUT() << "Engine not found:" << graphTypeToString(graphType);
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
    DEBUG_OUT() << "GraphLayout: Disconnecting external container connections";

    // Disconnect external connections to prevent duplicate connections
    // while preserving internal connections like TimelineView -> GraphContainer
    for (auto *container : m_graphContainers)
    {
        if (container)
        {
            // Disconnect IntervalChanged, TimeSelectionCreated, TimeSelectionModified, and TimeSelectionsCleared signals to preserve internal functionality
            // (time-scope propagation is now bus-driven, not signal-driven)
            container->disconnect(SIGNAL(IntervalChanged(TimeInterval)));
            container->disconnect(SIGNAL(TimeSelectionCreated(TimeSelectionSpan)));
            container->disconnect(SIGNAL(TimeSelectionModified(int, TimeSelectionSpan)));
            container->disconnect(SIGNAL(TimeSelectionsCleared()));
            DEBUG_OUT() << "GraphLayout: Disconnected external signals from container";
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
    DEBUG_OUT() << "GraphLayout: deleteInteractiveMarkers invoked";
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
    DEBUG_OUT() << "GraphLayout: Linking horizontal containers for layout type:" << static_cast<int>(m_layoutType);

    // Disconnect all existing connections first to avoid duplicates
    disconnectAllContainerConnections();

    // Interval and time-scope synchronization are now handled centrally:
    //   - intervals via GraphLayout::onContainerIntervalChanged (setTimeInterval API)
    //   - time-scope via TimeScopeBus (every container subscribes in initializeContainers)
    // Only the GPW4WH layout still needs direct interval forwarding from the
    // single timeline-bearing container to its siblings.

    switch (m_layoutType)
    {
    case LayoutType::GPW4W:
        DEBUG_OUT() << "GraphLayout: GPW4W: time-scope handled by TimeScopeBus";
        break;

    case LayoutType::GPW2WH:
        DEBUG_OUT() << "GraphLayout: GPW2WH: time-scope handled by TimeScopeBus";
        break;

    case LayoutType::GPW4WH:
        // Direct interval forwarding from container 2 (which owns the timeline) to siblings.
        connect(m_graphContainers[2], &GraphContainer::IntervalChanged,
                m_graphContainers[0], &GraphContainer::onTimeIntervalChanged);
        connect(m_graphContainers[2], &GraphContainer::IntervalChanged,
                m_graphContainers[1], &GraphContainer::onTimeIntervalChanged);
        connect(m_graphContainers[2], &GraphContainer::IntervalChanged,
                m_graphContainers[3], &GraphContainer::onTimeIntervalChanged);

        DEBUG_OUT() << "GraphLayout: Linked containers for GPW4WH layout (intervals only)";
        break;

    case LayoutType::GPW1W:
    case LayoutType::GPW2WV:
    case LayoutType::HIDDEN:
        // No horizontal linking needed for these layouts
        DEBUG_OUT() << "GraphLayout: No horizontal linking needed for layout type:" << static_cast<int>(m_layoutType);
        break;

    default:
        qWarning() << "GraphLayout: Unknown layout type for horizontal linking:" << static_cast<int>(m_layoutType);
        break;
    }
}

void GraphLayout::syncAllTimelineViews()
{
    DEBUG_OUT() << "GraphLayout: Syncing all timeline views for layout type:" << static_cast<int>(m_layoutType);
    
    // This function ensures all timeline views in the layout are properly synchronized:
    // 1. Timeline views are connected to each other for interval and scope changes
    // 2. Timeline views are connected to all visible containers (including those without timeline views)
    // 3. Internal connections (TimelineView -> its own container) are preserved
    // 4. Works for all layout types: GPW1W, GPW2WV, GPW4W, GPW2WH, GPW4WH, NOGPW2WH
    
    // Collect all visible TimelineView instances with their containers
    // Include every visible container that has a timeline view so ALL timeline views stay synced
    // (one changed applies to all, regardless of getShowTimelineView() in current layout)
    std::vector<std::pair<GraphContainer*, TimelineView*>> timelineViewPairs;
    for (auto *container : m_graphContainers)
    {
        if (container && container->isVisible())
        {
            TimelineView *timelineView = container->getTimelineView();
            if (timelineView)
            {
                timelineViewPairs.push_back({container, timelineView});
                DEBUG_OUT() << "GraphLayout: Found timeline view in container";
            }
        }
    }
    
    if (timelineViewPairs.size() <= 1)
    {
        if (timelineViewPairs.size() == 1)
        {
            DEBUG_OUT() << "GraphLayout: Only 1 timeline view found, ensuring internal connections are set up";
            // Even with 1 timeline view, ensure internal connections are properly set up.
            // Time-scope propagation is bus-driven; only interval still needs an internal connect.
            const auto &pair = timelineViewPairs[0];
            if (pair.first && pair.second)
            {
                connect(pair.second, &TimelineView::TimeIntervalChanged,
                        pair.first, &GraphContainer::onTimeIntervalChanged, Qt::UniqueConnection);
            }
        }
        else
        {
            DEBUG_OUT() << "GraphLayout: No timeline views found";
        }
        return;
    }
    
    DEBUG_OUT() << "GraphLayout: Found" << timelineViewPairs.size() << "timeline views to sync";
    
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
        
        // Disconnect AbsoluteTimeModeChanged connections to other timeline views
        for (size_t j = 0; j < timelineViewPairs.size(); ++j)
        {
            if (i != j && timelineViewPairs[j].second)
            {
                disconnect(sourceTimelineView, &TimelineView::AbsoluteTimeModeChanged,
                          timelineViewPairs[j].second, &TimelineView::setIsAbsoluteTime);
            }
        }
        
        // Time-scope is bus-driven; no per-pair TimelineView::TimeScopeChanged
        // connections to disconnect.

        // Disconnect GraphContainerInFollowModeChanged connections to other timeline views
        for (size_t j = 0; j < timelineViewPairs.size(); ++j)
        {
            if (i != j && timelineViewPairs[j].second)
            {
                disconnect(sourceTimelineView, &TimelineView::GraphContainerInFollowModeChanged,
                          timelineViewPairs[j].second, &TimelineView::onOtherContainerEnteredFollowMode);
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
                
                // Connect AbsoluteTimeModeChanged signal to setIsAbsoluteTime
                // This ensures all timeline views' abs/rel buttons stay in sync
                connect(timelineViewPairs[i].second, &TimelineView::AbsoluteTimeModeChanged,
                        timelineViewPairs[j].second, &TimelineView::setIsAbsoluteTime, Qt::UniqueConnection);
                
                // When one timeline enters follow mode (slider at y=0), switch all others to follow mode so sliders stay in sync
                connect(timelineViewPairs[i].second, &TimelineView::GraphContainerInFollowModeChanged,
                        timelineViewPairs[j].second, &TimelineView::onOtherContainerEnteredFollowMode, Qt::UniqueConnection);
            }
        }
    }
    
    // Ensure each TimelineView is connected to its own container (internal interval connection only).
    // Time-scope propagation is bus-driven; the container subscribed to TimeScopeBus in initializeContainers().
    for (const auto &pair : timelineViewPairs)
    {
        if (pair.first && pair.second)
        {
            connect(pair.second, &TimelineView::TimeIntervalChanged,
                    pair.first, &GraphContainer::onTimeIntervalChanged, Qt::UniqueConnection);
        }
    }

    DEBUG_OUT() << "GraphLayout: Timeline views synced successfully";
}

void GraphLayout::syncExternalTimelineView(TimelineView *externalTimelineView)
{
    if (!externalTimelineView)
    {
        qWarning() << "GraphLayout: Cannot sync null external timeline view";
        return;
    }
    
    DEBUG_OUT() << "GraphLayout: Syncing external timeline view with all timeline views";
    
    // Get all timeline views from graphlayout containers
    for (auto *container : m_graphContainers)
    {
        if (container && container->isVisible() && container->getShowTimelineView())
        {
            TimelineView *graphTimelineView = container->getTimelineView();
            if (graphTimelineView)
            {
                // Connect abs/rel button signals bidirectionally
                connect(externalTimelineView, &TimelineView::AbsoluteTimeModeChanged,
                        graphTimelineView, &TimelineView::setIsAbsoluteTime, Qt::UniqueConnection);
                connect(graphTimelineView, &TimelineView::AbsoluteTimeModeChanged,
                        externalTimelineView, &TimelineView::setIsAbsoluteTime, Qt::UniqueConnection);
                
                // Time-scope sync between this layout's timelines and the external timeline
                // is now handled centrally by TimeScopeBus. The external timeline can be
                // wired to it directly via getScopeBus()->subscribe(...) by the caller.

                // Sync time interval changes
                connect(externalTimelineView, &TimelineView::TimeIntervalChanged,
                        graphTimelineView, &TimelineView::setTimeLineLength, Qt::UniqueConnection);
                connect(graphTimelineView, &TimelineView::TimeIntervalChanged,
                        externalTimelineView, &TimelineView::setTimeLineLength, Qt::UniqueConnection);
                
                // When one enters follow mode (slider at y=0), switch the other to follow mode so sliders stay in sync
                connect(externalTimelineView, &TimelineView::GraphContainerInFollowModeChanged,
                        graphTimelineView, &TimelineView::onOtherContainerEnteredFollowMode, Qt::UniqueConnection);
                connect(graphTimelineView, &TimelineView::GraphContainerInFollowModeChanged,
                        externalTimelineView, &TimelineView::onOtherContainerEnteredFollowMode, Qt::UniqueConnection);
            }
        }
        
        // Time-scope updates from the external timeline are routed through TimeScopeBus.
        // External integrators are expected to publish into getScopeBus() directly.
        // Only interval is wired here for parity with the legacy behavior.
        if (container)
        {
            connect(externalTimelineView, &TimelineView::TimeIntervalChanged,
                    container, &GraphContainer::onTimeIntervalChanged, Qt::UniqueConnection);
        }
    }
    
    DEBUG_OUT() << "GraphLayout: External timeline view synced successfully";
}

void GraphLayout::onTimerTick()
{
    setCurrentTime(QTime::currentTime());
    
    // Update current navtime in sync state
    NavTimeUtils navTimeUtils;
    QDateTime currentSystemTime = QDateTime::currentDateTime();
    m_syncState.currentNavTime = navTimeUtils.covertSystemTimeToNavTime(currentSystemTime);
    m_syncState.hasCurrentNavTime = true;
}

void GraphLayout::onTimeSelectionCreated(const TimeSelectionSpan &selection)
{
    DEBUG_OUT() << "GraphLayout: Time selection created from" << selection.startTime.toString() << "to" << selection.endTime.toString();

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
            DEBUG_OUT() << "GraphLayout: Selection added to container";
        }
    }
    
    // Emit the signal for external components
    emit TimeSelectionCreated(selection);
}

void GraphLayout::onTimeSelectionModified(int index, const TimeSelectionSpan &newSpan)
{
    if (index < 0 || index >= static_cast<int>(m_syncState.timeSelections.size()))
        return;
    m_syncState.timeSelections[static_cast<size_t>(index)] = newSpan;
    GraphContainer *source = qobject_cast<GraphContainer *>(sender());
    for (auto *container : m_graphContainers) {
        if (container && container != source)
            container->setTimeSelection(index, newSpan);
    }
    emit TimeSelectionModified(index, newSpan);
}

void GraphLayout::onContainerIntervalChanged(TimeInterval interval)
{
    DEBUG_OUT() << "GraphLayout: Container interval changed to" << timeIntervalToString(interval);
    
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
            DEBUG_OUT() << "GraphLayout: Interval set on container via API";
        }
    }
}

// onContainerTimeScopeChanged removed: time-scope propagation is now centralized
// in TimeScopeBus. Containers publish via GraphContainer::onTimelineScope*() and
// every container subscribes via GraphContainer::attachScopeBus().

// void GraphLayout::onCursorTimeChanged(const QDateTime &time)
// {
//     m_syncState.cursorTime = time;
//     m_syncState.hasCursorTime = true;
// }

// ------------------------------------------------------------

void GraphLayout::propagateTimeSelectionToAllContainers(const TimeSelectionSpan &selection)
{
    // Add the selection to all visible containers
    for (auto *container : m_graphContainers)
    {
        if (container && container->isVisible())
        {
            container->addTimeSelection(selection);
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
    // Update shared sync state
    if (time.isValid())
    {
        m_syncState.cursorTime = time;
        m_syncState.hasCursorTime = true;
    }
    else
    {
        m_syncState.hasCursorTime = false;
    }

    // Propagate cursor time to all containers' timeline views
    // The source container already updated its timeline view in handleCursorTimeChanged
    for (auto *container : m_graphContainers)
    {
        if (container && container != source)
        {
            // Update timeline view crosshair timestamp in other containers
            if (container->getTimelineView())
            {
                if (time.isValid())
                {
                    container->getTimelineView()->updateCrosshairTimestampFromTime(time);
                }
                else
                {
                    container->getTimelineView()->clearCrosshairTimestamp();
                }
            }
        }
    }

    // All containers now read from sync state via timer for cursor layer
    // The cursor layer in each WaterfallGraph will automatically read from m_syncState
}

void GraphLayout::onTimeSelectionsCleared()
{
    DEBUG_OUT() << "GraphLayout: Time selections cleared by one container - clearing in all containers";
    
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
    DEBUG_OUT() << "GraphLayout: BTW manual marker placed at timestamp" << timestamp.toString() << "position" << position;
    
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
                DEBUG_OUT() << "GraphLayout: Calculated range" << range << "from X position" << position.x();
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
                // Use binary search to find closest data point (within 1 second = 1000ms)
                size_t unusedIndex;
                if (btwDataSource->findClosestDataPoint(seriesLabel, timestamp, 1000, range, unusedIndex))
                {
                    foundRange = true;
                    DEBUG_OUT() << "GraphLayout: Found range" << range << "from data at timestamp";
                    break;
                }
            }
        }
    }
    
    // If still no range found, use a default value
    if (!foundRange)
    {
        range = 50.0; // Default range value
        DEBUG_OUT() << "GraphLayout: Using default range" << range << "for BTW marker";
    }
    
    // Add magenta circle (BTW symbol) to all graphs at this timestamp
    // The range parameter is not needed - we'll find the data point at this timestamp in each graph
    addBTWSymbolToAllGraphs(timestamp, 0.0); // Range parameter is ignored, we find it from data points
}

void GraphLayout::onBTWHorizontalLinePlaced(const QUuid &lineId, const QDateTime &timestamp)
{
    // Get the source container that emitted the signal
    GraphContainer *sourceContainer = qobject_cast<GraphContainer*>(sender());
    
    DEBUG_OUT() << "GraphLayout: BTW horizontal line placed:" << lineId.toString() << "at" << timestamp.toString();
    
    // Propagate to all other containers (skip the source to avoid infinite loop)
    for (auto *container : m_graphContainers)
    {
        if (!container) continue;
        
        // Skip the source container to avoid infinite loop
        if (container == sourceContainer) continue;
        
        // Get the BTW graph from the container
        WaterfallGraph *btwGraphBase = container->getWaterfallGraph(GraphType::BTW);
        if (btwGraphBase)
        {
            BTWGraph *btwGraph = qobject_cast<BTWGraph*>(btwGraphBase);
            if (btwGraph)
            {
                // Add the horizontal line to this BTW graph
                // Use the same timestamp, color, and width as the source
                // Note: We need to get the color and width from the source line
                // For now, use default values (white, 2.0) - we could enhance this later
                btwGraph->addHorizontalLine(timestamp, Qt::white, 2.0);
                DEBUG_OUT() << "GraphLayout: Added horizontal line to container at" << timestamp.toString();
            }
        }
    }
}

void GraphLayout::onBTWHorizontalLineRemoved(const QUuid &lineId, const QDateTime &timestamp)
{
    // Get the source container that emitted the signal
    GraphContainer *sourceContainer = qobject_cast<GraphContainer*>(sender());
    
    DEBUG_OUT() << "GraphLayout: BTW horizontal line removed:" << lineId.toString() << "at" << timestamp.toString();
    
    if (!timestamp.isValid())
    {
        DEBUG_OUT() << "GraphLayout: Invalid timestamp for line removal, skipping sync";
        return;
    }
    
    // Remove lines with matching timestamp from all other containers
    for (auto *container : m_graphContainers)
    {
        if (!container) continue;
        
        // Skip the source container to avoid infinite loop
        if (container == sourceContainer) continue;
        
        // Get the BTW graph from the container
        WaterfallGraph *btwGraphBase = container->getWaterfallGraph(GraphType::BTW);
        if (btwGraphBase)
        {
            BTWGraph *btwGraph = qobject_cast<BTWGraph*>(btwGraphBase);
            if (btwGraph)
            {
                // Remove lines with matching timestamp (1ms tolerance)
                const qreal timeTolerance = 0.001; // 1ms tolerance
                int removed = btwGraph->removeHorizontalLineByTimestamp(timestamp, timeTolerance);
                if (removed > 0)
                {
                    DEBUG_OUT() << "GraphLayout: Removed" << removed << "horizontal line(s) from container at" << timestamp.toString();
                }
            }
        }
    }
}

void GraphLayout::onBTWMarkerSyncDataChanged(const BTWSyncMarkerData &markerData)
{
    // Get the source container that emitted the signal
    GraphContainer *sourceContainer = qobject_cast<GraphContainer*>(sender());
    
    // Update sync state (m_syncState is a direct object, not a pointer)
    m_syncState.addOrUpdateBTWMarker(markerData);
    
    // Propagate to all other containers
    for (auto *container : m_graphContainers)
    {
        if (!container) continue;
        
        // Skip the source container to avoid infinite loop
        if (container == sourceContainer) continue;
        
        // Call the sync slot on other containers
        container->onBTWMarkerSyncDataChanged(markerData);
    }
}

void GraphLayout::onBTWMarkerSyncDeleted(const QUuid &markerId)
{
    // Get the source container that emitted the signal
    GraphContainer *sourceContainer = qobject_cast<GraphContainer*>(sender());
    
    // Update sync state (m_syncState is a direct object, not a pointer)
    m_syncState.removeBTWMarker(markerId);
    
    // Propagate to all other containers
    for (auto *container : m_graphContainers)
    {
        if (!container) continue;
        
        // Skip the source container to avoid infinite loop
        if (container == sourceContainer) continue;
        
        // Call the sync slot on other containers
        container->onBTWMarkerSyncDeleted(markerId);
    }
}

void GraphLayout::onShadedRegionSyncAdded(const ShadedRegionSyncData &regionData)
{
    // Get the source container that emitted the signal
    GraphContainer *sourceContainer = qobject_cast<GraphContainer*>(sender());
    
    // Update sync state
    m_syncState.addOrUpdateShadedRegion(regionData);
    
    // Propagate to all other containers
    for (auto *container : m_graphContainers)
    {
        if (!container) continue;
        
        // Skip the source container to avoid infinite loop
        if (container == sourceContainer) continue;
        
        // Call the sync slot on other containers
        container->onShadedRegionSyncAdded(regionData);
    }
}

void GraphLayout::onShadedRegionSyncRemoved(const QUuid &syncId)
{
    // Get the source container that emitted the signal
    GraphContainer *sourceContainer = qobject_cast<GraphContainer*>(sender());
    
    // Update sync state
    m_syncState.removeShadedRegion(syncId);
    
    // Propagate to all other containers
    for (auto *container : m_graphContainers)
    {
        if (!container) continue;
        
        // Skip the source container to avoid infinite loop
        if (container == sourceContainer) continue;
        
        // Call the sync slot on other containers
        container->onShadedRegionSyncRemoved(syncId);
    }
}

void GraphLayout::onShadedRegionsSyncCleared()
{
    // Get the source container that emitted the signal
    GraphContainer *sourceContainer = qobject_cast<GraphContainer*>(sender());
    
    // Update sync state
    m_syncState.clearShadedRegions();
    
    // Propagate to all other containers
    for (auto *container : m_graphContainers)
    {
        if (!container) continue;
        
        // Skip the source container to avoid infinite loop
        if (container == sourceContainer) continue;
        
        // Call the sync slot on other containers
        container->onShadedRegionsSyncCleared();
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
    DEBUG_OUT() << "GraphLayout: Set chevron label 1 to:" << label << "for all visible containers";
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
    DEBUG_OUT() << "GraphLayout: Set chevron label 2 to:" << label << "for all visible containers";
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
    DEBUG_OUT() << "GraphLayout: Set chevron label 3 to:" << label << "for all visible containers";
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
        DEBUG_OUT() << "GraphLayout: Set chevron label 1 to:" << label << "for container:" << containerLabel;
    }
    else
    {
        DEBUG_OUT() << "GraphLayout: Container not found:" << containerLabel;
    }
}

void GraphLayout::setChevronLabel2(const QString &containerLabel, const QString &label)
{
    int containerIndex = getContainerIndex(containerLabel);
    if (containerIndex >= 0 && containerIndex < static_cast<int>(m_graphContainers.size()))
    {
        m_graphContainers[containerIndex]->setChevronLabel2(label);
        DEBUG_OUT() << "GraphLayout: Set chevron label 2 to:" << label << "for container:" << containerLabel;
    }
    else
    {
        DEBUG_OUT() << "GraphLayout: Container not found:" << containerLabel;
    }
}

void GraphLayout::setChevronLabel3(const QString &containerLabel, const QString &label)
{
    int containerIndex = getContainerIndex(containerLabel);
    if (containerIndex >= 0 && containerIndex < static_cast<int>(m_graphContainers.size()))
    {
        m_graphContainers[containerIndex]->setChevronLabel3(label);
        DEBUG_OUT() << "GraphLayout: Set chevron label 3 to:" << label << "for container:" << containerLabel;
    }
    else
    {
        DEBUG_OUT() << "GraphLayout: Container not found:" << containerLabel;
    }
}

void GraphLayout::addManoeuvre(const Manoeuvre &manoeuvre)
{
    // Add manoeuvre to sync state
    m_syncState.manoeuvres.push_back(manoeuvre);
    m_syncState.hasManoeuvres = true;
    
    // Propagate to all containers
    for (auto *container : m_graphContainers)
    {
        if (container)
        {
            container->setManoeuvres(&m_syncState.manoeuvres);
        }
    }
    
    DEBUG_OUT() << "GraphLayout: Added manoeuvre - startTime:" << manoeuvre.startTime.toString()
             << "endTime:" << manoeuvre.endTime.toString()
             << "Total manoeuvres:" << m_syncState.manoeuvres.size();
}

void GraphLayout::setManoeuvres(const std::vector<Manoeuvre> &manoeuvres)
{
    // Update sync state with new manoeuvres
    m_syncState.manoeuvres = manoeuvres;
    m_syncState.hasManoeuvres = !manoeuvres.empty();
    
    // Propagate to all containers
    for (auto *container : m_graphContainers)
    {
        if (container)
        {
            container->setManoeuvres(&m_syncState.manoeuvres);
        }
    }
    
    DEBUG_OUT() << "GraphLayout: Set manoeuvres - count:" << manoeuvres.size();
}

void GraphLayout::clearManoeuvres()
{
    // Clear manoeuvres from sync state
    m_syncState.manoeuvres.clear();
    m_syncState.hasManoeuvres = false;
    
    // Propagate to all containers (pass nullptr to clear)
    for (auto *container : m_graphContainers)
    {
        if (container)
        {
            container->setManoeuvres(nullptr);
        }
    }
    
    DEBUG_OUT() << "GraphLayout: Cleared all manoeuvres";
}

std::vector<Manoeuvre> GraphLayout::getManoeuvres() const
{
    return m_syncState.manoeuvres;
}

void GraphLayout::startManoeuvreDrawing(const QDateTime &startTime, int bearing, int speed, int depth)
{
    // Store the start time and parameters for the manoeuvre being drawn
    m_currentManoeuvreStartTime = startTime;
    m_currentManoeuvreBearing = bearing;
    m_currentManoeuvreSpeed = speed;
    m_currentManoeuvreDepth = depth;
    m_manoeuvreDrawingInProgress = true;
    
    // Propagate in-progress state to all containers (shows start line immediately)
    for (auto *container : m_graphContainers)
    {
        if (container)
        {
            container->setInProgressManoeuvre(startTime);
        }
    }
    
    DEBUG_OUT() << "GraphLayout: Started manoeuvre drawing - startTime:" << startTime.toString("yyyy-MM-dd hh:mm:ss")
             << "bearing:" << bearing
             << "speed:" << speed
             << "depth:" << depth;
}

void GraphLayout::endManoeuvreDrawing(const QDateTime &endTime)
{
    // Check if a manoeuvre drawing is in progress
    if (!m_manoeuvreDrawingInProgress)
    {
        qWarning() << "GraphLayout: endManoeuvreDrawing() called but no manoeuvre drawing in progress";
        return;
    }
    
    // Validate that start time is before end time
    if (!m_currentManoeuvreStartTime.isValid() || !endTime.isValid())
    {
        qWarning() << "GraphLayout: Invalid start or end time for manoeuvre";
        m_manoeuvreDrawingInProgress = false;
        return;
    }
    
    if (m_currentManoeuvreStartTime >= endTime)
    {
        qWarning() << "GraphLayout: Start time must be before end time for manoeuvre";
        m_manoeuvreDrawingInProgress = false;
        return;
    }
    
    // Clear in-progress state from all containers
    for (auto *container : m_graphContainers)
    {
        if (container)
        {
            container->clearInProgressManoeuvre();
        }
    }
    
    // Create the manoeuvre with the stored start time and parameters, and the provided end time
    Manoeuvre manoeuvre(m_currentManoeuvreStartTime, endTime, 
                        m_currentManoeuvreBearing, m_currentManoeuvreSpeed, m_currentManoeuvreDepth);
    
    // Add manoeuvre to graph layout
    addManoeuvre(manoeuvre);
    
    DEBUG_OUT() << "GraphLayout: Ended manoeuvre drawing - startTime:" << m_currentManoeuvreStartTime.toString("yyyy-MM-dd hh:mm:ss")
             << "endTime:" << endTime.toString("yyyy-MM-dd hh:mm:ss")
             << "bearing:" << m_currentManoeuvreBearing
             << "speed:" << m_currentManoeuvreSpeed
             << "depth:" << m_currentManoeuvreDepth;
    
    // Reset the drawing state
    m_manoeuvreDrawingInProgress = false;
    m_currentManoeuvreStartTime = QDateTime();
    m_currentManoeuvreBearing = 0;
    m_currentManoeuvreSpeed = 0;
    m_currentManoeuvreDepth = 0;
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
        DEBUG_OUT() << "GraphLayout: Container not found:" << containerLabel;
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
        DEBUG_OUT() << "GraphLayout: Container not found:" << containerLabel;
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
        DEBUG_OUT() << "GraphLayout: Container not found:" << containerLabel;
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
    DEBUG_OUT() << "GraphLayout: clearAllGraphs() - clearing all data, markers, and symbols from all graphs";
    
    // Clear all engines
    for (auto &pair : m_engines)
    {
        GraphEngine *engine = pair.second;
        if (engine)
        {
            // Clear all data series
            engine->clearAllDataSeries();
            
            // Clear all markers and symbols
            engine->clearRTWSymbols();
            engine->clearBTWSymbols();
            engine->clearBTWMarkers();
            engine->clearRTWRMarkers();
            
            DEBUG_OUT() << "GraphLayout: Cleared data for graph type:" << static_cast<int>(pair.first);
        }
    }
    
    // Trigger redraws on all containers
    for (auto *container : m_graphContainers)
    {
        if (container)
        {
            container->redrawWaterfallGraph();
            DEBUG_OUT() << "GraphLayout: Triggered redraw for container";
        }
    }
    
    DEBUG_OUT() << "GraphLayout: clearAllGraphs() completed";
}

void GraphLayout::clearGraph(const GraphType &graphType)
{
    DEBUG_OUT() << "GraphLayout: clearGraph() - clearing all data and forcing full redraw for graph type:" << static_cast<int>(graphType);
    
    // CRITICAL FIX: Clear all data series for this graph type first
    // This ensures that when forceFullRedraw() is called, the data source is empty
    // and the graph will actually be cleared instead of redrawing existing data
    std::vector<QString> seriesLabels = getSeriesLabelsInDataSource(graphType);
    for (const QString &seriesLabel : seriesLabels)
    {
        clearDataSource(graphType, seriesLabel);
        DEBUG_OUT() << "GraphLayout: Cleared data series" << seriesLabel << "for graph type" << static_cast<int>(graphType);
    }
    
    // Force full redraw on all containers that have this graph type
    // This ensures graphs are properly cleared when empty data is passed
    for (auto *container : m_graphContainers)
    {
        if (container)
        {
            // Get the specific graph type from the container
            WaterfallGraph *graph = container->getWaterfallGraph(graphType);
            if (graph)
            {
                // Force full redraw which will:
                // - Set render state to FULL_REDRAW
                // - Clear all graphics items
                // - Clear all caches
                // - Trigger complete redraw (but now with empty data source, so nothing will be drawn)
                graph->forceFullRedraw(QStringLiteral("graphlayout_clear_graph"));
                DEBUG_OUT() << "GraphLayout: Forced full redraw for graph type" << static_cast<int>(graphType) << "in container";
            }
        }
    }
    
    DEBUG_OUT() << "GraphLayout: clearGraph() completed for graph type:" << static_cast<int>(graphType);
}

// Marker and symbol management methods implementation

void GraphLayout::addRTWSymbol(const GraphType &graphType, const QString &symbolName, const QDateTime &timestamp, float range)
{
    auto it = m_engines.find(graphType);
    if (it != m_engines.end() && it->second)
    {
        it->second->addRTWSymbol(symbolName, timestamp, range);
        redrawGraph(graphType);
        DEBUG_OUT() << "GraphLayout: Added RTW symbol" << symbolName << "to graph type" << static_cast<int>(graphType);
    }
    else
    {
        DEBUG_OUT() << "GraphLayout: Cannot add RTW symbol - engine not found for graph type" << static_cast<int>(graphType);
    }
}

bool GraphLayout::removeRTWSymbol(const GraphType &graphType, const QString &symbolName, const QDateTime &timestamp, float range, float toleranceMs, float rangeTolerance)
{
    auto it = m_engines.find(graphType);
    if (it != m_engines.end() && it->second)
    {
        bool removed = it->second->removeRTWSymbol(symbolName, timestamp, range, toleranceMs, rangeTolerance);
        if (removed)
        {
            redrawGraph(graphType);
            DEBUG_OUT() << "GraphLayout: Removed RTW symbol" << symbolName << "from graph type" << static_cast<int>(graphType);
        }
        return removed;
    }
    else
    {
        DEBUG_OUT() << "GraphLayout: Cannot remove RTW symbol - engine not found for graph type" << static_cast<int>(graphType);
        return false;
    }
}

void GraphLayout::addBTWSymbol(const GraphType &graphType, const QString &symbolName, const QDateTime &timestamp, float range)
{
    auto it = m_engines.find(graphType);
    if (it != m_engines.end() && it->second)
    {
        it->second->addBTWSymbol(symbolName, timestamp, range);
        redrawGraph(graphType);
        DEBUG_OUT() << "GraphLayout: Added BTW symbol" << symbolName << "to graph type" << static_cast<int>(graphType);
    }
    else
    {
        DEBUG_OUT() << "GraphLayout: Cannot add BTW symbol - engine not found for graph type" << static_cast<int>(graphType);
    }
}

void GraphLayout::addBTWMarker(const GraphType &graphType, const QDateTime &timestamp, float range, float delta)
{
    auto it = m_engines.find(graphType);
    if (it != m_engines.end() && it->second)
    {
        it->second->addBTWMarker(timestamp, range, delta);
        redrawGraph(graphType);
        DEBUG_OUT() << "GraphLayout: Added BTW marker to graph type" << static_cast<int>(graphType);
        
        // Add magenta circle (BTW symbol) to all other graphs at this timestamp
        addBTWSymbolToAllGraphs(timestamp, range);
    }
    else
    {
        DEBUG_OUT() << "GraphLayout: Cannot add BTW marker - engine not found for graph type" << static_cast<int>(graphType);
    }
}

void GraphLayout::addRTWRMarker(const GraphType &graphType, const QDateTime &timestamp, float range)
{
    auto it = m_engines.find(graphType);
    if (it != m_engines.end() && it->second)
    {
        it->second->addRTWRMarker(timestamp, range);
        redrawGraph(graphType);
        DEBUG_OUT() << "GraphLayout: Added RTW R marker to graph type" << static_cast<int>(graphType);
    }
    else
    {
        DEBUG_OUT() << "GraphLayout: Cannot add RTW R marker - engine not found for graph type" << static_cast<int>(graphType);
    }
}

bool GraphLayout::removeBTWMarker(const GraphType &graphType, const QDateTime &timestamp, float range, float toleranceMs, float rangeTolerance)
{
    auto it = m_engines.find(graphType);
    if (it != m_engines.end() && it->second)
    {
        bool removed = it->second->removeBTWMarker(timestamp, range, toleranceMs, rangeTolerance);
        if (removed)
        {
            redrawGraph(graphType);
            DEBUG_OUT() << "GraphLayout: Removed BTW marker from graph type" << static_cast<int>(graphType);
        }
        return removed;
    }
    else
    {
        DEBUG_OUT() << "GraphLayout: Cannot remove BTW marker - engine not found for graph type" << static_cast<int>(graphType);
        return false;
    }
}

bool GraphLayout::removeRTWRMarker(const GraphType &graphType, const QDateTime &timestamp, float range, float toleranceMs, float rangeTolerance)
{
    auto it = m_engines.find(graphType);
    if (it != m_engines.end() && it->second)
    {
        bool removed = it->second->removeRTWRMarker(timestamp, range, toleranceMs, rangeTolerance);
        if (removed)
        {
            redrawGraph(graphType);
            DEBUG_OUT() << "GraphLayout: Removed RTW R marker from graph type" << static_cast<int>(graphType);
        }
        return removed;
    }
    else
    {
        DEBUG_OUT() << "GraphLayout: Cannot remove RTW R marker - engine not found for graph type" << static_cast<int>(graphType);
        return false;
    }
}

void GraphLayout::clearRTWSymbols(const GraphType &graphType)
{
    auto it = m_engines.find(graphType);
    if (it != m_engines.end() && it->second)
    {
        it->second->clearRTWSymbols();
        redrawGraph(graphType);
        DEBUG_OUT() << "GraphLayout: Cleared RTW symbols for graph type" << static_cast<int>(graphType);
    }
    else
    {
        DEBUG_OUT() << "GraphLayout: Cannot clear RTW symbols - data source not found for graph type" << static_cast<int>(graphType);
    }
}

void GraphLayout::clearBTWSymbols(const GraphType &graphType)
{
    auto it = m_engines.find(graphType);
    if (it != m_engines.end() && it->second)
    {
        it->second->clearBTWSymbols();
        redrawGraph(graphType);
        DEBUG_OUT() << "GraphLayout: Cleared BTW symbols for graph type" << static_cast<int>(graphType);
    }
    else
    {
        DEBUG_OUT() << "GraphLayout: Cannot clear BTW symbols - engine not found for graph type" << static_cast<int>(graphType);
    }
}

void GraphLayout::clearBTWMarkers(const GraphType &graphType)
{
    auto it = m_engines.find(graphType);
    if (it != m_engines.end() && it->second)
    {
        it->second->clearBTWMarkers();
        redrawGraph(graphType);
        DEBUG_OUT() << "GraphLayout: Cleared BTW markers for graph type" << static_cast<int>(graphType);
    }
    else
    {
        DEBUG_OUT() << "GraphLayout: Cannot clear BTW markers - engine not found for graph type" << static_cast<int>(graphType);
    }
}

void GraphLayout::clearRTWRMarkers(const GraphType &graphType)
{
    auto it = m_engines.find(graphType);
    if (it != m_engines.end() && it->second)
    {
        it->second->clearRTWRMarkers();
        redrawGraph(graphType);
        DEBUG_OUT() << "GraphLayout: Cleared RTW R markers for graph type" << static_cast<int>(graphType);
    }
    else
    {
        DEBUG_OUT() << "GraphLayout: Cannot clear RTW R markers - engine not found for graph type" << static_cast<int>(graphType);
    }
}

bool GraphLayout::addBTWManualMarker(const QDateTime &timestamp, float rangeValue, float bearingRate)
{
    // Find the first BTW graph container
    for (auto *container : m_graphContainers) {
        if (!container) continue;
        
        if (container->getCurrentDataOption() == GraphType::BTW) {
            WaterfallGraph *graph = container->getCurrentWaterfallGraph();
            BTWGraph *btwGraph = qobject_cast<BTWGraph*>(graph);
            
            if (btwGraph) {
                InteractiveGraphicsItem* marker = btwGraph->addBTWManualMarker(timestamp, rangeValue, bearingRate);
                if (marker) {
                    DEBUG_OUT() << "GraphLayout::addBTWManualMarker: Marker created successfully";
                    return true;
                } else {
                    DEBUG_OUT() << "GraphLayout::addBTWManualMarker: Failed to create marker";
                    return false;
                }
            }
        }
    }
    
    DEBUG_OUT() << "GraphLayout::addBTWManualMarker: No BTW graph found";
    return false;
}

void GraphLayout::clearBTWManualMarkers()
{
    DEBUG_OUT() << "GraphLayout: Clearing BTW manual markers (interactive overlay markers)";
    
    int markersCleared = 0;
    
    // Iterate through all containers to find BTW graphs
    for (auto *container : m_graphContainers)
    {
        if (!container)
            continue;
        
        // Get the BTW graph from the container (even if not currently displayed)
        WaterfallGraph *btwGraphBase = container->getWaterfallGraph(GraphType::BTW);
        if (btwGraphBase)
        {
            BTWGraph *btwGraph = qobject_cast<BTWGraph*>(btwGraphBase);
            if (btwGraph && btwGraph->getInteractiveOverlay())
            {
                btwGraph->getInteractiveOverlay()->clearAllMarkers();
                markersCleared++;
                DEBUG_OUT() << "GraphLayout: Cleared BTW manual markers in container";
            }
        }
    }
    
    // Redraw all graphs to ensure visual update
    redrawAllGraphs();
    
    DEBUG_OUT() << "GraphLayout: Cleared BTW manual markers from" << markersCleared << "graph(s)";
}

// ========== BTW Horizontal Line API Implementation ==========

void GraphLayout::setBTWHorizontalLineMode(const GraphType &graphType, BTWGraph::HorizontalLineMode mode)
{
    if (graphType != GraphType::BTW) {
        qWarning() << "GraphLayout: setBTWHorizontalLineMode called for non-BTW graph type";
        return;
    }
    
    const char* modeStr = (mode == BTWGraph::HorizontalLineMode::Normal) ? "Normal" :
                          (mode == BTWGraph::HorizontalLineMode::DrawLine) ? "DrawLine" : "DeleteLine";
    DEBUG_OUT() << "GraphLayout: Setting BTW horizontal line mode to" << modeStr;
    
    // Iterate through all containers to find BTW graphs
    for (auto *container : m_graphContainers)
    {
        if (!container)
            continue;
        
        // Get the BTW graph from the container
        WaterfallGraph *btwGraphBase = container->getWaterfallGraph(GraphType::BTW);
        if (btwGraphBase)
        {
            BTWGraph *btwGraph = qobject_cast<BTWGraph*>(btwGraphBase);
            if (btwGraph)
            {
                btwGraph->setHorizontalLineMode(mode);
            }
        }
    }
}

void GraphLayout::setBTWHorizontalLineMode(const GraphType &graphType, bool enabled)
{
    // Legacy boolean interface for backward compatibility
    setBTWHorizontalLineMode(graphType, enabled ? BTWGraph::HorizontalLineMode::DrawLine : BTWGraph::HorizontalLineMode::Normal);
}

QUuid GraphLayout::addBTWHorizontalLine(const GraphType &graphType, const QDateTime &timestamp, const QColor &color, qreal width)
{
    if (graphType != GraphType::BTW) {
        qWarning() << "GraphLayout: addBTWHorizontalLine called for non-BTW graph type";
        return QUuid();
    }
    
    DEBUG_OUT() << "GraphLayout: Adding BTW horizontal line at time" << timestamp.toString();
    
    QUuid lineId;
    bool lineAdded = false;
    
    // Iterate through all containers to find BTW graphs
    for (auto *container : m_graphContainers)
    {
        if (!container)
            continue;
        
        // Get the BTW graph from the container
        WaterfallGraph *btwGraphBase = container->getWaterfallGraph(GraphType::BTW);
        if (btwGraphBase)
        {
            BTWGraph *btwGraph = qobject_cast<BTWGraph*>(btwGraphBase);
            if (btwGraph)
            {
                lineId = btwGraph->addHorizontalLine(timestamp, color, width);
                lineAdded = true;
            }
        }
    }
    
    if (lineAdded) {
        // Redraw all graphs to show the line
        redrawAllGraphs();
    }
    
    return lineId;
}

QDateTime GraphLayout::getBTWHorizontalLineTimestamp(const GraphType &graphType, const QUuid &lineId) const
{
    if (graphType != GraphType::BTW) {
        qWarning() << "GraphLayout: getBTWHorizontalLineTimestamp called for non-BTW graph type";
        return QDateTime();
    }
    
    // Iterate through all containers to find BTW graphs
    for (auto *container : m_graphContainers)
    {
        if (!container)
            continue;
        
        // Get the BTW graph from the container
        WaterfallGraph *btwGraphBase = container->getWaterfallGraph(GraphType::BTW);
        if (btwGraphBase)
        {
            BTWGraph *btwGraph = qobject_cast<BTWGraph*>(btwGraphBase);
            if (btwGraph)
            {
                QDateTime timestamp = btwGraph->getHorizontalLineTimestamp(lineId);
                if (timestamp.isValid())
                {
                    return timestamp;
                }
            }
        }
    }
    
    return QDateTime(); // Return invalid QDateTime if not found
}

bool GraphLayout::removeBTWHorizontalLine(const GraphType &graphType, const QUuid &lineId)
{
    if (graphType != GraphType::BTW) {
        qWarning() << "GraphLayout: removeBTWHorizontalLine called for non-BTW graph type";
        return false;
    }
    
    DEBUG_OUT() << "GraphLayout: Removing BTW horizontal line ID" << lineId.toString();
    
    bool lineRemoved = false;
    
    // Iterate through all containers to find BTW graphs
    for (auto *container : m_graphContainers)
    {
        if (!container)
            continue;
        
        // Get the BTW graph from the container
        WaterfallGraph *btwGraphBase = container->getWaterfallGraph(GraphType::BTW);
        if (btwGraphBase)
        {
            BTWGraph *btwGraph = qobject_cast<BTWGraph*>(btwGraphBase);
            if (btwGraph)
            {
                if (btwGraph->removeHorizontalLine(lineId)) {
                    lineRemoved = true;
                }
            }
        }
    }
    
    if (lineRemoved) {
        // Redraw all graphs
        redrawAllGraphs();
    }
    
    return lineRemoved;
}

void GraphLayout::clearBTWHorizontalLines(const GraphType &graphType)
{
    if (graphType != GraphType::BTW) {
        qWarning() << "GraphLayout: clearBTWHorizontalLines called for non-BTW graph type";
        return;
    }
    
    DEBUG_OUT() << "GraphLayout: Clearing all BTW horizontal lines";
    
    int linesCleared = 0;
    
    // Iterate through all containers to find BTW graphs
    for (auto *container : m_graphContainers)
    {
        if (!container)
            continue;
        
        // Get the BTW graph from the container
        WaterfallGraph *btwGraphBase = container->getWaterfallGraph(GraphType::BTW);
        if (btwGraphBase)
        {
            BTWGraph *btwGraph = qobject_cast<BTWGraph*>(btwGraphBase);
            if (btwGraph)
            {
                btwGraph->clearHorizontalLines();
                linesCleared++;
            }
        }
    }
    
    // Redraw all graphs
    redrawAllGraphs();
    
    DEBUG_OUT() << "GraphLayout: Cleared BTW horizontal lines from" << linesCleared << "graph(s)";
}

// ========== Shaded Region API Implementation ==========

QUuid GraphLayout::addShadedRegionToAllBTW(qreal startX, qreal endX)
{
    DEBUG_OUT() << "GraphLayout: Adding shaded region to all BTW graphs - X range:" << startX << "to" << endX;
    
    QUuid syncId;
    bool firstRegion = true;
    
    // Iterate through all containers to find BTW graphs
    for (auto *container : m_graphContainers)
    {
        if (!container)
            continue;
        
        // Get the BTW graph from the container (even if not currently displayed)
        WaterfallGraph *btwGraphBase = container->getWaterfallGraph(GraphType::BTW);
        if (btwGraphBase)
        {
            BTWGraph *btwGraph = qobject_cast<BTWGraph*>(btwGraphBase);
            if (btwGraph)
            {
                if (firstRegion)
                {
                    // First BTW graph creates the region and generates the sync ID
                    int regionId = btwGraph->addShadedRegion(startX, endX, QDateTime());
                    
                    // Get the sync ID from the created region
                    // We need to access it through the internal storage
                    // For now, create a sync ID here and use it
                    syncId = QUuid::createUuid();
                    
                    DEBUG_OUT() << "GraphLayout: Created shaded region in first BTW graph, regionId:" << regionId
                             << "syncId:" << syncId.toString();
                    firstRegion = false;
                }
                else
                {
                    // Other BTW graphs receive the region via sync
                    // Since we already emitted the signal from the first graph,
                    // the sync system should handle it
                    // But for direct API calls, we create it directly
                    ShadedRegionSyncData syncData;
                    syncData.syncId = syncId;
                    syncData.startX = startX;
                    syncData.endX = endX;
                    syncData.isDeleted = false;
                    
                    if (!btwGraph->hasShadedRegionWithSyncId(syncId))
                    {
                        btwGraph->createShadedRegionFromSyncData(syncData);
                    }
                }
            }
        }
    }
    
    // Update sync state
    if (!syncId.isNull())
    {
        ShadedRegionSyncData syncData;
        syncData.syncId = syncId;
        syncData.startX = startX;
        syncData.endX = endX;
        syncData.isDeleted = false;
        m_syncState.addOrUpdateShadedRegion(syncData);
    }
    
    DEBUG_OUT() << "GraphLayout: Added shaded region to all BTW graphs, syncId:" << syncId.toString();
    return syncId;
}

bool GraphLayout::removeShadedRegionFromAllBTW(const QUuid &syncId)
{
    DEBUG_OUT() << "GraphLayout: Removing shaded region from all BTW graphs - syncId:" << syncId.toString();
    
    bool removed = false;
    
    // Iterate through all containers to find BTW graphs
    for (auto *container : m_graphContainers)
    {
        if (!container)
            continue;
        
        // Get the BTW graph from the container
        WaterfallGraph *btwGraphBase = container->getWaterfallGraph(GraphType::BTW);
        if (btwGraphBase)
        {
            BTWGraph *btwGraph = qobject_cast<BTWGraph*>(btwGraphBase);
            if (btwGraph)
            {
                if (btwGraph->deleteShadedRegionBySyncId(syncId))
                {
                    removed = true;
                    DEBUG_OUT() << "GraphLayout: Removed shaded region from BTW graph";
                }
            }
        }
    }
    
    // Update sync state
    m_syncState.removeShadedRegion(syncId);
    
    DEBUG_OUT() << "GraphLayout: Shaded region removal complete, success:" << removed;
    return removed;
}

void GraphLayout::clearAllShadedRegions()
{
    DEBUG_OUT() << "GraphLayout: Clearing all shaded regions from all BTW graphs";
    
    int regionsCleared = 0;
    
    // Iterate through all containers to find BTW graphs
    for (auto *container : m_graphContainers)
    {
        if (!container)
            continue;
        
        // Get the BTW graph from the container
        WaterfallGraph *btwGraphBase = container->getWaterfallGraph(GraphType::BTW);
        if (btwGraphBase)
        {
            BTWGraph *btwGraph = qobject_cast<BTWGraph*>(btwGraphBase);
            if (btwGraph)
            {
                btwGraph->clearShadedRegions();
                regionsCleared++;
                DEBUG_OUT() << "GraphLayout: Cleared shaded regions in BTW graph";
            }
        }
    }
    
    // Clear sync state
    m_syncState.clearShadedRegions();
    
    DEBUG_OUT() << "GraphLayout: Cleared shaded regions from" << regionsCleared << "BTW graph(s)";
}

std::vector<ShadedRegionSyncData> GraphLayout::getAllShadedRegions() const
{
    return m_syncState.getActiveShadedRegions();
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
            DEBUG_OUT() << "GraphLayout: Redrew graph type" << static_cast<int>(graphType) << "in container";
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
    DEBUG_OUT() << "GraphLayout: Redrew all graphs";
}

void GraphLayout::markTrackChanged()
{
    DEBUG_OUT() << "GraphLayout: Marking track change for all graphs in all containers";
    
    // Get all graph types
    std::vector<GraphType> allGraphTypes = getAllGraphTypes();
    
    // Iterate through all containers
    for (auto *container : m_graphContainers)
    {
        if (!container)
        {
            continue;
        }
        
        // Iterate through all graph types and mark track change on each graph
        for (GraphType graphType : allGraphTypes)
        {
            WaterfallGraph *graph = container->getWaterfallGraph(graphType);
            if (graph)
            {
                graph->markTrackChanged();
                DEBUG_OUT() << "GraphLayout: Marked track change for graph type" << static_cast<int>(graphType) << "in container";
            }
        }
    }
    
    DEBUG_OUT() << "GraphLayout: Track change marked for all graphs";
}

// Capacity management API implementation

void GraphLayout::setDataSeriesCapacity(size_t capacity)
{
    DEBUG_OUT() << "GraphLayout: Setting data series capacity to" << capacity << "for all data sources";
    
    // Iterate through all engines and set capacity for their data sources
    for (auto &pair : m_engines)
    {
        GraphEngine *engine = pair.second;
        if (engine)
        {
            WaterfallData *dataSource = engine->dataMutable();
            if (dataSource)
            {
                dataSource->setAllDataSeriesCapacity(capacity);
                DEBUG_OUT() << "GraphLayout: Set data series capacity for graph type" << static_cast<int>(pair.first);
            }
        }
    }
}

void GraphLayout::setSymbolsCapacity(size_t capacity)
{
    DEBUG_OUT() << "GraphLayout: Setting symbols capacity to" << capacity << "for all data sources";
    
    // Iterate through all engines and set capacity for their data sources
    for (auto &pair : m_engines)
    {
        GraphEngine *engine = pair.second;
        if (engine)
        {
            WaterfallData *dataSource = engine->dataMutable();
            if (dataSource)
            {
                dataSource->setRTWSymbolsCapacity(capacity);
                dataSource->setBTWSymbolsCapacity(capacity);
                DEBUG_OUT() << "GraphLayout: Set symbols capacity for graph type" << static_cast<int>(pair.first);
            }
        }
    }
}

void GraphLayout::setMarkersCapacity(size_t capacity)
{
    DEBUG_OUT() << "GraphLayout: Setting markers capacity to" << capacity << "for all data sources";
    
    // Iterate through all engines and set capacity for their data sources
    for (auto &pair : m_engines)
    {
        GraphEngine *engine = pair.second;
        if (engine)
        {
            WaterfallData *dataSource = engine->dataMutable();
            if (dataSource)
            {
                dataSource->setBTWMarkersCapacity(capacity);
                dataSource->setRTWRMarkersCapacity(capacity);
                DEBUG_OUT() << "GraphLayout: Set markers capacity for graph type" << static_cast<int>(pair.first);
            }
        }
    }
}

void GraphLayout::setRenderingCachesCapacity(size_t scatterCapacity, size_t linePathsCapacity, size_t cachedDataCapacity)
{
    DEBUG_OUT() << "GraphLayout: Setting rendering caches capacity - scatter:" << scatterCapacity 
                << "line paths:" << linePathsCapacity << "cached data:" << cachedDataCapacity;
    
    // Iterate through all containers and set capacity for their graphs
    for (auto *container : m_graphContainers)
    {
        if (container)
        {
            // Get all graph types available in this container
            std::vector<GraphType> graphTypes = {GraphType::BDW, GraphType::BRW, GraphType::BTW, 
                                                 GraphType::FDW, GraphType::FTW, GraphType::LTW, GraphType::RTW};
            
            for (GraphType graphType : graphTypes)
            {
                WaterfallGraph *graph = container->getWaterfallGraph(graphType);
                if (graph)
                {
                    graph->reserveAllRenderingCachesCapacity(scatterCapacity, linePathsCapacity, cachedDataCapacity);
                    DEBUG_OUT() << "GraphLayout: Set rendering caches capacity for graph type" << static_cast<int>(graphType);
                }
            }
        }
    }
}

void GraphLayout::setAllArraysCapacity(size_t dataSeriesCapacity, size_t symbolsCapacity, size_t markersCapacity,
                                       size_t scatterCapacity, size_t linePathsCapacity, size_t cachedDataCapacity)
{
    DEBUG_OUT() << "GraphLayout: Setting all arrays capacity - data series:" << dataSeriesCapacity
                << "symbols:" << symbolsCapacity << "markers:" << markersCapacity
                << "scatter:" << scatterCapacity << "line paths:" << linePathsCapacity 
                << "cached data:" << cachedDataCapacity;
    
    // Set capacity for all data sources
    setDataSeriesCapacity(dataSeriesCapacity);
    setSymbolsCapacity(symbolsCapacity);
    setMarkersCapacity(markersCapacity);
    
    // Set capacity for all graphs
    setRenderingCachesCapacity(scatterCapacity, linePathsCapacity, cachedDataCapacity);
    
    DEBUG_OUT() << "GraphLayout: Set all arrays capacity completed";
}

void GraphLayout::addBTWSymbolToAllGraphs(const QDateTime &timestamp, float /* unusedRange */)
{
    DEBUG_OUT() << "GraphLayout: Adding BTW symbol (magenta circle) to all graphs at timestamp" << timestamp.toString();
    
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
            DEBUG_OUT() << "GraphLayout: Skipping graph type" << static_cast<int>(graphType) << "- no data source or empty";
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
            // Use binary search to find closest data point
            qreal candidateValue;
            size_t candidateIndex;
            if (dataSource->findClosestDataPoint(seriesLabel, timestamp, closestTimeDiff, candidateValue, candidateIndex))
            {
                // Calculate actual time difference
                const std::vector<QDateTime> &timestamps = dataSource->getTimestampsSeries(seriesLabel);
                if (candidateIndex < timestamps.size()) {
                    qint64 timeDiff = qAbs(timestamps[candidateIndex].msecsTo(timestamp));
                    if (timeDiff < closestTimeDiff)
                    {
                        closestTimeDiff = timeDiff;
                        dataPointRange = candidateValue;
                        foundDataPoint = true;
                    }
                }
            }
        }
        
        if (!foundDataPoint)
        {
            DEBUG_OUT() << "GraphLayout: No data point found at timestamp" << timestamp.toString() << "in graph type" << static_cast<int>(graphType) << "- skipping";
            continue;
        }
        
        DEBUG_OUT() << "GraphLayout: Found data point at timestamp" << timestamp.toString() << "in graph type" << static_cast<int>(graphType) << "with range" << dataPointRange;
        
        // Check if symbol already exists at this timestamp (deduplication)
        // Use binary search to check symbols within a small time window (100ms tolerance)
        QDateTime checkStart = timestamp.addMSecs(-100);
        QDateTime checkEnd = timestamp.addMSecs(100);
        std::vector<BTWSymbolData> existingSymbols = dataSource->getBTWSymbolsWithinTimeRange(checkStart, checkEnd);
        bool symbolExists = false;
        for (const auto& existingSymbol : existingSymbols)
        {
            // Check if symbol name matches (time range already filtered)
            if (existingSymbol.symbolName == "MagentaCircle")
            {
                symbolExists = true;
                break;
            }
        }
        
        if (symbolExists)
        {
            DEBUG_OUT() << "GraphLayout: BTW symbol already exists in" << static_cast<int>(graphType) << "at this timestamp, skipping";
            continue;
        }
        
        // Add magenta circle symbol to this graph's data source
        // Use the range value from the data point at this timestamp (not the BTW marker's range)
        dataSource->addBTWSymbol("MagentaCircle", timestamp, dataPointRange, true); // isSynced=true for symbols added via addBTWSymbolToAllGraphs
        DEBUG_OUT() << "GraphLayout: Added BTW symbol to graph type" << static_cast<int>(graphType) << "at timestamp" << timestamp.toString() << "with range" << dataPointRange << "(from data point)";
        
        // Verify the symbol was added
        size_t symbolCount = dataSource->getBTWSymbolsCount();
        DEBUG_OUT() << "GraphLayout: Verified - graph type" << static_cast<int>(graphType) << "now has" << symbolCount << "BTW symbols";
        
        // Trigger redraw of this graph - redraw all containers that might show this graph type
        redrawGraph(graphType);
    }
    
    // Redraw all graphs once to ensure symbols appear in all containers
    redrawAllGraphs();
    
    // Emit signal for external consumers (e.g., SCWWindow)
    emit BTWSymbolAddedToAllGraphs(timestamp);
    
    DEBUG_OUT() << "GraphLayout: Finished adding BTW symbols to all graphs";
}

bool GraphLayout::addBTWSymbolToGraph(WaterfallData *dataSource, const QDateTime &timestamp, bool skipIfExists)
{
    if (!dataSource || dataSource->isEmpty())
    {
        return false;
    }
    
    // Check if symbol already exists at this timestamp (deduplication)
    if (skipIfExists)
    {
        QDateTime checkStart = timestamp.addMSecs(-100);
        QDateTime checkEnd = timestamp.addMSecs(100);
        std::vector<BTWSymbolData> existingSymbols = dataSource->getBTWSymbolsWithinTimeRange(checkStart, checkEnd);
        for (const auto& existingSymbol : existingSymbols)
        {
            if (existingSymbol.symbolName == "MagentaCircle")
            {
                return false; // Symbol already exists
            }
        }
    }
    
    // Find the data point at this timestamp
    qreal dataPointRange = 0.0;
    bool foundDataPoint = false;
    
    std::vector<QString> seriesLabels = dataSource->getDataSeriesLabels();
    const qint64 timeToleranceMs = 1000; // 1 second tolerance
    qint64 closestTimeDiff = timeToleranceMs;
    
    for (const QString &seriesLabel : seriesLabels)
    {
        qreal candidateValue;
        size_t candidateIndex;
        if (dataSource->findClosestDataPoint(seriesLabel, timestamp, closestTimeDiff, candidateValue, candidateIndex))
        {
            const std::vector<QDateTime> &timestamps = dataSource->getTimestampsSeries(seriesLabel);
            if (candidateIndex < timestamps.size()) {
                qint64 timeDiff = qAbs(timestamps[candidateIndex].msecsTo(timestamp));
                if (timeDiff < closestTimeDiff)
                {
                    closestTimeDiff = timeDiff;
                    dataPointRange = candidateValue;
                    foundDataPoint = true;
                }
            }
        }
    }
    
    if (!foundDataPoint)
    {
        return false;
    }
    
    // Add magenta circle symbol to this graph's data source
    dataSource->addBTWSymbol("MagentaCircle", timestamp, dataPointRange);
    return true;
}

void GraphLayout::addBTWSymbolsForExistingBTWMarkers()
{
    // Get BTW data source
    WaterfallData *btwDataSource = getDataSource(GraphType::BTW);
    if (!btwDataSource)
    {
        DEBUG_OUT() << "GraphLayout: No BTW data source found for batch processing";
        return;
    }
    
    // Get all existing BTW markers (one-time O(n) operation)
    std::vector<BTWMarkerData> allMarkers = btwDataSource->getBTWMarkers();
    
    if (allMarkers.empty())
    {
        DEBUG_OUT() << "GraphLayout: No existing BTW markers to process";
        return;
    }
    
    DEBUG_OUT() << "GraphLayout: Adding magenta circles for" << allMarkers.size() << "existing BTW markers (batch mode)";
    
    // Get all graph types (excluding BTW)
    std::vector<GraphType> allGraphTypes = getDataSourceLabels();
    
    int symbolsAdded = 0;
    
    // Process all markers for all graph types (batch mode - no redraws until end)
    for (GraphType graphType : allGraphTypes)
    {
        if (graphType == GraphType::BTW)
        {
            continue;
        }
        
        WaterfallData *dataSource = getDataSource(graphType);
        if (!dataSource || dataSource->isEmpty())
        {
            continue;
        }
        
        // Process all markers for this graph type
        for (const auto& marker : allMarkers)
        {
            if (addBTWSymbolToGraph(dataSource, marker.timestamp, true))
            {
                symbolsAdded++;
            }
        }
    }
    
    // ONE redraw at the end for all graphs (much more efficient than per-marker redraws)
    redrawAllGraphs();
    
    DEBUG_OUT() << "GraphLayout: Finished batch adding magenta circles - added" << symbolsAdded << "symbols across all graphs";
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