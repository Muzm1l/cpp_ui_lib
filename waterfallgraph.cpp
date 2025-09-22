#include "waterfallgraph.h"

/**
 * @brief Construct a new waterfallgraph::waterfallgraph object
 * 
 * @param parent Parent widget
 * @param enableGrid Whether to enable grid display
 * @param gridDivisions Number of grid divisions
 * @param timeInterval Time interval for the waterfall display
 */
waterfallgraph::waterfallgraph(QWidget *parent, bool enableGrid, int gridDivisions, TimeInterval timeInterval)
    : QWidget(parent)
    , graphicsView(nullptr)
    , graphicsScene(nullptr)
    , gridEnabled(enableGrid)
    , gridDivisions(gridDivisions)
    , yMin(0.0), yMax(0.0)
    , timeMin(QDateTime())
    , timeMax(QDateTime())
    , dataRangesValid(false)
    , timeInterval(timeInterval)
    , dataSource(nullptr)
    , isDragging(false)
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
    graphicsView->setDragMode(QGraphicsView::NoDrag); // We'll handle our own mouse events
    
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
    
    // Set up layout to make the graphics view fill the widget with no margins
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(graphicsView);
    setLayout(layout);
    
    // Size policy will be set by parent container
    
    // Enable mouse tracking
    setMouseTracking(true);
    
    // Initial setup will happen in showEvent
    qDebug() << "Constructor - Widget size:" << this->size();
}

/**
 * @brief Destroy the waterfallgraph::waterfallgraph object
 * 
 */
waterfallgraph::~waterfallgraph()
{
    // Note: graphicsView and graphicsScene are child widgets/scenes, so they will be automatically deleted by Qt's parent-child mechanism
}

/**
 * @brief Set the data source for this waterfall graph.
 * 
 * @param dataSource Reference to the WaterfallData object to use
 */
void waterfallgraph::setDataSource(WaterfallData& dataSource)
{
    this->dataSource = &dataSource;
    draw(); // Trigger redraw with new data source
    qDebug() << "Data source set successfully";
}

/**
 * @brief Get the current data source.
 * 
 * @return WaterfallData* Pointer to the current data source, or nullptr if not set
 */
WaterfallData* waterfallgraph::getDataSource() const
{
    return dataSource;
}

/**
 * @brief Set the data for the graph.
 * 
 * @param yData 
 * @param timestamps 
 */
void waterfallgraph::setData(const std::vector<qreal>& yData, const std::vector<QDateTime>& timestamps)
{
    if (!dataSource) {
        qDebug() << "Error: No data source set";
        return;
    }
    
    // Store the data using the data source
    dataSource->setData(yData, timestamps);
    
    qDebug() << "Data set successfully. Size:" << dataSource->getDataSize();
    
    // Redraw the graph with the new data
    draw();
}

/**
 * @brief Set the data for the graph using WaterfallData object.
 * 
 * @param data 
 */
void waterfallgraph::setData(const WaterfallData& data)
{
    if (!dataSource) {
        qDebug() << "Error: No data source set";
        return;
    }
    
    *dataSource = data;
    
    qDebug() << "Data set successfully from WaterfallData object. Size:" << dataSource->getDataSize();
    
    // Redraw the graph with the new data
    draw();
}

/**
 * @brief Clear all data from the graph.
 * 
 */
void waterfallgraph::clearData()
{
    if (!dataSource) {
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
void waterfallgraph::addDataPoint(qreal yValue, const QDateTime& timestamp)
{
    if (!dataSource) {
        qDebug() << "Error: No data source set";
        return;
    }
    
    dataSource->addDataPoint(yValue, timestamp);
    
    qDebug() << "Data point added. New size:" << dataSource->getDataSize();
    
    // Redraw the graph with the new data
    draw();
}

/**
 * @brief Add multiple data points to the graph.
 * 
 * @param yValues 
 * @param timestamps 
 */
void waterfallgraph::addDataPoints(const std::vector<qreal>& yValues, const std::vector<QDateTime>& timestamps)
{
    if (!dataSource) {
        qDebug() << "Error: No data source set";
        return;
    }
    
    dataSource->addDataPoints(yValues, timestamps);
    
    qDebug() << "Data points added. New size:" << dataSource->getDataSize();
    
    // Redraw the graph with the new data
    draw();
}

/**
 * @brief Get all data from the graph.
 * 
 * @return WaterfallData 
 */
WaterfallData waterfallgraph::getData() const
{
    if (!dataSource) {
        return WaterfallData();
    }
    return *dataSource;
}

/**
 * @brief Get data within specified y extents.
 * 
 * @param yMin 
 * @param yMax 
 * @return std::vector<std::pair<qreal, QDateTime>> 
 */
std::vector<std::pair<qreal, QDateTime>> waterfallgraph::getDataWithinYExtents(qreal yMin, qreal yMax) const
{
    if (!dataSource) {
        return std::vector<std::pair<qreal, QDateTime>>();
    }
    return dataSource->getDataWithinYExtents(yMin, yMax);
}

/**
 * @brief Get data within specified time range.
 * 
 * @param startTime 
 * @param endTime 
 * @return std::vector<std::pair<qreal, QDateTime>> 
 */
std::vector<std::pair<qreal, QDateTime>> waterfallgraph::getDataWithinTimeRange(const QDateTime& startTime, const QDateTime& endTime) const
{
    if (!dataSource) {
        return std::vector<std::pair<qreal, QDateTime>>();
    }
    return dataSource->getDataWithinTimeRange(startTime, endTime);
}

/**
 * @brief Get direct access to y data vector.
 * 
 * @return const std::vector<qreal>& 
 */
const std::vector<qreal>& waterfallgraph::getYData() const
{
    static const std::vector<qreal> emptyVector;
    if (!dataSource) {
        return emptyVector;
    }
    return dataSource->getYData();
}

/**
 * @brief Get direct access to timestamps vector.
 * 
 * @return const std::vector<QDateTime>& 
 */
const std::vector<QDateTime>& waterfallgraph::getTimestamps() const
{
    static const std::vector<QDateTime> emptyVector;
    if (!dataSource) {
        return emptyVector;
    }
    return dataSource->getTimestamps();
}

/**
 * @brief Set the time interval for the waterfall graph.
 * 
 * @param interval Time interval enum value
 */
void waterfallgraph::setTimeInterval(TimeInterval interval)
{
    timeInterval = interval;
    
    // Update data ranges and redraw if we have data
    if (dataSource && !dataSource->isEmpty()) {
        updateDataRanges();
        draw();
    }
    
    qDebug() << "Time interval set to:" << timeIntervalToString(interval);
}

/**
 * @brief Get the current time interval.
 * 
 * @return TimeInterval Current time interval enum value
 */
TimeInterval waterfallgraph::getTimeInterval() const
{
    return timeInterval;
}

/**
 * @brief Get the current time interval in milliseconds.
 * 
 * @return qint64 Time interval in milliseconds
 */
qint64 waterfallgraph::getTimeIntervalMs() const
{
    return static_cast<qint64>(timeInterval) * 60 * 1000; // Convert minutes to milliseconds
}

/**
 * @brief Enable or disable the grid display.
 * 
 * @param enabled True to enable grid, false to disable
 */
void waterfallgraph::setGridEnabled(bool enabled)
{
    if (gridEnabled != enabled) {
        gridEnabled = enabled;
        draw(); // Redraw to show/hide grid
        qDebug() << "Grid" << (enabled ? "enabled" : "disabled");
    }
}

/**
 * @brief Check if the grid is currently enabled.
 * 
 * @return bool True if grid is enabled, false otherwise
 */
bool waterfallgraph::isGridEnabled() const
{
    return gridEnabled;
}

/**
 * @brief Set the number of grid divisions.
 * 
 * @param divisions Number of grid divisions (must be positive)
 */
void waterfallgraph::setGridDivisions(int divisions)
{
    if (divisions > 0 && gridDivisions != divisions) {
        gridDivisions = divisions;
        if (gridEnabled) {
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
int waterfallgraph::getGridDivisions() const
{
    return gridDivisions;
}

/**
 * @brief Handle mouse click events.
 * 
 * @param scenePos 
 */
void waterfallgraph::onMouseClick(const QPointF& scenePos)
{
    qDebug() << "Mouse clicked at scene position:" << scenePos;
    // This is a virtual function that can be overridden in derived classes
}

void waterfallgraph::onMouseDrag(const QPointF& scenePos)
{
    qDebug() << "Mouse dragged to scene position:" << scenePos;
    // This is a virtual function that can be overridden in derived classes
}

/**
 * @brief Draw the graph.
 * 
 */
void waterfallgraph::draw()
{
    if (!graphicsScene)
        return;

    // Clear existing items
    graphicsScene->clear();

    // Update the drawing area
    setupDrawingArea();

    // Draw grid if enabled
    if (gridEnabled) {
        drawGrid();
    }

    // Draw the actual data line if we have data
    if (dataSource && !dataSource->isEmpty()) {
        updateDataRanges();
        drawDataLine();
    }
}

/**
 * @brief Update the graphics dimensions.
 * 
 */
void waterfallgraph::updateGraphicsDimensions()
{
    if (!graphicsView || !graphicsScene) return;
    
    // Get the current size of the widget
    QSize widgetSize = this->size();
    
    qDebug() << "updateGraphicsDimensions - Widget size:" << widgetSize;
    
    // Only update if we have valid dimensions
    if (widgetSize.width() > 0 && widgetSize.height() > 0) {
        // Set scene rect to match widget size exactly
        QRectF newSceneRect(0, 0, widgetSize.width(), widgetSize.height());
        graphicsScene->setSceneRect(newSceneRect);
        
        // Ensure the graphics view fits the scene exactly (no scrollbars)
        graphicsView->fitInView(newSceneRect, Qt::KeepAspectRatio);
        graphicsView->resetTransform(); // Reset any scaling
        graphicsView->setTransform(QTransform()); // Ensure 1:1 mapping
        
        // Update the drawing area
        setupDrawingArea();
        
        // Redraw the scene
        draw();
        
        qDebug() << "Graphics dimensions updated successfully to:" << widgetSize;
        qDebug() << "Scene rect is now:" << graphicsScene->sceneRect();
    } else {
        qDebug() << "Widget size is invalid, skipping update";
    }
}

/**
 * @brief Setup the drawing area.
 * 
 */
void waterfallgraph::setupDrawingArea()
{
    // Set up the drawing area to cover the entire scene
    drawingArea = graphicsScene->sceneRect();
    qDebug() << "Drawing area set to:" << drawingArea;
}


/**
 * @brief Draw the grid.
 * 
 */
void waterfallgraph::drawGrid()
{
    if (!graphicsScene || !gridEnabled || drawingArea.isEmpty() || gridDivisions <= 0) return;
    
    QPen gridPen(Qt::white, 1, Qt::DashLine); // White grid lines for black background
    
    // Draw vertical grid lines (for x-axis - variable)
    double stepX = drawingArea.width() / gridDivisions;
    for (int i = 0; i <= gridDivisions; ++i) {
        double x = drawingArea.left() + i * stepX;
        graphicsScene->addLine(x, drawingArea.top(), x, drawingArea.bottom(), gridPen);
    }
    
    // Draw horizontal grid lines (for y-axis - time)
    // These represent time divisions within the fixed interval
    double stepY = drawingArea.height() / gridDivisions;
    for (int i = 0; i <= gridDivisions; ++i) {
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
void waterfallgraph::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // Convert widget coordinates to scene coordinates
        QPointF scenePos = graphicsView->mapToScene(event->pos());
        
        // Check if the click is within the drawing area
        if (drawingArea.contains(scenePos)) {
            isDragging = true;
            lastMousePos = scenePos;
            onMouseClick(scenePos);
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
void waterfallgraph::mouseMoveEvent(QMouseEvent *event)
{
    if (isDragging && (event->buttons() & Qt::LeftButton)) {
        // Convert widget coordinates to scene coordinates
        QPointF scenePos = graphicsView->mapToScene(event->pos());
        
        // Check if the move is within the drawing area
        if (drawingArea.contains(scenePos)) {
            onMouseDrag(scenePos);
            lastMousePos = scenePos;
        }
    }
    
    // Call parent implementation
    QWidget::mouseMoveEvent(event);
}

/**
 * @brief Handle mouse release events.
 * 
 * @param event 
 */
void waterfallgraph::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
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
void waterfallgraph::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    
    // Ensure graphics view fits the widget exactly
    if (graphicsView) {
        graphicsView->resize(event->size());
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
void waterfallgraph::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    
    // This is called when the widget becomes visible
    qDebug() << "showEvent - Widget size:" << this->size();
    qDebug() << "showEvent - Graphics view size:" << graphicsView->size();
    
    // Ensure graphics view fits the widget exactly
    if (graphicsView) {
        graphicsView->resize(this->size());
    }
    
    // Update graphics dimensions now that we're visible
    updateGraphicsDimensions();
}

/**
 * @brief Update data ranges from the waterfall data.
 * 
 */
void waterfallgraph::updateDataRanges()
{
    if (!dataSource || dataSource->isEmpty()) {
        dataRangesValid = false;
        return;
    }
    
    auto yRange = dataSource->getYRange();
    
    yMin = yRange.first;
    yMax = yRange.second;
    
    // Set time range based on fixed interval with current time as top (t=0)
    timeMax = QDateTime::currentDateTime(); // Current time (top of graph)
    timeMin = timeMax.addMSecs(-getTimeIntervalMs()); // Bottom of graph
    
    dataRangesValid = true;
    
    qDebug() << "Data ranges updated - Y:" << yMin << "to" << yMax 
             << "Time:" << timeMin.toString() << "to" << timeMax.toString()
             << "Interval:" << timeIntervalToString(timeInterval);
}

/**
 * @brief Map data coordinates to screen coordinates.
 * 
 * @param yValue 
 * @param timestamp 
 * @return QPointF 
 */
QPointF waterfallgraph::mapDataToScreen(qreal yValue, const QDateTime& timestamp) const
{
    if (!dataRangesValid || drawingArea.isEmpty()) {
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
 * @brief Draw the data line from top to bottom.
 * 
 */
void waterfallgraph::drawDataLine()
{
    if (!graphicsScene || !dataSource || dataSource->isEmpty() || !dataRangesValid) {
        return;
    }
    
    const auto& yData = dataSource->getYData();
    const auto& timestamps = dataSource->getTimestamps();
    
    if (yData.size() < 2) {
        // Draw a single point if we only have one data point
        QPointF screenPoint = mapDataToScreen(yData[0], timestamps[0]);
        QPen pointPen(Qt::green, 3);
        graphicsScene->addEllipse(screenPoint.x() - 2, screenPoint.y() - 2, 4, 4, pointPen);
        return;
    }
    
    // Create a path for the line
    QPainterPath path;
    QPointF firstPoint = mapDataToScreen(yData[0], timestamps[0]);
    path.moveTo(firstPoint);
    
    // Add lines connecting all data points
    for (size_t i = 1; i < yData.size(); ++i) {
        QPointF point = mapDataToScreen(yData[i], timestamps[i]);
        path.lineTo(point);
    }
    
    // Draw the line
    QPen linePen(Qt::green, 2);
    graphicsScene->addPath(path, linePen);
    
    // Draw data points
    QPen pointPen(Qt::yellow, 2);
    for (size_t i = 0; i < yData.size(); ++i) {
        QPointF point = mapDataToScreen(yData[i], timestamps[i]);
        graphicsScene->addEllipse(point.x() - 1, point.y() - 1, 2, 2, pointPen);
    }
    
    qDebug() << "Data line drawn with" << yData.size() << "points";
}
