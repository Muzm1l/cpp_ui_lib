#include "ltwgraph.h"
#include "debugutils.h"
#include <QDebug>
#include <QMouseEvent>
// QEnterEvent not available in Qt5 for QWidget::enterEvent; using QEvent*
#include <QtMath>

/**
 * @brief Construct a new LTWGraph::LTWGraph object
 *
 * @param parent Parent widget
 * @param enableGrid Whether to enable grid display
 * @param gridDivisions Number of grid divisions
 * @param timeInterval Time interval for the waterfall display
 */
LTWGraph::LTWGraph(QWidget *parent, bool enableGrid, int gridDivisions, TimeInterval timeInterval)
    : WaterfallGraph(parent, enableGrid, gridDivisions, timeInterval)
{
    DEBUG_OUT() << "LTWGraph constructor called";
}

/**
 * @brief Destroy the LTWGraph::LTWGraph object
 *
 */
LTWGraph::~LTWGraph()
{
    DEBUG_OUT() << "LTWGraph destructor called";
}

/**
 * @brief Override draw method to create scatterplots by default
 *
 */
void LTWGraph::draw()
{
    DEBUG_OUT() << "LTW: draw() called";
    
    if (!graphicsScene) {
        DEBUG_OUT() << "LTW: draw() early return - no graphicsScene";
        return;
    }
    
    // Prevent concurrent drawing to avoid marker duplication
    if (isDrawing) {
        DEBUG_OUT() << "LTWGraph: draw() already in progress, skipping";
        return;
    }
    
    isDrawing = true;

    // Only perform full clear for FULL_REDRAW state
    bool needsFullClear = (m_renderState == RenderState::FULL_REDRAW);
    
    if (needsFullClear)
    {
        // Clear all item pointers since clear() will delete them
        // This prevents use-after-free in cleanup functions
        m_seriesScatterplotItems.clear();
        m_seriesPathItems.clear();
        m_seriesPointItems.clear();
        
        graphicsScene->clear();
        graphicsScene->update(); // Force immediate update to ensure clearing is visible
    }
    
    setupDrawingArea();

    if (needsFullClear && gridEnabled)
    {
        drawGrid();
    }

    if (dataSource && !dataSource->isEmpty())
    {
        DEBUG_OUT() << "LTW: draw() - dataSource available, updating ranges and drawing series";
        updateDataRanges();
        
        // Draw custom markers for each series with their respective colors
        std::vector<QString> seriesLabels = dataSource->getDataSeriesLabels();
        DEBUG_OUT() << "LTW: draw() - found" << seriesLabels.size() << "series labels";
        
        for (const QString &seriesLabel : seriesLabels)
        {
            DEBUG_OUT() << "LTW: draw() - processing series:" << seriesLabel << "visible:" << isSeriesVisible(seriesLabel);
            if (isSeriesVisible(seriesLabel))
            {
                QColor seriesColor = getSeriesColor(seriesLabel);
                
                if (seriesLabel == "ADOPTED")
                {
                    // Draw curve for ADOPTED series without points (only on full redraw)
                    if (needsFullClear)
                    {
                        DEBUG_OUT() << "LTW: draw() - drawing ADOPTED series as line";
                        drawDataLine(seriesLabel, false);
                    }
                }
                else
                {
                    // Draw custom markers for other series with adaptive sampling
                    // Note: drawCustomMarkers doesn't support incremental yet, so always redraw on full clear
                    if (needsFullClear)
                    {
                        DEBUG_OUT() << "LTW: draw() - drawing custom markers for series:" << seriesLabel;
                        drawCustomMarkers(seriesLabel, seriesColor);
                    }
                }
            }
        }
    }
    else if (dataSource && dataSource->isEmpty())
    {
        // Data source is empty - cleanup all scatterplot items to ensure they're removed
        cleanupAllScatterplotItems();
        DEBUG_OUT() << "LTWGraph: Data source is empty, cleaned up all scatterplot items";
    }
    else
    {
        DEBUG_OUT() << "LTW: draw() - no dataSource or dataSource is empty";
    }
    
    // Draw BTW symbols (magenta circles) if any exist in data source
    // CRITICAL FIX: Draw during both full redraw and incremental updates
    // Symbols need to be redrawn when time range changes (timer ticks, animation, zoom)
    // because their Y positions depend on the time range
    if (needsFullClear || m_renderState == RenderState::RANGE_UPDATE_ONLY || m_renderState == RenderState::INCREMENTAL_UPDATE)
    {
        drawBTWSymbols();
    }
    
    // Reset render state to clean after drawing
    setRenderState(RenderState::CLEAN);
    
    isDrawing = false;
}

/**
 * @brief Handle mouse click events specific to LTW graph
 *
 * @param scenePos Scene position of the click
 */
void LTWGraph::onMouseClick(const QPointF &scenePos)
{
    DEBUG_OUT() << "LTWGraph mouse clicked at scene position:" << scenePos;
    // Call parent implementation
    WaterfallGraph::onMouseClick(scenePos);
}

/**
 * @brief Handle mouse drag events specific to LTW graph
 *
 * @param scenePos Scene position of the drag
 */
void LTWGraph::onMouseDrag(const QPointF &scenePos)
{
    DEBUG_OUT() << "LTWGraph mouse dragged to scene position:" << scenePos;
    // Call parent implementation
    WaterfallGraph::onMouseDrag(scenePos);
}

/**
 * @brief Draw LTW-specific scatterplot
 *
 */
void LTWGraph::drawLTWScatterplot()
{
    // By default, create a scatterplot using the parent's scatterplot functionality
    drawScatterplot(QString("LTW-1"), Qt::green, 4.0, Qt::white);

    DEBUG_OUT() << "LTW scatterplot drawn";
}

/**
 * @brief Draw custom markers for LTW graph with adaptive time-based binning
 * Uses 1/5 of the current time interval as the bin duration for sampling
 *
 * @param seriesLabel The series label to draw markers for
 * @param markerColor The color for the markers
 */
void LTWGraph::drawCustomMarkers(const QString &seriesLabel, const QColor &markerColor)
{
    if (!dataSource || !graphicsScene) {
        DEBUG_OUT() << "LTW: drawCustomMarkers early return - no dataSource or graphicsScene";
        return;
    }

    // Get total data size for comparison
    size_t totalDataSize = dataSource->getDataSeriesSize(seriesLabel);
    DEBUG_OUT() << "LTW: drawCustomMarkers called for series" << seriesLabel << "with total data size:" << totalDataSize;

    if (totalDataSize == 0) {
        DEBUG_OUT() << "LTW: No data available for series" << seriesLabel;
        return;
    }

    // Use the static binning method to sample data based on time intervals
    qint64 samplingIntervalMs = 300000; // 3 seconds

    // Convert to QTime for the binning method
    QTime binDuration = QTime(0, 0, 0).addMSecs(samplingIntervalMs);
    
    // Get raw data and use static binning method
    const std::vector<qreal>& yData = dataSource->getYDataSeries(seriesLabel);
    const std::vector<QDateTime>& timestamps = dataSource->getTimestampsSeries(seriesLabel);
    std::vector<std::pair<qreal, QDateTime>> binnedData = WaterfallData::binDataByTime(yData, timestamps, binDuration);
    
    // Filter binned data to only include points within the visible time range
    std::vector<std::pair<qreal, QDateTime>> visibleBinnedData;
    for (const auto& point : binnedData) {
        if (point.second >= timeMin && point.second <= timeMax) {
            visibleBinnedData.push_back(point);
        }
    }
    
    DEBUG_OUT() << "LTW: Time range filtering - Total binned:" << binnedData.size() 
             << "- Visible binned:" << visibleBinnedData.size()
             << "- Time range:" << timeMin.toString() << "to" << timeMax.toString();

    DEBUG_OUT() << "LTW: Binning completed for series" << seriesLabel 
             << "- Total data:" << totalDataSize 
             << "- Binned data:" << binnedData.size()
             << "- Visible binned data:" << visibleBinnedData.size()
             << "- Bin duration:" << samplingIntervalMs << "ms";

    // Check if time range is valid and reasonable before drawing markers
    // Use the robust helper function that checks validity, range size, and reasonableness
    if (!isTimeRangeValidForDrawing()) {
        DEBUG_OUT() << "LTW: Time range is invalid or unreasonable - skipping marker drawing until time range is properly set";
        DEBUG_OUT() << "LTW: timeMin:" << timeMin.toString() << "valid:" << timeMin.isValid();
        DEBUG_OUT() << "LTW: timeMax:" << timeMax.toString() << "valid:" << timeMax.isValid();
        DEBUG_OUT() << "LTW: customTimeRangeEnabled:" << customTimeRangeEnabled;
        return;
    }

    if (visibleBinnedData.empty()) {
        DEBUG_OUT() << "LTW: No visible binned data available for series" << seriesLabel;
        DEBUG_OUT() << "LTW: Time range is valid but no data points within range - skipping marker drawing";
        return;
    }

    // Draw markers for each visible binned point
    int markersDrawn = 0;
    for (const auto& point : visibleBinnedData) {
        qreal yValue = point.first;
        QDateTime timestamp = point.second;
        QPointF screenPos = mapDataToScreen(yValue, timestamp);
        
        // Check if point is within visible area
        if (drawingArea.contains(screenPos)) {
            // Calculate marker sizes
            QSize windowSize = this->size();
            qreal squareSize = std::min(0.05 * windowSize.width(), 12.0); // Cap at 12 pixels
            qreal triangleSize = squareSize * 0.5; // Triangle is half the width of square
            
            // Draw cyan square border
            QGraphicsRectItem *square = new QGraphicsRectItem(
                screenPos.x() - squareSize/2, 
                screenPos.y() - squareSize/2, 
                squareSize, 
                squareSize
            );
            square->setPen(QPen(Qt::white, 1.0));
            square->setBrush(QBrush(Qt::transparent));
            square->setZValue(500); // Lower z-value than triangle
            graphicsScene->addItem(square);
            
            // Draw cyan triangle
            QPolygonF triangle;
            triangle << QPointF(screenPos.x(), screenPos.y() - triangleSize/2)  // Top point
                     << QPointF(screenPos.x() - triangleSize/2, screenPos.y() + triangleSize/2)  // Bottom left
                     << QPointF(screenPos.x() + triangleSize/2, screenPos.y() + triangleSize/2); // Bottom right
            
            QGraphicsPolygonItem *triangleItem = new QGraphicsPolygonItem(triangle);
            triangleItem->setPen(QPen(Qt::white, 1.0));
            triangleItem->setBrush(QBrush(Qt::white));
            triangleItem->setZValue(600); // Higher z-value than square
            graphicsScene->addItem(triangleItem);
            
            markersDrawn++;
        }
    }
    
    DEBUG_OUT() << "LTW: Successfully drew" << markersDrawn << "markers for series" << seriesLabel;
}

