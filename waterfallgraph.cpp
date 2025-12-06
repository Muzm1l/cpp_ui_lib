#include "waterfallgraph.h"
#include "waterfalldata.h"  // For BTWSymbolData
#include <QApplication>
#include <QPointF>

/**
 * @brief Construct a new WaterfallGraph::WaterfallGraph object
 *
 * @param parent Parent widget
 * @param enableGrid Whether to enable grid display
 * @param gridDivisions Number of grid divisions
 * @param timeInterval Time interval for the waterfall display
 */
WaterfallGraph::WaterfallGraph(QWidget *parent, bool enableGrid, int gridDivisions, TimeInterval timeInterval)
    : QWidget(parent), 
    graphicsView(nullptr), 
    graphicsScene(nullptr), 
    overlayView(nullptr), 
    overlayScene(nullptr),
    cursorView(nullptr),
    cursorScene(nullptr),
    cursorUpdateTimer(nullptr),
    cursorCrosshairHorizontal(nullptr),
    cursorCrosshairVertical(nullptr),
    cursorTimeAxisLine(nullptr),
    m_cursorSyncState(nullptr),
    m_lastMousePos(QPointF()),
    m_cursorLayerEnabled(true), 
    gridEnabled(enableGrid), 
    gridDivisions(gridDivisions), 
    yMin(0.0), 
    yMax(0.0), 
    timeMin(QDateTime()), 
    timeMax(QDateTime()), 
    dataRangesValid(false), 
    rangeLimitingEnabled(true), 
    customYMin(0.0), 
    customYMax(0.0), 
    customTimeRangeEnabled(false), 
    customTimeMin(QDateTime()), 
    customTimeMax(QDateTime()), 
    timeInterval(timeInterval), 
    dataSource(nullptr), 
    isDragging(false), 
    isDrawing(false),
    crosshairHorizontal(nullptr), 
    crosshairVertical(nullptr), 
    crosshairEnabled(true), 
    timeAxisCursor(nullptr), 
    mouseSelectionEnabled(false), 
    selectionRect(nullptr), 
    autoUpdateYRange(true),
    lastNotifiedCursorTime(QDateTime()),
    lastNotifiedYPosition(-1.0),
    lastNotifiedCrosshairXPosition(-1.0),
    m_renderState(RenderState::FULL_REDRAW),
    m_rangeUpdateNeeded(false),
    m_zeroAxisValue(0.0)
{
    // Remove all margins and padding for snug fit
    setContentsMargins(0, 0, 0, 0);

    // Set black background
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::black);
    setPalette(pal);
    setAutoFillBackground(true);

    // Ensure the widget expands to fill all available space in the parent container
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Initialize scene
    graphicsScene = new QGraphicsScene(this);
    graphicsScene->setBackgroundBrush(QBrush(Qt::black));

    // Create graphics view
    graphicsView = new QGraphicsView(graphicsScene, this);
    graphicsView->setRenderHint(QPainter::Antialiasing);
    graphicsView->setDragMode(QGraphicsView::NoDrag);                   // We'll handle our own mouse events
    graphicsView->setMouseTracking(true);                               // Enable mouse tracking
    graphicsView->setAttribute(Qt::WA_TransparentForMouseEvents, true); // Make transparent to mouse events

    // Set black background for view
    graphicsView->setBackgroundBrush(QBrush(Qt::black));

    // Disable scrollbars
    graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Ensure the view fits the scene exactly
    graphicsView->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    graphicsView->setFrameStyle(QFrame::NoFrame);

    // Set size policy for graphics view to fill the widget
    graphicsView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Initialize overlay scene for interactive elements
    overlayScene = new QGraphicsScene(this);
    overlayScene->setBackgroundBrush(QBrush(Qt::transparent)); // Transparent background

    // Create overlay graphics view
    overlayView = new QGraphicsView(overlayScene, this);
    overlayView->setRenderHint(QPainter::Antialiasing);
    overlayView->setDragMode(QGraphicsView::NoDrag);                   // We'll handle our own mouse events
    overlayView->setMouseTracking(true);                               // Enable mouse tracking
    // Make overlay view transparent to mouse events so WaterfallGraph receives them
    overlayView->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    // Set transparent background for overlay view
    overlayView->setBackgroundBrush(QBrush(Qt::transparent));
    overlayView->setStyleSheet("background: transparent;"); // Additional transparency

    // Disable scrollbars for overlay
    overlayView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    overlayView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Ensure the overlay view fits the scene exactly
    overlayView->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    overlayView->setFrameStyle(QFrame::NoFrame);

    // Set size policy for overlay view to fill the widget
    overlayView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Make the overlay view completely transparent
    overlayView->setAttribute(Qt::WA_TranslucentBackground, true);

    // Set up layout to make the main graphics view fill the widget with no margins
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(graphicsView);
    setLayout(layout);

    // Position overlay view on top of the main view using absolute positioning
    overlayView->setParent(this);
    overlayView->setGeometry(QRect(0, 0, 100, 100)); // Initial size, will be resized in resizeEvent
    overlayView->raise();

    // Size policy will be set by parent container

    // Enable mouse tracking
    setMouseTracking(true);

    selectionRect = new QGraphicsRectItem();
    selectionRect->setPen(QPen(Qt::white, 2, Qt::DashLine));    // White dashed line
    selectionRect->setBrush(QBrush(QColor(255, 255, 255, 50))); // Semi-transparent white
    selectionRect->setZValue(1000);                             // Ensure it's drawn on top
    overlayScene->addItem(selectionRect);                       // Add to overlay scene

    // Setup crosshair
    setupCrosshair();

    // Setup time axis cursor
    if (overlayScene) {
        timeAxisCursor = new QGraphicsLineItem();
        timeAxisCursor->setPen(QPen(Qt::cyan, 1.5, Qt::SolidLine)); // Cyan line for time cursor
        timeAxisCursor->setZValue(999); // Just below crosshair but above other elements
        timeAxisCursor->setVisible(false);
        overlayScene->addItem(timeAxisCursor);
    }

    // Initialize cursor layer
    cursorScene = new QGraphicsScene(this);
    cursorScene->setBackgroundBrush(QBrush(Qt::transparent)); // Transparent background

    // Create cursor graphics view
    cursorView = new QGraphicsView(cursorScene, this);
    cursorView->setRenderHint(QPainter::Antialiasing);
    cursorView->setDragMode(QGraphicsView::NoDrag);
    cursorView->setMouseTracking(true);
    // Make cursor view transparent to mouse events so WaterfallGraph receives them
    cursorView->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    // Set transparent background for cursor view
    cursorView->setBackgroundBrush(QBrush(Qt::transparent));
    cursorView->setStyleSheet("background: transparent;");

    // Disable scrollbars for cursor view
    cursorView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    cursorView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Ensure the cursor view fits the scene exactly
    cursorView->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    cursorView->setFrameStyle(QFrame::NoFrame);

    // Set size policy for cursor view to fill the widget
    cursorView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Make the cursor view completely transparent
    cursorView->setAttribute(Qt::WA_TranslucentBackground, true);

    // Position cursor view on top of overlay view using absolute positioning
    cursorView->setParent(this);
    cursorView->setGeometry(QRect(0, 0, 100, 100)); // Initial size, will be resized in resizeEvent
    cursorView->raise(); // Ensure it's above overlayView

    // Create cursor graphics items
    cursorCrosshairHorizontal = new QGraphicsLineItem();
    cursorCrosshairHorizontal->setPen(QPen(Qt::cyan, 1.0, Qt::SolidLine)); // Cyan solid line to match legacy crosshair
    cursorCrosshairHorizontal->setZValue(2000); // Above overlay (overlay is 1000)
    cursorCrosshairHorizontal->setVisible(false);
    cursorScene->addItem(cursorCrosshairHorizontal);

    cursorCrosshairVertical = new QGraphicsLineItem();
    cursorCrosshairVertical->setPen(QPen(Qt::cyan, 1.0, Qt::SolidLine)); // Cyan solid line to match legacy crosshair
    cursorCrosshairVertical->setZValue(2000);
    cursorCrosshairVertical->setVisible(false);
    cursorScene->addItem(cursorCrosshairVertical);

    cursorTimeAxisLine = new QGraphicsLineItem();
    cursorTimeAxisLine->setPen(QPen(Qt::cyan, 1.5, Qt::SolidLine));
    cursorTimeAxisLine->setZValue(1999); // Just below crosshair but above overlay
    cursorTimeAxisLine->setVisible(false);
    cursorScene->addItem(cursorTimeAxisLine);

    // Create cursor update timer (60fps = ~16ms)
    cursorUpdateTimer = new QTimer(this);
    cursorUpdateTimer->setInterval(16); // 60fps
    connect(cursorUpdateTimer, &QTimer::timeout, this, &WaterfallGraph::updateCursorLayer);

    // Debug: Print initial state
    qDebug() << "WaterfallGraph constructor - mouseSelectionEnabled:" << mouseSelectionEnabled;
    qDebug() << "WaterfallGraph constructor - graphicsScene:" << graphicsScene;
    qDebug() << "WaterfallGraph constructor - graphicsView:" << graphicsView;

    // Initial setup will happen in showEvent
    qDebug() << "Constructor - Widget size:" << this->size();
}

/**
 * @brief Destroy the WaterfallGraph::WaterfallGraph object
 *
 */
WaterfallGraph::~WaterfallGraph()
{
    // Clean up crosshair items
    if (crosshairHorizontal) {
        delete crosshairHorizontal;
        crosshairHorizontal = nullptr;
    }
    if (crosshairVertical) {
        delete crosshairVertical;
        crosshairVertical = nullptr;
    }
    
    // Clean up time axis cursor
    if (timeAxisCursor) {
        delete timeAxisCursor;
        timeAxisCursor = nullptr;
    }

    // Clean up cursor layer
    if (cursorUpdateTimer) {
        cursorUpdateTimer->stop();
        delete cursorUpdateTimer;
        cursorUpdateTimer = nullptr;
    }
    if (cursorCrosshairHorizontal) {
        delete cursorCrosshairHorizontal;
        cursorCrosshairHorizontal = nullptr;
    }
    if (cursorCrosshairVertical) {
        delete cursorCrosshairVertical;
        cursorCrosshairVertical = nullptr;
    }
    if (cursorTimeAxisLine) {
        delete cursorTimeAxisLine;
        cursorTimeAxisLine = nullptr;
    }
    if (cursorScene) {
        delete cursorScene;
        cursorScene = nullptr;
    }
    if (cursorView) {
        delete cursorView;
        cursorView = nullptr;
    }
    
    // Note: graphicsView and graphicsScene are child widgets/scenes, so they will be automatically deleted by Qt's parent-child mechanism
}

/**
 * @brief Set the data source for this waterfall graph.
 *
 * @param dataSource Reference to the WaterfallData object to use
 */
void WaterfallGraph::setDataSource(WaterfallData &dataSource)
{
    this->dataSource = &dataSource;
    // New data source requires full redraw (automatically marks all series dirty)
    setRenderState(RenderState::FULL_REDRAW);
    draw(); // Trigger redraw with new data source
    qDebug() << "Data source set successfully";
}

/**
 * @brief Get the current data source.
 *
 * @return WaterfallData* Pointer to the current data source, or nullptr if not set
 */
WaterfallData *WaterfallGraph::getDataSource() const
{
    return dataSource;
}

/**
 * @brief Set the data for the graph.
 *
 * @param yData
 * @param timestamps
 */
void WaterfallGraph::setData(const QString &seriesLabel, const std::vector<qreal> &yData, const std::vector<QDateTime> &timestamps)
{
    if (!dataSource)
    {
        qDebug() << "Error: No data source set";
        return;
    }

    // Store the data using the data source
    dataSource->setDataSeries(seriesLabel, yData, timestamps);

    qDebug() << "Data set successfully. Size:" << dataSource->getDataSeriesSize(seriesLabel);

    // Mark ranges as invalid so they'll be recalculated
    dataRangesValid = false;

    // Redraw the graph with the new data
    draw();
}

/**
 * @brief Set the data for the graph using WaterfallData object.
 *
 * @param data
 */
void WaterfallGraph::setData(const WaterfallData &data)
{
    if (!dataSource)
    {
        qDebug() << "Error: No data source set";
        return;
    }

    *dataSource = data;

    qDebug() << "Data set successfully from WaterfallData object. Series labels:" << dataSource->getDataSeriesLabels();

    // Mark ranges as invalid so they'll be recalculated
    dataRangesValid = false;

    // Redraw the graph with the new data
    draw();
}

/**
 * @brief Clear all data from the graph.
 *
 */
void WaterfallGraph::clearData()
{
    if (!dataSource)
    {
        qDebug() << "Error: No data source set";
        return;
    }

    dataSource->clearData();

    qDebug() << "Data cleared successfully";

    // Redraw the graph
    draw();
}

/**
 * @brief Add a single data point to the graph.
 *
 * @param yValue
 * @param timestamp
 */
void WaterfallGraph::addDataPoint(const QString &seriesLabel, qreal yValue, const QDateTime &timestamp)
{
    if (!dataSource)
    {
        qDebug() << "Error: No data source set";
        return;
    }

    dataSource->addDataPointToSeries(seriesLabel, yValue, timestamp);

    qDebug() << "Data point added. New size:" << dataSource->getDataSeriesSize(seriesLabel);

    // Mark series as dirty and range update needed
    markSeriesDirty(seriesLabel);
    markRangeUpdateNeeded();
    dataRangesValid = false;

    // Use incremental draw instead of full redraw
    drawIncremental();
}

/**
 * @brief Add multiple data points to the graph.
 *
 * @param yValues
 * @param timestamps
 */
void WaterfallGraph::addDataPoints(const QString &seriesLabel, const std::vector<qreal> &yValues, const std::vector<QDateTime> &timestamps)
{
    if (!dataSource)
    {
        qDebug() << "Error: No data source set";
        return;
    }

    dataSource->addDataPointsToSeries(seriesLabel, yValues, timestamps);

    qDebug() << "Data points added. New size:" << dataSource->getDataSeriesSize(seriesLabel);

    // Mark series as dirty and range update needed
    markSeriesDirty(seriesLabel);
    markRangeUpdateNeeded();
    dataRangesValid = false;

    // Use incremental draw instead of full redraw
    drawIncremental();
}

/**
 * @brief Get data within specified y extents.
 *
 * @param yMin
 * @param yMax
 * @return std::vector<std::pair<qreal, QDateTime>>
 */
std::vector<std::pair<qreal, QDateTime>> WaterfallGraph::getDataWithinYExtents(const QString &seriesLabel, qreal yMin, qreal yMax) const
{
    if (!dataSource)
    {
        return std::vector<std::pair<qreal, QDateTime>>();
    }
    return dataSource->getDataSeriesWithinYExtents(seriesLabel, yMin, yMax);
}

/**
 * @brief Get data within specified time range.
 *
 * @param startTime
 * @param endTime
 * @return std::vector<std::pair<qreal, QDateTime>>
 */
std::vector<std::pair<qreal, QDateTime>> WaterfallGraph::getDataWithinTimeRange(const QString &seriesLabel, const QDateTime &startTime, const QDateTime &endTime) const
{
    if (!dataSource)
    {
        return std::vector<std::pair<qreal, QDateTime>>();
    }
    return dataSource->getDataSeriesWithinTimeRange(seriesLabel, startTime, endTime);
}

/**
 * @brief Get direct access to y data vector.
 *
 * @return const std::vector<qreal>&
 */
const std::vector<qreal> &WaterfallGraph::getYData(const QString &seriesLabel) const
{
    static const std::vector<qreal> emptyVector;
    if (!dataSource)
    {
        return emptyVector;
    }
    return dataSource->getYDataSeries(seriesLabel);
}

/**
 * @brief Get direct access to timestamps vector.
 *
 * @return const std::vector<QDateTime>&
 */
const std::vector<QDateTime> &WaterfallGraph::getTimestamps(const QString &seriesLabel) const
{
    static const std::vector<QDateTime> emptyVector;
    if (!dataSource)
    {
        return emptyVector;
    }
    return dataSource->getTimestampsSeries(seriesLabel);
}

/**
 * @brief Set the time interval for the waterfall graph.
 *
 * @param interval Time interval enum value
 */
void WaterfallGraph::setTimeInterval(TimeInterval interval)
{
    timeInterval = interval;

    // Always update time range based on new interval
    if (customTimeRangeEnabled)
    {
        // If custom time range is enabled, keep it but adjust the interval
        // The custom range takes precedence
        qDebug() << "Custom time range is enabled, keeping custom range";
    }
    else
    {
        // Update time range keeping the max time at current time
        // This ensures the graph always shows up to "now" when interval changes
        timeMax = QDateTime::currentDateTime();
        timeMin = timeMax.addMSecs(-getTimeIntervalMs());
        qDebug() << "Time range updated with interval - max set to current time. Time:" 
                 << timeMin.toString() << "to" << timeMax.toString() 
                 << "Interval:" << timeIntervalToString(interval);
    }

    // Update data ranges if we have data
    if (dataSource && !dataSource->isEmpty())
    {
        // Use combined range from all series
        auto yRange = dataSource->getCombinedYRange();
        yMin = yRange.first;
        yMax = yRange.second;
        dataRangesValid = true;
    }
    else
    {
        // Set default Y range when no data
        yMin = 0.0;
        yMax = 100.0;
        dataRangesValid = true;
    }

    // Force redraw regardless of data presence to update grid and layout
    draw();

    // Explicitly update the graphics view to ensure repaint
    if (graphicsView)
    {
        graphicsView->update();
        graphicsView->viewport()->update();
    }

    qDebug() << "Time interval set to:" << timeIntervalToString(interval);
}

/**
 * @brief Get the current time interval.
 *
 * @return TimeInterval Current time interval enum value
 */
TimeInterval WaterfallGraph::getTimeInterval() const
{
    return timeInterval;
}

/**
 * @brief Get the current time interval in milliseconds.
 *
 * @return qint64 Time interval in milliseconds
 */
qint64 WaterfallGraph::getTimeIntervalMs() const
{
    return static_cast<qint64>(timeInterval) * 60 * 1000; // Convert minutes to milliseconds
}

/**
 * @brief Enable or disable the grid display.
 *
 * @param enabled True to enable grid, false to disable
 */
void WaterfallGraph::setGridEnabled(bool enabled)
{
    if (gridEnabled != enabled)
    {
        gridEnabled = enabled;
        setRenderState(RenderState::FULL_REDRAW); // Grid change requires full redraw
        draw(); // Redraw to show/hide grid
        qDebug() << "Grid" << (enabled ? "enabled" : "disabled");
    }
}

/**
 * @brief Check if the grid is currently enabled.
 *
 * @return bool True if grid is enabled, false otherwise
 */
bool WaterfallGraph::isGridEnabled() const
{
    return gridEnabled;
}

/**
 * @brief Set the number of grid divisions.
 *
 * @param divisions Number of grid divisions (must be positive)
 */
void WaterfallGraph::setGridDivisions(int divisions)
{
    if (divisions > 0 && gridDivisions != divisions)
    {
        gridDivisions = divisions;
        if (gridEnabled)
        {
            setRenderState(RenderState::FULL_REDRAW); // Grid change requires full redraw
            draw(); // Redraw to update grid divisions
        }
        qDebug() << "Grid divisions set to:" << divisions;
    }
}

/**
 * @brief Get the current number of grid divisions.
 *
 * @return int Number of grid divisions
 */
int WaterfallGraph::getGridDivisions() const
{
    return gridDivisions;
}

/**
 * @brief Handle mouse click events.
 *
 * @param scenePos
 */
void WaterfallGraph::onMouseClick(const QPointF &scenePos)
{
    qDebug() << "Mouse clicked at scene position:" << scenePos;
    // This is a virtual function that can be overridden in derived classes
}

void WaterfallGraph::onMouseDrag(const QPointF &scenePos)
{
    qDebug() << "Mouse dragged to scene position:" << scenePos;
    // This is a virtual function that can be overridden in derived classes
}

/**
 * @brief Draw the graph.
 *
 */
void WaterfallGraph::draw()
{
    if (!graphicsScene)
        return;

    // Mark for full redraw (automatically marks all series dirty)
    setRenderState(RenderState::FULL_REDRAW);

    // Use incremental draw which will handle the full redraw state
    drawIncremental();
}

/**
 * @brief Incremental draw method that only redraws dirty series.
 *
 */
void WaterfallGraph::drawIncremental()
{
    if (!graphicsScene)
        return;

    switch (m_renderState)
    {
        case RenderState::CLEAN:
            return; // Nothing to do

        case RenderState::RANGE_UPDATE_ONLY:
            // Update ranges only
            if (dataSource && !dataSource->isEmpty())
            {
                updateDataRanges();
            }
            m_rangeUpdateNeeded = false;
            m_renderState = RenderState::CLEAN;
            break;

        case RenderState::INCREMENTAL_UPDATE:
            // Update ranges if needed
            if (m_rangeUpdateNeeded || !dataRangesValid)
            {
                if (dataSource && !dataSource->isEmpty())
                {
                    updateDataRanges();
                }
                m_rangeUpdateNeeded = false;
            }

            // Redraw only dirty series
            if (dataSource && !dataSource->isEmpty() && dataRangesValid)
            {
                for (const QString &seriesLabel : m_dirtySeries)
                {
                    if (isSeriesVisible(seriesLabel))
                    {
                        drawDataSeries(seriesLabel);
                    }
                }
            }

            m_dirtySeries.clear();
            m_renderState = RenderState::CLEAN;
            break;

        case RenderState::FULL_REDRAW:
            // Clear scene and graphics item maps
            graphicsScene->clear();
            m_seriesPathItems.clear();
            for (auto &pair : m_seriesPointItems)
            {
                pair.second.clear();
            }
            m_seriesPointItems.clear();

            // Update drawing area and grid
            setupDrawingArea();
            if (gridEnabled)
            {
                drawGrid();
            }

            // Update ranges
            if (dataSource && !dataSource->isEmpty())
            {
                updateDataRanges();
            }
            m_rangeUpdateNeeded = false;

            // Redraw all series
            if (dataSource && !dataSource->isEmpty() && dataRangesValid)
            {
                std::vector<QString> allSeries = dataSource->getDataSeriesLabels();
                for (const QString &seriesLabel : allSeries)
                {
                    if (isSeriesVisible(seriesLabel))
                    {
                        drawDataSeries(seriesLabel);
                    }
                }
            }

            m_dirtySeries.clear();
            m_renderState = RenderState::CLEAN;
            break;
    }
}

/**
 * @brief Transition to the appropriate state based on current conditions.
 *
 */
void WaterfallGraph::transitionToAppropriateState()
{
    // FULL_REDRAW takes precedence - don't downgrade from it
    if (m_renderState == RenderState::FULL_REDRAW)
    {
        return;
    }

    // If series are dirty, need incremental update
    if (!m_dirtySeries.empty())
    {
        m_renderState = RenderState::INCREMENTAL_UPDATE;
        return;
    }
    
    // Draw BTW symbols (magenta circles) if any exist in data source
    drawBTWSymbols();
    
    isDrawing = false;
}

/**
 * @brief Draw BTW symbols (magenta circles) from data source
 *
 */
void WaterfallGraph::drawBTWSymbols()
{
    // Follow the same pattern as RTW symbols - read symbols from dataSource
    if (!graphicsScene || !dataSource)
    {
        qDebug() << "WaterfallGraph: drawBTWSymbols - no graphicsScene or dataSource";
        return;
    }
    
    // Get symbols from dataSource
    std::vector<BTWSymbolData> btwSymbols = dataSource->getBTWSymbols();
    
    qDebug() << "WaterfallGraph: drawBTWSymbols - found" << btwSymbols.size() << "BTW symbols in data source";
    
    if (btwSymbols.empty())
    {
        return;
    }
    
    // Filter symbols to only include those within the visible time range
    std::vector<BTWSymbolData> visibleSymbols;
    bool timeRangeValid = timeMin.isValid() && timeMax.isValid() && timeMin <= timeMax;
    
    qDebug() << "WaterfallGraph: drawBTWSymbols - timeRangeValid:" << timeRangeValid 
             << "timeMin:" << (timeMin.isValid() ? timeMin.toString() : "invalid")
             << "timeMax:" << (timeMax.isValid() ? timeMax.toString() : "invalid");
    
    if (timeRangeValid)
    {
        for (const auto& symbolData : btwSymbols)
        {
            if (symbolData.timestamp >= timeMin && symbolData.timestamp <= timeMax)
            {
                visibleSymbols.push_back(symbolData);
            }
            else
            {
                qDebug() << "WaterfallGraph: Symbol filtered out - timestamp" << symbolData.timestamp.toString() 
                         << "not in range [" << timeMin.toString() << "," << timeMax.toString() << "]";
            }
        }
    }
    else
    {
        // If time range is not valid, show all symbols (they might be needed for initialization)
        visibleSymbols = btwSymbols;
        qDebug() << "WaterfallGraph: Time range not valid, showing all" << visibleSymbols.size() << "symbols";
    }
    
    qDebug() << "WaterfallGraph: drawBTWSymbols - drawing" << visibleSymbols.size() << "visible symbols";
    
    int symbolsDrawn = 0;
    // Draw symbols using a simple magenta circle (we'll create it inline since BTWSymbolDrawing is BTW-specific)
    for (const auto& symbolData : visibleSymbols)
    {
        // Map symbol position to screen coordinates
        QPointF screenPos = mapDataToScreen(symbolData.range, symbolData.timestamp);
        
        qDebug() << "WaterfallGraph: Symbol at timestamp" << symbolData.timestamp.toString() 
                 << "range" << symbolData.range << "mapped to screen position" << screenPos;
        
        // Check if point is within visible area
        if (!drawingArea.contains(screenPos))
        {
            qDebug() << "WaterfallGraph: Symbol filtered out - screen position" << screenPos 
                     << "not in drawing area" << drawingArea;
            continue;
        }
        
        // Draw a simple magenta circle
        QGraphicsEllipseItem *magentaCircle = new QGraphicsEllipseItem();
        qreal circleSize = 8.0; // Small circle, 8 pixels diameter
        magentaCircle->setRect(screenPos.x() - circleSize/2, screenPos.y() - circleSize/2, circleSize, circleSize);
        magentaCircle->setPen(QPen(QColor(255, 0, 255), 2)); // Magenta color
        magentaCircle->setBrush(QBrush(QColor(255, 0, 255))); // Filled magenta
        magentaCircle->setZValue(1003); // Above markers but below interactive items
        
        graphicsScene->addItem(magentaCircle);
        symbolsDrawn++;
        qDebug() << "WaterfallGraph: Drew magenta circle at" << screenPos;
    }
    
    qDebug() << "WaterfallGraph: drawBTWSymbols - drew" << symbolsDrawn << "magenta circles";

    // If only ranges need update
    if (m_rangeUpdateNeeded || !dataRangesValid)
    {
        m_renderState = RenderState::RANGE_UPDATE_ONLY;
        return;
    }

    // Otherwise clean
    m_renderState = RenderState::CLEAN;
}

/**
 * @brief Set the render state, with FULL_REDRAW taking precedence.
 *
 */
void WaterfallGraph::setRenderState(RenderState newState)
{
    // FULL_REDRAW can only be set explicitly, never downgraded
    if (m_renderState == RenderState::FULL_REDRAW && newState != RenderState::FULL_REDRAW)
    {
        return; // Don't downgrade from FULL_REDRAW
    }

    // FULL_REDRAW always supersedes
    if (newState == RenderState::FULL_REDRAW)
    {
        m_renderState = RenderState::FULL_REDRAW;
        markAllSeriesDirty();
        return;
    }

    m_renderState = newState;
}

/**
 * @brief Mark a specific series as dirty.
 *
 */
void WaterfallGraph::markSeriesDirty(const QString &seriesLabel)
{
    m_dirtySeries.insert(seriesLabel);
    transitionToAppropriateState();
}

/**
 * @brief Mark all series as dirty and set state to FULL_REDRAW.
 *
 */
void WaterfallGraph::markAllSeriesDirty()
{
    if (dataSource && !dataSource->isEmpty())
    {
        std::vector<QString> allSeries = dataSource->getDataSeriesLabels();
        for (const QString &seriesLabel : allSeries)
        {
            m_dirtySeries.insert(seriesLabel);
        }
    }
    m_renderState = RenderState::FULL_REDRAW;
}

/**
 * @brief Mark that ranges need updating.
 *
 */
void WaterfallGraph::markRangeUpdateNeeded()
{
    m_rangeUpdateNeeded = true;
    transitionToAppropriateState();
}

/**
 * @brief Update the graphics dimensions.
 *
 */
void WaterfallGraph::updateGraphicsDimensions()
{
    if (!graphicsView || !graphicsScene || !overlayView || !overlayScene)
        return;

    // Get the current size of the widget
    QSize widgetSize = this->size();

    qDebug() << "updateGraphicsDimensions - Widget size:" << widgetSize;

    // Only update if we have valid dimensions
    if (widgetSize.width() > 0 && widgetSize.height() > 0)
    {
        // Set scene rect to match widget size exactly
        QRectF newSceneRect(0, 0, widgetSize.width(), widgetSize.height());
        graphicsScene->setSceneRect(newSceneRect);
        overlayScene->setSceneRect(newSceneRect); // Also update overlay scene

        // Ensure the graphics view fits the scene exactly (no scrollbars)
        graphicsView->fitInView(newSceneRect, Qt::KeepAspectRatio);
        graphicsView->resetTransform();           // Reset any scaling
        graphicsView->setTransform(QTransform()); // Ensure 1:1 mapping

        // Ensure the overlay view also fits the scene exactly
        overlayView->fitInView(newSceneRect, Qt::KeepAspectRatio);
        overlayView->resetTransform();           // Reset any scaling
        overlayView->setTransform(QTransform()); // Ensure 1:1 mapping

        // Update the drawing area
        setupDrawingArea();

        // Redraw the scene
        draw();

        qDebug() << "Graphics dimensions updated successfully to:" << widgetSize;
        qDebug() << "Scene rect is now:" << graphicsScene->sceneRect();
    }
    else
    {
        qDebug() << "Widget size is invalid, skipping update";
    }
}

/**
 * @brief Setup the drawing area.
 *
 */
void WaterfallGraph::setupDrawingArea()
{
    // Set up the drawing area to cover the entire scene
    drawingArea = graphicsScene->sceneRect();
    qDebug() << "Drawing area set to:" << drawingArea;
}

/**
 * @brief Draw the grid.
 *
 */
void WaterfallGraph::drawGrid()
{
    if (!graphicsScene || !gridEnabled || drawingArea.isEmpty() || gridDivisions <= 0)
        return;

    QPen gridPen(Qt::white, 1, Qt::DashLine); // White grid lines for black background

    // Draw vertical grid lines (for x-axis - variable)
    double stepX = drawingArea.width() / gridDivisions;
    for (int i = 0; i <= gridDivisions; ++i)
    {
        double x = drawingArea.left() + i * stepX;
        graphicsScene->addLine(x, drawingArea.top(), x, drawingArea.bottom(), gridPen);
    }

    // Draw horizontal grid lines (for y-axis - time)
    // These represent time divisions within the fixed interval
    double stepY = drawingArea.height() / gridDivisions;
    for (int i = 0; i <= gridDivisions; ++i)
    {
        double y = drawingArea.top() + i * stepY;
        graphicsScene->addLine(drawingArea.left(), y, drawingArea.right(), y, gridPen);
    }

    // Draw border
    QPen borderPen(Qt::white, 2); // White border for black background
    graphicsScene->addRect(drawingArea, borderPen);
}

/**
 * @brief Handle mouse press events.
 *
 * @param event
 */
void WaterfallGraph::mousePressEvent(QMouseEvent *event)
{
    qDebug() << "Mouse press event - button:" << event->button() << "mouseSelectionEnabled:" << mouseSelectionEnabled;

    if (event->button() == Qt::LeftButton)
    {
        // Convert widget coordinates to scene coordinates
        QPointF scenePos = graphicsView->mapToScene(event->pos());
        qDebug() << "Scene position:" << scenePos << "drawingArea:" << drawingArea;

        // Check if the click is within the drawing area
        if (drawingArea.contains(scenePos))
        {
            // Check if we clicked on a magenta circle (BTW symbol) in graphicsScene
            if (graphicsScene) {
                QGraphicsItem *itemAtPos = graphicsScene->itemAt(scenePos, QTransform());
                if (itemAtPos) {
                    QGraphicsEllipseItem *ellipseItem = qgraphicsitem_cast<QGraphicsEllipseItem*>(itemAtPos);
                    if (ellipseItem) {
                        // Check if it's a magenta circle by checking the brush color
                        QBrush brush = ellipseItem->brush();
                        if (brush.color() == QColor(255, 0, 255)) { // Magenta color
                            // This is a magenta circle - extract timestamp and value
                            QDateTime timestamp = mapScreenToTime(scenePos.y());
                            qreal value = mapScreenXToRange(scenePos.x());
                            
                            if (timestamp.isValid()) {
                                qDebug() << "WaterfallGraph: Magenta marker clicked at timestamp:" << timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz") << "value:" << value;
                                emit markerTimestampValueChanged(timestamp, value);
                            }
                            return; // Don't process further
                        }
                    }
                }
            }
            
            // First, try to forward the mouse event to the overlay view if we clicked on an interactive item
            // This allows interactive markers (like BTW markers) to handle their own events
            // RTW R markers are in graphicsScene and will be handled in RTWGraph::onMouseClick
            if (overlayView && overlayScene) {
                QPointF overlayScenePos = overlayView->mapToScene(event->pos());
                QGraphicsItem *itemAtPos = overlayScene->itemAt(overlayScenePos, QTransform());
                // Filter out crosshair items - they should not intercept mouse events
                // Only forward if we clicked on an actual interactive item (not crosshair, not empty)
                if (itemAtPos && itemAtPos != crosshairHorizontal && itemAtPos != crosshairVertical) {
                    qDebug() << "WaterfallGraph: Found interactive item at overlay position:" << overlayScenePos << "item:" << itemAtPos;
                    // Forward the mouse event to the overlay view so the interactive item can handle it
                    QMouseEvent *overlayEvent = new QMouseEvent(event->type(), event->pos(), event->globalPos(), event->button(), event->buttons(), event->modifiers());
                    QApplication::postEvent(overlayView, overlayEvent);
                    return; // Don't process further, let the overlay handle it
                }
                // If no interactive item found, continue to onMouseClick to allow adding new markers
            }

            isDragging = true;
            lastMousePos = scenePos;

            // Start selection if mouse selection is enabled
            if (mouseSelectionEnabled)
            {
                qDebug() << "Starting selection...";
                startSelection(scenePos);
            }
            else
            {
                qDebug() << "Mouse selection is disabled";
            }

            onMouseClick(scenePos);
        }
        else
        {
            qDebug() << "Click outside drawing area";
        }
    }

    // Call parent implementation
    QWidget::mousePressEvent(event);
}

/**
 * @brief Handle mouse move events.
 *
 * @param event
 */
void WaterfallGraph::mouseMoveEvent(QMouseEvent *event)
{
    // First, try to forward the mouse event to the overlay view if we're dragging
    if (isDragging && overlayView && overlayScene) {
        QPointF overlayScenePos = overlayView->mapToScene(event->pos());
        QGraphicsItem *itemAtPos = overlayScene->itemAt(overlayScenePos, QTransform());
        // Filter out crosshair items - they should not intercept mouse events
        if (itemAtPos && itemAtPos != crosshairHorizontal && itemAtPos != crosshairVertical) {
            qDebug() << "WaterfallGraph: Forwarding mouse move to overlay item:" << itemAtPos;
            // Forward the mouse event to the overlay view
            QMouseEvent *overlayEvent = new QMouseEvent(event->type(), event->pos(), event->globalPos(), event->button(), event->buttons(), event->modifiers());
            QApplication::postEvent(overlayView, overlayEvent);
            return; // Don't process further, let the overlay handle it
        }
    }

    if (isDragging && (event->buttons() & Qt::LeftButton))
    {
        // Convert widget coordinates to scene coordinates
        QPointF scenePos = graphicsView->mapToScene(event->pos());

        // Check if the move is within the drawing area
        if (drawingArea.contains(scenePos))
        {
            // Update selection if mouse selection is enabled
            if (mouseSelectionEnabled)
            {
                updateSelection(scenePos);
            }

            onMouseDrag(scenePos);
            lastMousePos = scenePos;
        }
    }

    // Store mouse position for cursor layer (timer will handle rendering)
    m_lastMousePos = event->pos();

    // Update crosshair if enabled (legacy overlay mode)
    if (crosshairEnabled && !m_cursorLayerEnabled && overlayScene && overlayView)
    {
        QPointF scenePos = overlayView->mapToScene(event->pos());
        // Show crosshair if not already visible
        if (crosshairHorizontal && !crosshairHorizontal->isVisible())
        {
            showCrosshair();
        }
        updateCrosshair(scenePos);
    }

    // Update cursor time notification
    if (overlayView)
    {
        QPointF scenePos = overlayView->mapToScene(event->pos());
        if (drawingArea.contains(scenePos))
        {
            QDateTime cursorTime = mapScreenToTime(scenePos.y());
            notifyCursorTimeChanged(cursorTime, scenePos.y());
        }
        else
        {
            notifyCursorTimeChanged(QDateTime(), -1.0);
        }
    }

    // Call parent implementation
    QWidget::mouseMoveEvent(event);
}

/**
 * @brief Handle enter events (mouse enters widget).
 *
 * @param event
 */
void WaterfallGraph::enterEvent(QEnterEvent *event)
{
    QWidget::enterEvent(event);
    
    // Get mouse position from cursor (QEvent doesn't have pos() in Qt 5)
    m_lastMousePos = mapFromGlobal(QCursor::pos());
    
    // Enable mouse tracking when mouse enters the widget
    setMouseTracking(true);
    
    // Show crosshair when mouse enters if enabled
    if (crosshairEnabled)
    {
        if (m_cursorLayerEnabled)
        {
            cursorCrosshairHorizontal->setVisible(true);
            cursorCrosshairVertical->setVisible(true);
        }
        else
        {
            showCrosshair();
        }
    }
    
    // Ensure cursor layer timer is running
    if (m_cursorLayerEnabled && !cursorUpdateTimer->isActive())
    {
        cursorUpdateTimer->start();
    }
    
    qDebug() << "Mouse entered WaterfallGraph widget";
}

/**
 * @brief Handle leave events (mouse leaves widget).
 *
 * @param event
 */
void WaterfallGraph::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    
    // Clear any ongoing selection when mouse leaves
    if (mouseSelectionEnabled)
    {
        clearSelection();
    }
    
    // Hide crosshair when mouse leaves (but keep time axis cursor if valid)
    if (crosshairEnabled)
    {
        if (m_cursorLayerEnabled)
        {
            cursorCrosshairHorizontal->setVisible(false);
            cursorCrosshairVertical->setVisible(false);
        }
        else
        {
            hideCrosshair();
        }
    }
    
    // Clear mouse position
    m_lastMousePos = QPointF();
    
    // Notify cursor time cleared
    notifyCursorTimeChanged(QDateTime());
    
    qDebug() << "Mouse left WaterfallGraph widget";
}

/**
 * @brief Handle mouse release events.
 *
 * @param event
 */
void WaterfallGraph::mouseReleaseEvent(QMouseEvent *event)
{
    // First, try to forward the mouse event to the overlay view if we're dragging
    if (isDragging && overlayView && overlayScene) {
        QPointF overlayScenePos = overlayView->mapToScene(event->pos());
        QGraphicsItem *itemAtPos = overlayScene->itemAt(overlayScenePos, QTransform());
        if (itemAtPos) {
            qDebug() << "WaterfallGraph: Forwarding mouse release to overlay item:" << itemAtPos;
            // Forward the mouse event to the overlay view
            QMouseEvent *overlayEvent = new QMouseEvent(event->type(), event->pos(), event->globalPos(), event->button(), event->buttons(), event->modifiers());
            QApplication::postEvent(overlayView, overlayEvent);
        }
    }

    if (event->button() == Qt::LeftButton)
    {
        // End selection if mouse selection is enabled
        if (mouseSelectionEnabled)
        {
            endSelection();
        }

        isDragging = false;
    }

    // Call parent implementation
    QWidget::mouseReleaseEvent(event);
}

/**
 * @brief Handle resize events.
 *
 * @param event
 */
void WaterfallGraph::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    // Ensure graphics view fits the widget exactly
    if (graphicsView)
    {
        graphicsView->resize(event->size());
    }

    // Ensure overlay view also fits the widget exactly and is positioned on top
    if (overlayView)
    {
        overlayView->setGeometry(QRect(0, 0, event->size().width(), event->size().height()));
        overlayView->raise();
    }

    // Ensure cursor view also fits the widget exactly and is positioned on top
    if (cursorView)
    {
        cursorView->setGeometry(QRect(0, 0, event->size().width(), event->size().height()));
        cursorView->raise(); // Ensure it's above overlayView
    }

    // Update cursor scene rect to match widget dimensions
    if (cursorScene)
    {
        cursorScene->setSceneRect(0, 0, event->size().width(), event->size().height());
    }

    // Update graphics dimensions when the widget is resized
    updateGraphicsDimensions();

    qDebug() << "Resize event - New size:" << size();
}

/**
 * @brief Handle show events.
 *
 * @param event
 */
void WaterfallGraph::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    // This is called when the widget becomes visible
    qDebug() << "showEvent - Widget size:" << this->size();
    qDebug() << "showEvent - Graphics view size:" << graphicsView->size();
    qDebug() << "showEvent - crosshairEnabled:" << crosshairEnabled;

    // Ensure graphics view fits the widget exactly
    if (graphicsView)
    {
        graphicsView->resize(this->size());
    }

    // Ensure overlay view also fits the widget exactly and is positioned on top
    if (overlayView)
    {
        overlayView->setGeometry(QRect(0, 0, this->size().width(), this->size().height()));
        overlayView->raise();
        overlayView->show();
        overlayView->update(); // Force a repaint
        qDebug() << "showEvent - Overlay view geometry:" << overlayView->geometry() << "visible:" << overlayView->isVisible();
    }

    // Ensure cursor view also fits the widget exactly and is positioned on top
    if (cursorView)
    {
        cursorView->setGeometry(QRect(0, 0, this->size().width(), this->size().height()));
        cursorView->raise(); // Ensure it's above overlayView
        cursorView->show();
        cursorView->update();
        qDebug() << "showEvent - Cursor view geometry:" << cursorView->geometry() << "visible:" << cursorView->isVisible();
    }

    // Update cursor scene rect to match widget dimensions
    if (cursorScene)
    {
        cursorScene->setSceneRect(0, 0, this->size().width(), this->size().height());
    }

    // Start cursor update timer if cursor layer is enabled
    if (m_cursorLayerEnabled && !cursorUpdateTimer->isActive())
    {
        cursorUpdateTimer->start();
    }

    // Update graphics dimensions now that we're visible
    updateGraphicsDimensions();
}

/**
 * @brief Update data ranges from the waterfall data.
 *
 */
void WaterfallGraph::updateDataRanges()
{
    if (!dataSource || dataSource->isEmpty())
    {
        dataRangesValid = false;
        return;
    }

    auto yRange = dataSource->getCombinedYRange();
    qreal dataYMin = yRange.first;
    qreal dataYMax = yRange.second;

    if (autoUpdateYRange)
    {
        // Auto-update mode: existing implementation should run
        if (rangeLimitingEnabled)
        {
            // Apply range limiting: use the intersection of custom range and data range
            // Custom range acts as bounds, but we never exceed the actual data range
            yMin = qMax(customYMin, dataYMin); // Take the larger of custom min and data min
            yMax = qMin(customYMax, dataYMax); // Take the smaller of custom max and data max

            // Ensure min < max
            if (yMin >= yMax)
            {
                // If custom range is invalid or doesn't overlap with data, use data range
                yMin = dataYMin;
                yMax = dataYMax;
                qDebug() << "Warning: Custom range doesn't overlap with data range, using data range";
                qDebug() << "Custom range:" << customYMin << "to" << customYMax;
                qDebug() << "Data range:" << dataYMin << "to" << dataYMax;
            }
        }
        else
        {
            // Use data range directly
            yMin = dataYMin;
            yMax = dataYMax;
        }
    }
    else
    {
        // Manual mode: range is always locked to the custom min and max
        yMin = customYMin;
        yMax = customYMax;

        // Ensure min < max
        if (yMin >= yMax)
        {
            qDebug() << "Warning: Custom range is invalid (min >= max), using data range";
            qDebug() << "Custom range:" << customYMin << "to" << customYMax;
            yMin = dataYMin;
            yMax = dataYMax;
        }
    }

    // Set time range based on data or custom range
    if (customTimeRangeEnabled)
    {
        // Use custom time range
        timeMin = customTimeMin;
        timeMax = customTimeMax;
    }
    else
    {
        // Set time range based on data timestamps
        setTimeRangeFromData();
    }

    dataRangesValid = true;

    qDebug() << "Data ranges updated - Y:" << yMin << "to" << yMax
             << "Time:" << timeMin.toString() << "to" << timeMax.toString()
             << "Interval:" << timeIntervalToString(timeInterval)
             << "Auto-update:" << (autoUpdateYRange ? "enabled" : "disabled")
             << "Range limiting:" << (rangeLimitingEnabled ? "enabled" : "disabled");
}

/**
 * @brief Map data coordinates to screen coordinates.
 *
 * @param yValue
 * @param timestamp
 * @return QPointF
 */
QPointF WaterfallGraph::mapDataToScreen(qreal yValue, const QDateTime &timestamp) const
{
    if (!dataRangesValid || drawingArea.isEmpty())
    {
        return QPointF(0, 0);
    }

    // Map y-value to x-coordinate (horizontal position)
    qreal x = drawingArea.left() + ((yValue - yMin) / (yMax - yMin)) * drawingArea.width();

    // Map timestamp to y-coordinate (vertical position, top to bottom)
    // Use fixed time interval instead of data range
    qint64 timeOffset = timestamp.msecsTo(timeMax); // Time from current time (top) to data point
    qreal y = drawingArea.top() + (timeOffset / (qreal)getTimeIntervalMs()) * drawingArea.height();

    return QPointF(x, y);
}

/**
 * @brief Map screen X coordinate to data range value (inverse of mapDataToScreen X mapping)
 *
 * @param xPos Screen X position
 * @return qreal Range value, or 0.0 if invalid
 */
qreal WaterfallGraph::mapScreenXToRange(qreal xPos) const
{
    if (!dataRangesValid || drawingArea.isEmpty() || (yMax - yMin) <= 0.0)
    {
        return 0.0;
    }
    
    // Reverse the X mapping: x = drawingArea.left() + ((yValue - yMin) / (yMax - yMin)) * drawingArea.width()
    // So: yValue = yMin + ((x - drawingArea.left()) / drawingArea.width()) * (yMax - yMin)
    qreal normalizedX = (xPos - drawingArea.left()) / drawingArea.width();
    normalizedX = qMax(0.0, qMin(1.0, normalizedX)); // Clamp to [0,1]
    
    qreal range = yMin + normalizedX * (yMax - yMin);
    return range;
}

/**
 * @brief Draw the data line from top to bottom.
 *
 */
void WaterfallGraph::drawDataLine(const QString &seriesLabel, bool plotPoints)
{
    if (!graphicsScene || !dataSource || dataSource->isEmpty() || !dataRangesValid)
    {
        return;
    }

    const auto &yData = dataSource->getYDataSeries(seriesLabel);
    const auto &timestamps = dataSource->getTimestampsSeries(seriesLabel);

    // Filter data points to only include those within the current time range
    std::vector<std::pair<qreal, QDateTime>> visibleData;
    for (size_t i = 0; i < yData.size(); ++i)
    {
        if (timestamps[i] >= timeMin && timestamps[i] <= timeMax)
        {
            visibleData.push_back({yData[i], timestamps[i]});
        }
    }

    if (visibleData.empty())
    {
        qDebug() << "No data points within current time range";
        return;
    }

    if (visibleData.size() < 2)
    {
        // Draw a single point if we only have one data point
        QPointF screenPoint = mapDataToScreen(visibleData[0].first, visibleData[0].second);
        QPen pointPen(Qt::green, 0); // No stroke (width 0)
        graphicsScene->addEllipse(screenPoint.x() - 2, screenPoint.y() - 2, 4, 4, pointPen);
        qDebug() << "Data line drawn with 1 visible point";
        return;
    }

    // Create a path for the line
    QPainterPath path;
    QPointF firstPoint = mapDataToScreen(visibleData[0].first, visibleData[0].second);
    path.moveTo(firstPoint);

    // Add lines connecting all visible data points
    for (size_t i = 1; i < visibleData.size(); ++i)
    {
        QPointF point = mapDataToScreen(visibleData[i].first, visibleData[i].second);
        path.lineTo(point);
    }

    // Draw the line
    QColor seriesColor = getSeriesColor(seriesLabel);
    QPen linePen(seriesColor, 2);
    graphicsScene->addPath(path, linePen);

    // Draw data points if enabled
    if (plotPoints)
    {
        // Draw data points
        QPen pointPen(seriesColor, 0); // No stroke (width 0)
        for (size_t i = 0; i < visibleData.size(); ++i)
        {
            QPointF point = mapDataToScreen(visibleData[i].first, visibleData[i].second);
            graphicsScene->addEllipse(point.x() - 1, point.y() - 1, 2, 2, pointPen);
        }
    }

    qDebug() << "Data line drawn for series" << seriesLabel << "with" << visibleData.size() << "visible points out of" << yData.size() << "total points";
}

// Mouse selection functionality implementation

void WaterfallGraph::setMouseSelectionEnabled(bool enabled)
{
    mouseSelectionEnabled = enabled;
    if (!enabled)
    {
        clearSelection();
    }
    qDebug() << "Mouse selection" << (enabled ? "enabled" : "disabled");
}

bool WaterfallGraph::isMouseSelectionEnabled() const
{
    return mouseSelectionEnabled;
}

void WaterfallGraph::startSelection(const QPointF &scenePos)
{
    qDebug() << "startSelection called with scenePos:" << scenePos;
    qDebug() << "graphicsScene:" << graphicsScene;

    selectionStartPos = scenePos;
    selectionEndPos = scenePos;

    qDebug() << "Creating new selection rectangle";

    overlayScene->addItem(selectionRect);

    // Initialize with a small rectangle at the start position
    selectionRect->setRect(scenePos.x() - 1, scenePos.y() - 1, 2, 2);

    qDebug() << "Selection rectangle created and added to scene. Rect:" << selectionRect->rect();
    qDebug() << "Selection started at:" << scenePos;
}

void WaterfallGraph::updateSelection(const QPointF &scenePos)
{
    if (!selectionRect || !dataSource || dataSource->isEmpty())
        return;

    selectionEndPos = scenePos;

    // Update rectangle bounds
    QRectF rect;
    rect.setLeft(qMin(selectionStartPos.x(), selectionEndPos.x()));
    rect.setRight(qMax(selectionStartPos.x(), selectionEndPos.x()));
    rect.setTop(qMin(selectionStartPos.y(), selectionEndPos.y()));
    rect.setBottom(qMax(selectionStartPos.y(), selectionEndPos.y()));

    // Clamp to drawing area
    rect = rect.intersected(drawingArea);

    // Additional validation: ensure selection is within valid time range
    if (dataSource && !dataSource->isEmpty())
    {
        // Convert Y coordinates to times to validate
        QDateTime topTime = mapScreenToTime(rect.top());
        QDateTime bottomTime = mapScreenToTime(rect.bottom());

        // If either time is invalid, clamp the rectangle to valid bounds
        if (!topTime.isValid() || !bottomTime.isValid())
        {
            // Clamp to the drawing area bounds
            rect.setTop(qMax(rect.top(), drawingArea.top()));
            rect.setBottom(qMin(rect.bottom(), drawingArea.bottom()));
        }
    }

    selectionRect->setRect(rect);
}

void WaterfallGraph::endSelection()
{
    if (!selectionRect || !dataSource || dataSource->isEmpty())
    {
        qDebug() << "endSelection: No valid selection or data source";
        clearSelection();
        return;
    }

    // Calculate the time range of the selection
    // Find min and max Y positions (min Y = later time, max Y = earlier time)
    qreal minY = qMin(selectionStartPos.y(), selectionEndPos.y());
    qreal maxY = qMax(selectionStartPos.y(), selectionEndPos.y());

    // Map to time: minY (bottom) = later time, maxY (top) = earlier time
    QDateTime startTime = mapScreenToTime(maxY); // Earlier time (top of selection)
    QDateTime endTime = mapScreenToTime(minY);   // Later time (bottom of selection)

    qDebug() << "Selection Y range: minY=" << minY << "maxY=" << maxY;
    qDebug() << "Time range: start=" << startTime.toString() << "end=" << endTime.toString();

    // Validate that both times are valid
    if (startTime.isValid() && endTime.isValid() && startTime != endTime)
    {
        // Ensure start time is before end time
        if (startTime > endTime)
        {
            QDateTime temp = startTime;
            startTime = endTime;
            endTime = temp;
        }

        TimeSelectionSpan selection(startTime, endTime);
        emit SelectionCreated(selection);
        qDebug() << "Selection created:" << startTime.toString() << "to" << endTime.toString();
    }
    else
    {
        qDebug() << "Invalid selection times - start:" << startTime.toString()
                 << "end:" << endTime.toString() << "or times are equal";
    }

    // Clear the visual selection immediately on mouse release
    clearSelection();
}

void WaterfallGraph::clearSelection()
{
    if (selectionRect)
    {
        overlayScene->removeItem(selectionRect);
    }
}

QDateTime WaterfallGraph::mapScreenToTime(qreal yPos) const
{
    if (!dataRangesValid || drawingArea.isEmpty() || !dataSource || dataSource->isEmpty())
    {
        qDebug() << "mapScreenToTime: Invalid conditions - dataRangesValid:" << dataRangesValid
                 << "drawingArea.isEmpty:" << drawingArea.isEmpty()
                 << "dataSource:" << (dataSource ? "exists" : "null")
                 << "dataSource->isEmpty:" << (dataSource ? dataSource->isEmpty() : true);
        return QDateTime();
    }

    // Map y-coordinate to time
    // yPos is from top (current time) to bottom (past time)
    qreal normalizedY = (yPos - drawingArea.top()) / drawingArea.height();
    normalizedY = qMax(0.0, qMin(1.0, normalizedY)); // Clamp to [0,1]

    // Calculate time offset from current time (top of graph)
    qint64 timeOffsetMs = static_cast<qint64>(normalizedY * getTimeIntervalMs());

    // Convert to QTime using the data source's time range
    QDateTime selectionTime = timeMax.addMSecs(-timeOffsetMs);

    return selectionTime;
}

void WaterfallGraph::testSelectionRectangle()
{
    qDebug() << "testSelectionRectangle called";
    if (!graphicsScene)
    {
        qDebug() << "Graphics scene is null!";
        return;
    }

    // Create a test rectangle
    QGraphicsRectItem *testRect = new QGraphicsRectItem(100, 100, 200, 100);
    testRect->setPen(QPen(Qt::white, 2, Qt::DashLine));
    testRect->setBrush(QBrush(QColor(255, 255, 255, 50)));
    testRect->setZValue(1000);
    graphicsScene->addItem(testRect);

    qDebug() << "Test selection rectangle added to scene";
}

// Range limiting methods implementation

/**
 * @brief Enable or disable range limiting functionality.
 *
 * @param enabled True to enable range limiting, false to disable
 */
void WaterfallGraph::setRangeLimitingEnabled(bool enabled)
{
    if (rangeLimitingEnabled != enabled)
    {
        rangeLimitingEnabled = enabled;

        // Update data ranges and redraw if we have data
        if (dataSource && !dataSource->isEmpty())
        {
            updateDataRanges();
            draw();
        }

        qDebug() << "Range limiting" << (enabled ? "enabled" : "disabled");
    }
}

/**
 * @brief Check if range limiting is currently enabled.
 *
 * @return bool True if range limiting is enabled, false otherwise
 */
bool WaterfallGraph::isRangeLimitingEnabled() const
{
    return rangeLimitingEnabled;
}

/**
 * @brief Set custom Y range for limiting the display range.
 *
 * @param yMin Minimum Y value for the custom range
 * @param yMax Maximum Y value for the custom range
 */
void WaterfallGraph::setCustomYRange(const qreal yMin, const qreal yMax)
{
    // Validate the range
    if (yMin >= yMax)
    {
        qDebug() << "Error: Invalid custom Y range - min must be less than max";
        return;
    }

    // Check if range changed significantly (more than 10% difference)
    bool significantChange = (qAbs(customYMin - yMin) > (customYMax - customYMin) * 0.1) ||
                             (qAbs(customYMax - yMax) > (customYMax - customYMin) * 0.1);

    customYMin = yMin;
    customYMax = yMax;

    // Always update Y range immediately when custom range is set
    updateYRange();

    // Y range change significantly requires full redraw, otherwise just range update
    if (significantChange)
    {
        setRenderState(RenderState::FULL_REDRAW);
    }
    else
    {
        markRangeUpdateNeeded();
    }

    // Force redraw to show new range
    draw();

    qDebug() << "Custom Y range set to:" << yMin << "to" << yMax;
}

/**
 * @brief Get the current custom Y range values.
 *
 * @return std::pair<qreal,qreal> The current custom Y range values
 */
std::pair<qreal, qreal> WaterfallGraph::getCustomYRange() const
{
    return std::make_pair(customYMin, customYMax);
}

/**
 * @brief Update the time range and redraw the graph.
 *
 */
void WaterfallGraph::updateTimeRange()
{
    if (customTimeRangeEnabled)
    {
        // Use custom time range
        timeMin = customTimeMin;
        timeMax = customTimeMax;
        qDebug() << "Time range updated using custom range - Time:" << timeMin.toString() << "to" << timeMax.toString();
    }
    else
    {
        // Update time range based on data
        setTimeRangeFromData();
        qDebug() << "Time range updated from data - Time:" << timeMin.toString() << "to" << timeMax.toString();
    }

    // Update data ranges if we have data
    if (dataSource && !dataSource->isEmpty())
    {
        updateDataRanges();
    }

    // Force redraw to show only data within the new time range
    draw();
}

/**
 * @brief Unset the custom Y range and revert to using the data's min/max range.
 *
 */
void WaterfallGraph::unsetCustomYRange()
{
    customYMin = 0.0;
    customYMax = 0.0;

    // Update data ranges and redraw if range limiting is enabled and we have data
    if (rangeLimitingEnabled && dataSource && !dataSource->isEmpty())
    {
        updateDataRanges();
        draw();
    }

    qDebug() << "Custom Y range unset, reverting to data range";
}

/**
 * @brief Draw a point as a small dot at the specified position.
 *
 * @param position The position to draw the point at
 * @param color The color of the point (default: white)
 * @param size The size of the point (default: 2.0)
 */
void WaterfallGraph::drawPoint(const QPointF &position, const QColor &color, qreal size)
{
    if (!graphicsScene)
        return;

    // Create a small circle for the point
    QGraphicsEllipseItem *point = new QGraphicsEllipseItem();
    point->setRect(position.x() - size / 2, position.y() - size / 2, size, size);
    point->setPen(QPen(color, 0)); // No stroke (width 0)
    point->setBrush(QBrush(color));
    point->setZValue(100); // Draw on top of grid but below selection

    graphicsScene->addItem(point);
}

/**
 * @brief Draw a dashed white translucent axis line between two points.
 *
 * @param startPos The starting position of the line
 * @param endPos The ending position of the line
 * @param color The color of the line (default: white translucent)
 */
void WaterfallGraph::drawAxisLine(const QPointF &startPos, const QPointF &endPos, const QColor &color)
{
    if (!graphicsScene)
        return;

    // Create a dashed line
    QGraphicsLineItem *line = new QGraphicsLineItem();
    line->setLine(startPos.x(), startPos.y(), endPos.x(), endPos.y());
    line->setPen(QPen(color, 1, Qt::DashLine));
    line->setZValue(50); // Draw above grid but below data points

    graphicsScene->addItem(line);
}

/**
 * @brief Draw a character label at the specified position.
 *
 * @param text The text to display
 * @param position The position to draw the text at
 * @param color The color of the text (default: white)
 * @param fontSize The font size (default: 12)
 */
void WaterfallGraph::drawCharacterLabel(const QString &text, const QPointF &position, const QColor &color, int fontSize)
{
    if (!graphicsScene)
        return;

    // Create a text item
    QGraphicsTextItem *textItem = new QGraphicsTextItem();
    textItem->setPlainText(text);
    textItem->setPos(position);

    // Set font and color
    QFont font;
    font.setPointSize(fontSize);
    textItem->setFont(font);
    textItem->setDefaultTextColor(color);
    textItem->setZValue(200); // Draw on top of everything

    graphicsScene->addItem(textItem);
}

/**
 * @brief Draw a solid triangle with square outline marker at the specified position.
 *
 * @param position The position to draw the marker at
 * @param fillColor The fill color of the triangle (default: red)
 * @param outlineColor The outline color (default: black)
 * @param size The size of the marker (default: 8.0)
 */
void WaterfallGraph::drawTriangleMarker(const QPointF &position, const QColor &fillColor, const QColor &outlineColor, qreal size)
{
    if (!graphicsScene)
        return;

    // Create triangle polygon (pointing up)
    QPolygonF triangle;
    triangle << QPointF(position.x(), position.y() - size / 2)             // Top point
             << QPointF(position.x() - size / 2, position.y() + size / 2)  // Bottom left
             << QPointF(position.x() + size / 2, position.y() + size / 2); // Bottom right

    // Create the triangle polygon item
    QGraphicsPolygonItem *triangleItem = new QGraphicsPolygonItem(triangle);
    triangleItem->setPen(QPen(outlineColor, 2));
    triangleItem->setBrush(QBrush(fillColor));
    triangleItem->setZValue(150); // Draw above data points but below text

    graphicsScene->addItem(triangleItem);

    // Create square outline around the triangle
    QGraphicsRectItem *squareOutline = new QGraphicsRectItem();
    squareOutline->setRect(position.x() - size / 2, position.y() - size / 2, size, size);
    squareOutline->setPen(QPen(outlineColor, 2));
    squareOutline->setBrush(QBrush(Qt::transparent));
    squareOutline->setZValue(149); // Draw just below the triangle

    graphicsScene->addItem(squareOutline);
}

/**
 * @brief Draw a scatterplot for the default data series.
 *
 * @param pointColor The color of the scatterplot points (default: white)
 * @param pointSize The size of the scatterplot points (default: 3.0)
 * @param outlineColor The outline color of the scatterplot points (default: black)
 */
void WaterfallGraph::drawScatterplot(const QString &seriesLabel, const QColor &pointColor, qreal pointSize, const QColor &outlineColor)
{
    if (!graphicsScene || !dataSource)
        return;

    // Get the default data series
    const std::vector<qreal> &yData = dataSource->getYDataSeries(seriesLabel);
    const std::vector<QDateTime> &timestamps = dataSource->getTimestampsSeries(seriesLabel);

    if (yData.empty() || timestamps.empty())
    {
        qDebug() << "No data available for default scatterplot";
        return;
    }

    if (yData.size() != timestamps.size())
    {
        qDebug() << "Data size mismatch for default scatterplot";
        return;
    }

    // Filter data points to only include those within the current time range
    std::vector<std::pair<qreal, QDateTime>> visibleData;
    for (size_t i = 0; i < yData.size(); ++i)
    {
        if (timestamps[i] >= timeMin && timestamps[i] <= timeMax)
        {
            visibleData.push_back({yData[i], timestamps[i]});
        }
    }

    if (visibleData.empty())
    {
        qDebug() << "No data points within current time range for default scatterplot";
        return;
    }

    // Draw scatterplot points
    for (const auto &dataPoint : visibleData)
    {
        QPointF screenPoint = mapDataToScreen(dataPoint.first, dataPoint.second);

        // Create a circle for the scatterplot point
        QGraphicsEllipseItem *point = new QGraphicsEllipseItem();
        point->setRect(screenPoint.x() - pointSize / 2, screenPoint.y() - pointSize / 2, pointSize, pointSize);
        point->setPen(QPen(outlineColor, 0)); // No stroke (width 0)
        point->setBrush(QBrush(pointColor));
        point->setZValue(120); // Draw above data lines but below markers

        graphicsScene->addItem(point);
    }

    qDebug() << "Default scatterplot drawn with" << visibleData.size() << "points";
}

/**
 * @brief Draw all data series in the waterfall graph.
 *
 */
void WaterfallGraph::drawAllDataSeries()
{
    if (!graphicsScene || !dataSource || !dataRangesValid)
    {
        qDebug() << "drawAllDataSeries: Early return - graphicsScene:" << (graphicsScene != nullptr)
                 << "dataSource:" << (dataSource != nullptr)
                 << "dataRangesValid:" << dataRangesValid;
        return;
    }

    // Get all available data series labels
    std::vector<QString> seriesLabels = dataSource->getDataSeriesLabels();
    qDebug() << "drawAllDataSeries: Found" << seriesLabels.size() << "series labels";

    // If no multi-series data, fall back to legacy single series
    if (seriesLabels.empty())
    {
        qDebug() << "drawAllDataSeries: No series found, falling back to legacy single series";
        // Throw an exception
        // Gather more debug info about the WaterfallGraph state
        QString debugInfo;
        debugInfo += "No series found in data source.\n";
        debugInfo += QString("WaterfallGraph info:\n");
        debugInfo += QString("  graphicsScene: %1\n").arg(graphicsScene ? "set" : "nullptr");
        debugInfo += QString("  dataSource: %1\n").arg(dataSource ? "set" : "nullptr");
        debugInfo += QString("  dataRangesValid: %1\n").arg(dataRangesValid ? "true" : "false");
        debugInfo += QString("  drawingArea: (%1, %2, %3, %4)\n")
                         .arg(drawingArea.left())
                         .arg(drawingArea.top())
                         .arg(drawingArea.width())
                         .arg(drawingArea.height());
        debugInfo += QString("  yMin: %1, yMax: %2\n").arg(yMin).arg(yMax);
        debugInfo += QString("  timeMin: %1, timeMax: %2\n")
                         .arg(timeMin.toString())
                         .arg(timeMax.toString());
        debugInfo += QString("  autoUpdateYRange: %1\n").arg(autoUpdateYRange ? "true" : "false");
        debugInfo += QString("  rangeLimitingEnabled: %1\n").arg(rangeLimitingEnabled ? "true" : "false");
        qDebug() << debugInfo;
        throw std::runtime_error(debugInfo.toStdString());
    }

    // Draw each visible series
    for (const QString &seriesLabel : seriesLabels)
    {
        qDebug() << "drawAllDataSeries: Processing series:" << seriesLabel
                 << "visible:" << isSeriesVisible(seriesLabel);
        if (isSeriesVisible(seriesLabel))
        {
            drawDataSeries(seriesLabel);
        }
    }
}

/**
 * @brief Draw a specific data series.
 *
 * @param seriesLabel The label of the series to draw
 */
void WaterfallGraph::drawDataSeries(const QString &seriesLabel)
{
    if (!graphicsScene || !dataSource || !dataRangesValid)
    {
        qDebug() << "drawDataSeries: Early return for series:" << seriesLabel;
        return;
    }

    // Remove existing graphics items for this series if they exist (for incremental updates)
    auto pathIt = m_seriesPathItems.find(seriesLabel);
    if (pathIt != m_seriesPathItems.end() && pathIt->second)
    {
        graphicsScene->removeItem(pathIt->second);
        delete pathIt->second;
        m_seriesPathItems.erase(pathIt);
    }

    auto pointIt = m_seriesPointItems.find(seriesLabel);
    if (pointIt != m_seriesPointItems.end())
    {
        for (QGraphicsEllipseItem *item : pointIt->second)
        {
            if (item)
            {
                graphicsScene->removeItem(item);
                delete item;
            }
        }
        pointIt->second.clear();
    }

    const auto &yData = dataSource->getYDataSeries(seriesLabel);
    const auto &timestamps = dataSource->getTimestampsSeries(seriesLabel);

    qDebug() << "drawDataSeries: Series" << seriesLabel << "has" << yData.size() << "yData points and" << timestamps.size() << "timestamps";

    if (yData.empty() || timestamps.empty())
    {
        qDebug() << "No data available for series:" << seriesLabel;
        return;
    }

    // Filter data points to only include those within the current time range
    std::vector<std::pair<qreal, QDateTime>> visibleData;
    for (size_t i = 0; i < yData.size(); ++i)
    {
        if (timestamps[i] >= timeMin && timestamps[i] <= timeMax)
        {
            visibleData.push_back({yData[i], timestamps[i]});
        }
    }

    qDebug() << "drawDataSeries: Series" << seriesLabel << "has" << visibleData.size() << "visible data points within time range"
             << timeMin.toString() << "to" << timeMax.toString();

    if (visibleData.empty())
    {
        qDebug() << "No data points within current time range for series:" << seriesLabel;
        return;
    }

    // Get series color
    QColor seriesColor = getSeriesColor(seriesLabel);

    if (visibleData.size() < 2)
    {
        // Draw a single point if we only have one data point
        QPointF screenPoint = mapDataToScreen(visibleData[0].first, visibleData[0].second);
        QPen pointPen(seriesColor, 0); // No stroke (width 0)
        QGraphicsEllipseItem *pointItem = graphicsScene->addEllipse(screenPoint.x() - 2, screenPoint.y() - 2, 4, 4, pointPen);
        m_seriesPointItems[seriesLabel].push_back(pointItem);
        qDebug() << "Data series" << seriesLabel << "drawn with 1 visible point";
        return;
    }

    // Create a path for the line
    QPainterPath path;
    QPointF firstPoint = mapDataToScreen(visibleData[0].first, visibleData[0].second);
    path.moveTo(firstPoint);

    // Add lines connecting all visible data points
    for (size_t i = 1; i < visibleData.size(); ++i)
    {
        QPointF point = mapDataToScreen(visibleData[i].first, visibleData[i].second);
        path.lineTo(point);
    }

    // Draw the line and store reference
    QPen linePen(seriesColor, 2);
    QGraphicsPathItem *pathItem = graphicsScene->addPath(path, linePen);
    m_seriesPathItems[seriesLabel] = pathItem;

    // Draw data points and store references
    QPen pointPen(seriesColor, 0); // No stroke (width 0)
    std::vector<QGraphicsEllipseItem*> &pointItems = m_seriesPointItems[seriesLabel];
    pointItems.reserve(visibleData.size());
    for (size_t i = 0; i < visibleData.size(); ++i)
    {
        QPointF point = mapDataToScreen(visibleData[i].first, visibleData[i].second);
        QGraphicsEllipseItem *pointItem = graphicsScene->addEllipse(point.x() - 1, point.y() - 1, 2, 2, pointPen);
        pointItems.push_back(pointItem);
    }

    qDebug() << "Data series" << seriesLabel << "drawn with" << visibleData.size() << "visible points out of" << yData.size() << "total points";
}

// Multi-series support methods implementation

/**
 * @brief Set the color for a specific data series.
 *
 * @param seriesLabel The label of the series
 * @param color The color to use for the series
 */
void WaterfallGraph::setSeriesColor(const QString &seriesLabel, const QColor &color)
{
    seriesColors[seriesLabel] = color;
    qDebug() << "Series color set for" << seriesLabel << "to" << color.name();
}

/**
 * @brief Get the color for a specific data series.
 *
 * @param seriesLabel The label of the series
 * @return QColor The color of the series, or a default color if not set
 */
QColor WaterfallGraph::getSeriesColor(const QString &seriesLabel) const
{
    auto it = seriesColors.find(seriesLabel);
    if (it != seriesColors.end())
    {
        return it->second;
    }

    // Return a default color based on series index
    static const QColor defaultColors[] = {
        Qt::green, Qt::red, Qt::blue, Qt::yellow, Qt::cyan, Qt::magenta, Qt::white};

    // Generate a consistent color based on the series label hash
    uint hash = qHash(seriesLabel);
    return defaultColors[hash % (sizeof(defaultColors) / sizeof(defaultColors[0]))];
}

/**
 * @brief Set the visibility for a specific data series.
 *
 * @param seriesLabel The label of the series
 * @param visible True to make the series visible, false to hide it
 */
void WaterfallGraph::setSeriesVisible(const QString &seriesLabel, bool visible)
{
    seriesVisibility[seriesLabel] = visible;
    qDebug() << "Series visibility set for" << seriesLabel << "to" << (visible ? "visible" : "hidden");
}

/**
 * @brief Check if a specific data series is visible.
 *
 * @param seriesLabel The label of the series
 * @return bool True if the series is visible, false otherwise
 */
bool WaterfallGraph::isSeriesVisible(const QString &seriesLabel) const
{
    auto it = seriesVisibility.find(seriesLabel);
    if (it != seriesVisibility.end())
    {
        return it->second;
    }

    // Default to visible if not explicitly set
    return true;
}

/**
 * @brief Get all visible series labels.
 *
 * @return std::vector<QString> Vector of visible series labels
 */
std::vector<QString> WaterfallGraph::getVisibleSeries() const
{
    std::vector<QString> visibleSeries;

    if (!dataSource)
    {
        return visibleSeries;
    }

    std::vector<QString> allSeries = dataSource->getDataSeriesLabels();
    for (const QString &seriesLabel : allSeries)
    {
        if (isSeriesVisible(seriesLabel))
        {
            visibleSeries.push_back(seriesLabel);
        }
    }

    return visibleSeries;
}

void WaterfallGraph::setAutoUpdateYRange(bool enabled)
{
    autoUpdateYRange = enabled;

    // Trigger range update when switching modes
    if (dataSource && !dataSource->isEmpty())
    {
        updateYRange();
        draw();
    }

    qDebug() << "Auto-update Y range" << (enabled ? "enabled" : "disabled");
}

bool WaterfallGraph::getAutoUpdateYRange() const
{
    return autoUpdateYRange;
}

void WaterfallGraph::setZeroAxisValue(qreal value)
{
    m_zeroAxisValue = value;
}

qreal WaterfallGraph::getZeroAxisValue() const
{
    return m_zeroAxisValue;
}

// Convenience method to force range update for manual control
void WaterfallGraph::forceRangeUpdate()
{
    dataRangesValid = false;
    updateDataRanges();
    draw();
    qDebug() << "Forced range update - Y:" << yMin << "to" << yMax;
}

// New refactored range management methods

/**
 * @brief Update Y ranges based on current mode (manual vs auto)
 *
 */
void WaterfallGraph::updateYRange()
{
    if (autoUpdateYRange)
    {
        updateYRangeFromData();
    }
    else
    {
        updateYRangeFromCustom();
    }
}

/**
 * @brief Update Y range from data source (auto mode)
 *
 */
void WaterfallGraph::updateYRangeFromData()
{
    if (!dataSource || dataSource->isEmpty())
    {
        dataRangesValid = false;
        return;
    }

    auto yRange = dataSource->getCombinedYRange();
    qreal dataYMin = yRange.first;
    qreal dataYMax = yRange.second;

    if (rangeLimitingEnabled)
    {
        // Apply range limiting:
        // Use the intersection of custom range and data range
        yMin = qMax(customYMin, dataYMin); // Take the larger of custom min and data min
        yMax = qMin(customYMax, dataYMax); // Take the smaller of custom max and data max

        // Ensure min < max
        if (yMin >= yMax)
        {
            // If custom range is invalid or doesn't overlap with data, use data range
            yMin = dataYMin;
            yMax = dataYMax;
            qDebug() << "Warning: Custom range doesn't overlap with data range, using data range";
            qDebug() << "Custom range:" << customYMin << "to" << customYMax;
            qDebug() << "Data range:" << dataYMin << "to" << dataYMax;
        }
    }
    else
    {
        // Use data range directly
        yMin = dataYMin;
        yMax = dataYMax;
    }

    dataRangesValid = true;
    qDebug() << "Y range updated from data - Y:" << yMin << "to" << yMax
             << "Range limiting:" << (rangeLimitingEnabled ? "enabled" : "disabled");
}

/**
 * @brief Update Y range from custom values (manual mode)
 *
 */
void WaterfallGraph::updateYRangeFromCustom()
{
    qreal dataYMin = 0.0, dataYMax = 100.0;

    // Get actual data range for validation, even in manual mode
    if (dataSource && !dataSource->isEmpty())
    {
        auto yRange = dataSource->getCombinedYRange();
        dataYMin = yRange.first;
        dataYMax = yRange.second;
    }

    // Manual mode: range is locked to the custom min and max
    yMin = customYMin;
    yMax = customYMax;

    // Validate range is reasonable
    if (yMin >= yMax)
    {
        qDebug() << "Warning: Custom range is invalid (min >= max), using data range";
        qDebug() << "Custom range:" << customYMin << "to" << customYMax;
        yMin = dataYMin;
        yMax = dataYMax;
    }

    dataRangesValid = true;
    qDebug() << "Y range updated from custom - Y:" << yMin << "to" << yMax;
}

// Time range management methods implementation

/**
 * @brief Set a custom time range for the waterfall graph.
 *
 * @param timeMin Minimum time for the display range
 * @param timeMax Maximum time for the display range
 */
void WaterfallGraph::setTimeRange(const QDateTime &timeMin, const QDateTime &timeMax)
{
    // Validate the range
    if (timeMin >= timeMax)
    {
        qDebug() << "Error: Invalid time range - min must be before max";
        return;
    }

    customTimeMin = timeMin;
    customTimeMax = timeMax;
    customTimeRangeEnabled = true;

    // Update the current time range
    this->timeMin = timeMin;
    this->timeMax = timeMax;

    // Time range change requires full redraw (automatically marks all series dirty)
    setRenderState(RenderState::FULL_REDRAW);

    // Force redraw to show new time range
    draw();

    qDebug() << "Custom time range set to:" << timeMin.toString() << "to" << timeMax.toString();
}

/**
 * @brief Set the maximum time for the waterfall graph.
 *
 * @param timeMax Maximum time for the display range
 */
void WaterfallGraph::setTimeMax(const QDateTime &timeMax)
{
    if (customTimeRangeEnabled)
    {
        customTimeMax = timeMax;
        this->timeMax = timeMax;
    }
    else
    {
        // If not using custom range, set it based on data
        setTimeRangeFromData();
    }

    // Time range change requires full redraw (automatically marks all series dirty)
    setRenderState(RenderState::FULL_REDRAW);

    // Force redraw to show new time range
    draw();

    qDebug() << "Time max set to:" << timeMax.toString();
}

/**
 * @brief Set the minimum time for the waterfall graph.
 *
 * @param timeMin Minimum time for the display range
 */
void WaterfallGraph::setTimeMin(const QDateTime &timeMin)
{
    if (customTimeRangeEnabled)
    {
        customTimeMin = timeMin;
        this->timeMin = timeMin;
    }
    else
    {
        // If not using custom range, set it based on data
        setTimeRangeFromData();
    }

    // Time range change requires full redraw (automatically marks all series dirty)
    setRenderState(RenderState::FULL_REDRAW);

    // Force redraw to show new time range
    draw();

    qDebug() << "Time min set to:" << timeMin.toString();
}

/**
 * @brief Get the current maximum time.
 *
 * @return QDateTime Current maximum time
 */
QDateTime WaterfallGraph::getTimeMax() const
{
    return timeMax;
}

/**
 * @brief Get the current minimum time.
 *
 * @return QDateTime Current minimum time
 */
QDateTime WaterfallGraph::getTimeMin() const
{
    return timeMin;
}

/**
 * @brief Get the current time range.
 *
 * @return std::pair<QDateTime, QDateTime> Current time range (min, max)
 */
std::pair<QDateTime, QDateTime> WaterfallGraph::getTimeRange() const
{
    return std::make_pair(timeMin, timeMax);
}

/**
 * @brief Set the time range based on the data source timestamps.
 *
 */
void WaterfallGraph::setTimeRangeFromData()
{
    if (!dataSource || dataSource->isEmpty())
    {
        // No data available, use default range
        timeMax = QDateTime::currentDateTime();
        timeMin = timeMax.addMSecs(-getTimeIntervalMs());
        qDebug() << "No data available, using default time range";
        return;
    }

    // Get the combined time range from all series
    auto timeRange = dataSource->getCombinedTimeRange();
    timeMin = timeRange.first;
    timeMax = timeRange.second;

    qDebug() << "Time range set from data - Time:" << timeMin.toString() << "to" << timeMax.toString();
}

/**
 * @brief Set the time range based on the data source timestamps with a specific interval.
 *
 * @param intervalMs Time interval in milliseconds
 */
void WaterfallGraph::setTimeRangeFromDataWithInterval(qint64 intervalMs)
{
    if (!dataSource || dataSource->isEmpty())
    {
        // No data available, use default range
        timeMax = QDateTime::currentDateTime();
        timeMin = timeMax.addMSecs(-intervalMs);
        qDebug() << "No data available, using default time range with interval:" << intervalMs << "ms";
        return;
    }

    // Get the latest timestamp from data
    timeMax = dataSource->getLatestTime();
    timeMin = timeMax.addMSecs(-intervalMs);

    qDebug() << "Time range set from data with interval - Time:" << timeMin.toString() << "to" << timeMax.toString() << "Interval:" << intervalMs << "ms";
}

/**
 * @brief Check if time range is valid and reasonable for drawing markers
 * 
 * This performs a more robust check than just isValid():
 * - Both timeMin and timeMax must be valid
 * - timeMin must be strictly less than timeMax
 * - The range should be reasonable (not too large, not in the distant future)
 * - Either customTimeRangeEnabled is true (explicitly set) OR the range is within reasonable bounds
 * 
 * @return true if time range is valid for drawing, false otherwise
 */
bool WaterfallGraph::isTimeRangeValidForDrawing() const
{
    // Basic validity check
    if (!timeMin.isValid() || !timeMax.isValid())
    {
        return false;
    }
    
    // timeMin must be strictly less than timeMax
    if (timeMin >= timeMax)
    {
        return false;
    }
    
    // Check if range is reasonable (not too large - max 24 hours)
    qint64 rangeMs = timeMin.msecsTo(timeMax);
    const qint64 maxReasonableRangeMs = 24 * 60 * 60 * 1000; // 24 hours
    if (rangeMs > maxReasonableRangeMs)
    {
        qDebug() << "WaterfallGraph: Time range too large:" << rangeMs << "ms (max:" << maxReasonableRangeMs << "ms)";
        return false;
    }
    
    // Check if range is not too small (at least 100ms - very small but valid)
    // This allows for very short intervals like 15 minutes
    const qint64 minReasonableRangeMs = 100; // 100ms - very small but valid
    if (rangeMs < minReasonableRangeMs)
    {
        qDebug() << "WaterfallGraph: Time range too small:" << rangeMs << "ms (min:" << minReasonableRangeMs << "ms)";
        return false;
    }
    
    // If custom time range is enabled, it means it was explicitly set - trust it completely
    if (customTimeRangeEnabled)
    {
        return true;
    }
    
    // For non-custom ranges, perform additional reasonableness checks
    // But be more lenient - allow ranges that are set by setTimeInterval() or setTimeRangeFromData()
    QDateTime currentTime = QDateTime::currentDateTime();
    
    // Check if timeMax is not too far in the future (max 2 hours - more lenient)
    qint64 futureDiffMs = currentTime.msecsTo(timeMax);
    const qint64 maxFutureMs = 2 * 60 * 60 * 1000; // 2 hours (more lenient)
    if (futureDiffMs > maxFutureMs)
    {
        qDebug() << "WaterfallGraph: timeMax too far in future:" << futureDiffMs << "ms (max:" << maxFutureMs << "ms)";
        return false;
    }
    
    // Check if timeMin is not too far in the past (max 48 hours)
    qint64 pastDiffMs = timeMin.msecsTo(currentTime);
    const qint64 maxPastMs = 48 * 60 * 60 * 1000; // 48 hours
    if (pastDiffMs > maxPastMs)
    {
        qDebug() << "WaterfallGraph: timeMin too far in past:" << pastDiffMs << "ms (max:" << maxPastMs << "ms)";
        return false;
    }
    
    // Additional check: if the range is within reasonable bounds relative to current time,
    // and the range size matches a known interval (15min, 30min, 1hr, etc.), trust it
    // This handles cases where setTimeInterval() was called and set a valid range
    qint64 interval15Min = 15 * 60 * 1000;
    qint64 interval30Min = 30 * 60 * 1000;
    qint64 interval1Hr = 60 * 60 * 1000;
    qint64 interval2Hr = 2 * 60 * 60 * 1000;
    qint64 interval3Hr = 3 * 60 * 60 * 1000;
    qint64 interval6Hr = 6 * 60 * 60 * 1000;
    qint64 interval12Hr = 12 * 60 * 60 * 1000;
    
    // Check if range matches a known interval (within 1% tolerance)
    bool matchesKnownInterval = false;
    qreal tolerance = 0.01; // 1% tolerance
    if (qAbs(rangeMs - interval15Min) < interval15Min * tolerance ||
        qAbs(rangeMs - interval30Min) < interval30Min * tolerance ||
        qAbs(rangeMs - interval1Hr) < interval1Hr * tolerance ||
        qAbs(rangeMs - interval2Hr) < interval2Hr * tolerance ||
        qAbs(rangeMs - interval3Hr) < interval3Hr * tolerance ||
        qAbs(rangeMs - interval6Hr) < interval6Hr * tolerance ||
        qAbs(rangeMs - interval12Hr) < interval12Hr * tolerance)
    {
        matchesKnownInterval = true;
    }
    
    // If it matches a known interval and timeMax is within 2 hours of now, it's valid
    if (matchesKnownInterval && futureDiffMs <= maxFutureMs && pastDiffMs <= maxPastMs)
    {
        return true;
    }
    
    // Final check: if timeMax is close to current time (within 5 minutes) and range is reasonable, trust it
    if (futureDiffMs >= -300000 && futureDiffMs <= 300000 && rangeMs <= maxReasonableRangeMs) // Within 5 minutes
    {
        return true;
    }
    
    return true; // Default to true if all basic checks pass
}

/**
 * @brief Unset the custom time range and revert to using data-based time range.
 *
 */
void WaterfallGraph::unsetCustomTimeRange()
{
    customTimeRangeEnabled = false;
    customTimeMin = QDateTime();
    customTimeMax = QDateTime();

    // Update time range based on data
    setTimeRangeFromData();

    // Force redraw to show new time range
    draw();

    qDebug() << "Custom time range unset, reverting to data-based time range";
}

// Crosshair functionality implementation

/**
 * @brief Setup crosshair graphics items in the overlay scene
 */
void WaterfallGraph::setupCrosshair()
{
    if (!overlayScene) {
        return;
    }


    // Create horizontal crosshair line
    crosshairHorizontal = new QGraphicsLineItem();
    crosshairHorizontal->setPen(QPen(Qt::cyan, 1.0, Qt::SolidLine)); // Thin cyan line
    crosshairHorizontal->setZValue(1000); // High z-value to appear on top
    crosshairHorizontal->setVisible(false);
    // Make crosshair not accept mouse events so it doesn't block marker selection or cause duplication
    crosshairHorizontal->setAcceptedMouseButtons(Qt::NoButton);
    crosshairHorizontal->setAcceptHoverEvents(false);
    overlayScene->addItem(crosshairHorizontal);
    
    // Create vertical crosshair line
    crosshairVertical = new QGraphicsLineItem();
    crosshairVertical->setPen(QPen(Qt::cyan, 1.0, Qt::SolidLine)); // Thin cyan line
    crosshairVertical->setZValue(1000); // High z-value to appear on top
    crosshairVertical->setVisible(false);
    // Make crosshair not accept mouse events so it doesn't block marker selection or cause duplication
    crosshairVertical->setAcceptedMouseButtons(Qt::NoButton);
    crosshairVertical->setAcceptHoverEvents(false);
    overlayScene->addItem(crosshairVertical);
}

/**
 * @brief Update crosshair position based on mouse position
 *
 * @param mousePos Mouse position in scene coordinates
 */
void WaterfallGraph::updateCrosshair(const QPointF &mousePos)
{
    // If cursor layer is enabled, just update position (timer will handle rendering)
    if (m_cursorLayerEnabled)
    {
        // Convert scene position to widget position for m_lastMousePos
        if (overlayView)
        {
            m_lastMousePos = overlayView->mapFromScene(mousePos);
        }
        return;
    }

    // Legacy overlay mode: update crosshair directly
    if (!overlayScene) {
        return;
    }
    
    // Get the scene rectangle
    QRectF sceneRect = overlayScene->sceneRect();
    if (sceneRect.isEmpty()) {
        sceneRect = QRectF(0, 0, this->width(), this->height());
    }
    
    // Update horizontal and vertical lines
    crosshairHorizontal->setLine(sceneRect.left(), mousePos.y(), sceneRect.right(), mousePos.y());
    crosshairVertical->setLine(mousePos.x(), sceneRect.top(), mousePos.x(), sceneRect.bottom());
    
    // Notify crosshair X position change (only if position changed significantly)
    qreal currentX = mousePos.x();
    if (qAbs(currentX - lastNotifiedCrosshairXPosition) >= 1.0)
    {
        notifyCrosshairPositionChanged(currentX);
    }
}

/**
 * @brief Show the crosshair
 */
void WaterfallGraph::showCrosshair()
{
    if (m_cursorLayerEnabled)
    {
        cursorCrosshairHorizontal->setVisible(true);
        cursorCrosshairVertical->setVisible(true);
    }
    else
    {
        crosshairHorizontal->setVisible(true);
        crosshairVertical->setVisible(true);
    }
}

/**
 * @brief Hide the crosshair
 */
void WaterfallGraph::hideCrosshair()
{
    if (m_cursorLayerEnabled)
    {
        cursorCrosshairHorizontal->setVisible(false);
        cursorCrosshairVertical->setVisible(false);
    }
    else
    {
        crosshairHorizontal->setVisible(false);
        crosshairVertical->setVisible(false);
    }
    
    // Notify that crosshair is hidden (clear label)
    notifyCrosshairPositionChanged(-1.0); // Use -1 to indicate crosshair is hidden
}

/**
 * @brief Enable or disable the crosshair functionality
 *
 * @param enabled True to enable crosshair, false to disable
 */
void WaterfallGraph::setCrosshairEnabled(bool enabled)
{
    if (crosshairEnabled != enabled)
    {
        crosshairEnabled = enabled;

        // Make overlay view transparent to mouse events so WaterfallGraph receives them
        overlayView->setAttribute(Qt::WA_TransparentForMouseEvents, true);

        // If disabling, hide the crosshair
        if (!enabled)
        {
            hideCrosshair();
            // Make overlay view opaque to mouse events so WaterfallGraph receives them
            overlayView->setAttribute(Qt::WA_TransparentForMouseEvents, false);
        }
    }
}

/**
 * @brief Check if crosshair is currently enabled
 *
 * @return bool True if crosshair is enabled, false otherwise
 */
bool WaterfallGraph::isCrosshairEnabled() const
{
    return crosshairEnabled;
}

/**
 * @brief Update the cursor layer - called by timer at fixed rate (60fps)
 */
void WaterfallGraph::updateCursorLayer()
{
    if (!cursorScene || !cursorView || !m_cursorLayerEnabled)
    {
        return;
    }

    // Get the scene rectangle (cache it to avoid recalculation)
    QRectF sceneRect = cursorScene->sceneRect();
    if (sceneRect.isEmpty())
    {
        sceneRect = QRectF(0, 0, this->width(), this->height());
        cursorScene->setSceneRect(sceneRect);
    }

    bool needsUpdate = false;

    // Update time axis cursor from shared state
    bool timeAxisVisible = false;
    if (m_cursorSyncState && m_cursorSyncState->hasCursorTime && m_cursorSyncState->cursorTime.isValid())
    {
        qreal yPos = mapTimeToY(m_cursorSyncState->cursorTime);
        if (yPos >= 0)
        {
            cursorTimeAxisLine->setLine(sceneRect.left(), yPos, sceneRect.right(), yPos);
            timeAxisVisible = true;
            needsUpdate = true;
        }
    }
    
    if (cursorTimeAxisLine->isVisible() != timeAxisVisible)
    {
        cursorTimeAxisLine->setVisible(timeAxisVisible);
        needsUpdate = true;
    }

    // Update crosshair from last mouse position
    bool crosshairVisible = crosshairEnabled && !m_lastMousePos.isNull() && 
                            m_lastMousePos.x() >= 0 && m_lastMousePos.y() >= 0 &&
                            m_lastMousePos.x() < this->width() && m_lastMousePos.y() < this->height();
    
    if (crosshairVisible)
    {
        cursorCrosshairHorizontal->setLine(sceneRect.left(), m_lastMousePos.y(), 
                                           sceneRect.right(), m_lastMousePos.y());
        cursorCrosshairVertical->setLine(m_lastMousePos.x(), sceneRect.top(), 
                                         m_lastMousePos.x(), sceneRect.bottom());
        needsUpdate = true;
        
        // Notify crosshair X position change (only if position changed significantly)
        qreal currentX = m_lastMousePos.x();
        if (qAbs(currentX - lastNotifiedCrosshairXPosition) >= 1.0)
        {
            notifyCrosshairPositionChanged(currentX);
        }
    }
    else
    {
        // Crosshair is not visible, notify with -1 to clear label
        if (lastNotifiedCrosshairXPosition >= 0)
        {
            notifyCrosshairPositionChanged(-1.0);
        }
    }
    
    if (cursorCrosshairHorizontal->isVisible() != crosshairVisible)
    {
        cursorCrosshairHorizontal->setVisible(crosshairVisible);
        cursorCrosshairVertical->setVisible(crosshairVisible);
        needsUpdate = true;
    }

    // Only trigger repaint if something changed
    if (needsUpdate)
    {
        cursorView->update();
    }
}

/**
 * @brief Set the shared sync state pointer for cursor synchronization
 *
 * @param syncState Pointer to the shared sync state
 */
void WaterfallGraph::setCursorSyncState(GraphContainerSyncState *syncState)
{
    m_cursorSyncState = syncState;
    // Timer will automatically read from sync state when it fires
}

/**
 * @brief Enable or disable the cursor layer
 *
 * @param enabled True to enable cursor layer, false to disable
 */
void WaterfallGraph::setCursorLayerEnabled(bool enabled)
{
    if (m_cursorLayerEnabled != enabled)
    {
        m_cursorLayerEnabled = enabled;

        if (enabled)
        {
            if (!cursorUpdateTimer->isActive())
            {
                cursorUpdateTimer->start();
            }
        }
        else
        {
            if (cursorUpdateTimer->isActive())
            {
                cursorUpdateTimer->stop();
            }
            // Hide all cursor items
            cursorCrosshairHorizontal->setVisible(false);
            cursorCrosshairVertical->setVisible(false);
            cursorTimeAxisLine->setVisible(false);
        }
    }
}

/**
 * @brief Check if cursor layer is currently enabled
 *
 * @return bool True if cursor layer is enabled, false otherwise
 */
bool WaterfallGraph::isCursorLayerEnabled() const
{
    return m_cursorLayerEnabled;
}

// Time axis cursor functionality implementation

/**
 * @brief Map a time value to a Y coordinate on the screen
 *
 * @param time The time to map
 * @return qreal The Y coordinate, or -1 if invalid
 */
qreal WaterfallGraph::mapTimeToY(const QDateTime &time) const
{
    if (!time.isValid())
    {
        qDebug() << "mapTimeToY: Invalid time provided";
        return -1.0;
    }

    QRectF area = drawingArea;
    if (area.isEmpty())
    {
        if (graphicsScene)
        {
            area = graphicsScene->sceneRect();
        }
        if (area.isEmpty())
        {
            qDebug() << "mapTimeToY: Drawing area unavailable";
            return -1.0;
        }
    }

    if (!timeMax.isValid())
    {
        qDebug() << "mapTimeToY: timeMax is invalid";
        return -1.0;
    }

    qint64 intervalMs = getTimeIntervalMs();
    if (intervalMs <= 0)
    {
        qDebug() << "mapTimeToY: Invalid interval" << intervalMs;
        return -1.0;
    }

    qint64 timeOffsetMs = time.msecsTo(timeMax);
    qreal normalizedY = timeOffsetMs / static_cast<qreal>(intervalMs);
    normalizedY = qMax(0.0, qMin(1.0, normalizedY));

    return area.top() + normalizedY * area.height();
}

/**
 * @brief Set the time axis cursor at a specific time
 *
 * @param time The time at which to display the horizontal cursor line
 */
void WaterfallGraph::setTimeAxisCursor(const QDateTime &time)
{
    // Update shared sync state if available (cursor layer will read from it)
    if (m_cursorSyncState)
    {
        if (time.isValid())
        {
            m_cursorSyncState->cursorTime = time;
            m_cursorSyncState->hasCursorTime = true;
        }
        else
        {
            m_cursorSyncState->hasCursorTime = false;
        }
    }

    // Legacy overlay mode: update timeAxisCursor directly if cursor layer is disabled
    if (!m_cursorLayerEnabled)
    {
        if (!timeAxisCursor || !overlayScene)
        {
            qDebug() << "Time axis cursor not initialized";
            return;
        }

        if (!time.isValid())
        {
            qDebug() << "Invalid time provided for time axis cursor";
            clearTimeAxisCursor();
            return;
        }

        // Map time to Y coordinate
        qreal yPos = mapTimeToY(time);

        if (yPos < 0)
        {
            qDebug() << "Could not map time to Y position - data ranges may not be valid";
            clearTimeAxisCursor();
            return;
        }

        // Get the scene rectangle
        QRectF sceneRect = overlayScene->sceneRect();
        
        // If scene rect is empty, use widget dimensions
        if (sceneRect.isEmpty()) {
            sceneRect = QRectF(0, 0, this->width(), this->height());
        }

        // Update horizontal line (left to right) at the calculated Y position
        timeAxisCursor->setLine(sceneRect.left(), yPos, sceneRect.right(), yPos);
        timeAxisCursor->setVisible(true);
//-------syed -----------------rebase conflict here
    // Force immediate repaint to clear any stale rendering and prevent trails
    if (overlayView) {
        overlayView->update();
    }

        qDebug() << "Time axis cursor set at time:" << time.toString() << "Y position:" << yPos;
    }
    else
    {
        // Cursor layer mode: timer will handle rendering from sync state
        qDebug() << "Time axis cursor set at time:" << time.toString() << "(cursor layer will render)";
    }
}

/**
 * @brief Clear/hide the time axis cursor
 */
void WaterfallGraph::clearTimeAxisCursor()
{
    // Update shared sync state if available
    if (m_cursorSyncState)
    {
        m_cursorSyncState->hasCursorTime = false;
    }

    // Legacy overlay mode: hide timeAxisCursor directly if cursor layer is disabled
    if (!m_cursorLayerEnabled && timeAxisCursor)
    {
        timeAxisCursor->setVisible(false);
        // Force immediate repaint to clear the cursor
        if (overlayView) {
            overlayView->update();
        }
        qDebug() << "Time axis cursor cleared";
    }
    else
    {
        // Cursor layer mode: timer will handle hiding from sync state
        qDebug() << "Time axis cursor cleared (cursor layer will handle)";
    }
}

/**
 * @brief Set a callback to receive cursor time updates
 *
 * @param callback Function to invoke when cursor time changes
 */
void WaterfallGraph::setCursorTimeChangedCallback(const std::function<void(const QDateTime &, qreal)> &callback)
{
    cursorTimeChangedCallback = callback;
}

/**
 * @brief Set a callback to receive crosshair position updates
 *
 * @param callback Function to invoke when crosshair X position changes
 */
void WaterfallGraph::setCrosshairPositionChangedCallback(const std::function<void(qreal xPosition)> &callback)
{
    crosshairPositionChangedCallback = callback;
}

/**
 * @brief Notify listeners about cursor time changes
 *
 * @param time Cursor time or invalid to clear
 * @param yPosition Y position in scene coordinates, or -1.0 if not available
 */
void WaterfallGraph::notifyCursorTimeChanged(const QDateTime &time, qreal yPosition)
{
    if (!cursorTimeChangedCallback)
    {
        lastNotifiedCursorTime = time;
        lastNotifiedYPosition = yPosition;
        return;
    }

    // Check if both time and Y position are the same (skip update if unchanged)
    if (time.isValid())
    {
        if (lastNotifiedCursorTime.isValid() && time == lastNotifiedCursorTime)
        {
            // Time is the same, check if Y position changed significantly (more than 1 pixel)
            if (qAbs(yPosition - lastNotifiedYPosition) < 1.0)
            {
                return; // Both time and position are essentially the same
            }
        }
    }
    else if (!lastNotifiedCursorTime.isValid() && yPosition < 0)
    {
        // Both are invalid/cleared, skip if we already notified this state
        if (lastNotifiedYPosition < 0)
        {
            return;
        }
    }

    lastNotifiedCursorTime = time;
    
    lastNotifiedYPosition = yPosition;
    cursorTimeChangedCallback(time, yPosition);
}
//-------syed -----------------rebase conflict here
/**
 * @brief Notify listeners about crosshair position changes
 *
 * @param xPosition Crosshair X position in scene coordinates, or -1.0 to clear
 */
void WaterfallGraph::notifyCrosshairPositionChanged(qreal xPosition)
{
    // Skip if position hasn't changed significantly (more than 1 pixel)
    if (xPosition >= 0 && lastNotifiedCrosshairXPosition >= 0)
    {
        if (qAbs(xPosition - lastNotifiedCrosshairXPosition) < 1.0)
        {
            return; // Position hasn't changed enough
        }
    }
    else if (xPosition < 0 && lastNotifiedCrosshairXPosition < 0)
    {
        return; // Both are cleared, no need to notify again
    }
    
    lastNotifiedCrosshairXPosition = xPosition;
    
    if (crosshairPositionChangedCallback)
    {
        crosshairPositionChangedCallback(xPosition);
    }
}

