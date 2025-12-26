#include "fdwgraph.h"
#include "debugutils.h"
#include <QDebug>

/**
 * @brief Construct a new FDWGraph::FDWGraph object
 *
 * @param parent Parent widget
 * @param enableGrid Whether to enable grid display
 * @param gridDivisions Number of grid divisions
 * @param timeInterval Time interval for the waterfall display
 */
FDWGraph::FDWGraph(QWidget *parent, bool enableGrid, int gridDivisions, TimeInterval timeInterval)
    : WaterfallGraph(parent, enableGrid, gridDivisions, timeInterval)
{
    DEBUG_OUT() << "FDWGraph constructor called";
}

/**
 * @brief Destroy the FDWGraph::FDWGraph object
 *
 */
FDWGraph::~FDWGraph()
{
    DEBUG_OUT() << "FDWGraph destructor called";
}

/**
 * @brief Override draw method to create scatterplots by default
 *
 */
void FDWGraph::draw()
{
    if (!graphicsScene)
        return;
    
    // Prevent concurrent drawing to avoid marker duplication
    if (isDrawing) {
        DEBUG_OUT() << "FDWGraph: draw() already in progress, skipping";
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

    // Draw dashed white horizontal line at 0 value (only on full redraw)
    if (needsFullClear)
    {
        drawZeroAxis();
    }

    if (dataSource && !dataSource->isEmpty())
    {
        updateDataRanges();
        
        // Draw scatterplots for each series with their respective colors
        std::vector<QString> seriesLabels = dataSource->getDataSeriesLabels();
        for (const QString &seriesLabel : seriesLabels)
        {
            if (isSeriesVisible(seriesLabel))
            {
                QColor seriesColor = getSeriesColor(seriesLabel);
                
                if (seriesLabel == "ADOPTED")
                {
                    // Draw curve for ADOPTED series without points (only on full redraw)
                    if (needsFullClear)
                    {
                        drawDataLine(seriesLabel, false);
                    }
                }
                else
                {
                    // Draw scatterplot for other series - respects render state internally
                    drawScatterplot(seriesLabel, seriesColor, 4.0, Qt::black);
                }
            }
        }
    }
    else if (dataSource && dataSource->isEmpty())
    {
        // Data source is empty - cleanup all scatterplot items to ensure they're removed
        cleanupAllScatterplotItems();
        DEBUG_OUT() << "FDWGraph: Data source is empty, cleaned up all scatterplot items";
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
 * @brief Handle mouse click events specific to FDW graph
 *
 * @param scenePos Scene position of the click
 */
void FDWGraph::onMouseClick(const QPointF &scenePos)
{
    DEBUG_OUT() << "FDWGraph mouse clicked at scene position:" << scenePos;
    // Call parent implementation
    WaterfallGraph::onMouseClick(scenePos);
}

/**
 * @brief Handle mouse drag events specific to FDW graph
 *
 * @param scenePos Scene position of the drag
 */
void FDWGraph::onMouseDrag(const QPointF &scenePos)
{
    DEBUG_OUT() << "FDWGraph mouse dragged to scene position:" << scenePos;
    // Call parent implementation
    WaterfallGraph::onMouseDrag(scenePos);
}

/**
 * @brief Draw FDW-specific scatterplot
 *
 */
void FDWGraph::drawFDWScatterplot()
{
    // By default, create a scatterplot using the parent's scatterplot functionality
    drawScatterplot(QString("FDW-1"), Qt::cyan, 4.0, Qt::white);

    DEBUG_OUT() << "FDW scatterplot drawn";
}

/**
 * @brief Override drawDataLine to use dashed lines for FDW graph
 *
 */
void FDWGraph::drawDataLine(const QString &seriesLabel, bool plotPoints)
{
    if (!graphicsScene || !dataSource || dataSource->isEmpty() || !dataRangesValid)
    {
        return;
    }

    const auto &yData = dataSource->getYDataSeries(seriesLabel);
    const auto &timestamps = dataSource->getTimestampsSeries(seriesLabel);

    if (yData.empty() || timestamps.empty())
    {
        DEBUG_OUT() << "FDW: drawDataLine - no data available for series" << seriesLabel;
        return;
    }

    if (yData.size() != timestamps.size())
    {
        DEBUG_OUT() << "FDW: drawDataLine - data size mismatch for series" << seriesLabel
                 << "- yData:" << yData.size() << "timestamps:" << timestamps.size();
        return;
    }

    // Use cached visible data for O(k) incremental filtering instead of O(n) full filter
    // Cache is automatically updated when time range or data changes
    if (!isVisibleDataCacheValid(seriesLabel))
    {
        // Check if we can do incremental update (same time range, just new data)
        auto rangeIt = m_cachedTimeRange.find(seriesLabel);
        if (rangeIt != m_cachedTimeRange.end() &&
            rangeIt->second.first == timeMin && rangeIt->second.second == timeMax)
        {
            updateVisibleDataCacheIncremental(seriesLabel);
        }
        else
        {
            updateVisibleDataCacheFull(seriesLabel);
        }
    }

    const std::vector<std::pair<qreal, QDateTime>> &visibleData = m_cachedVisibleData[seriesLabel];

    if (visibleData.empty())
    {
        DEBUG_OUT() << "FDW: drawDataLine - no visible points within current time range for series" << seriesLabel;
        return;
    }

    QColor seriesColor = getSeriesColor(seriesLabel);

    if (visibleData.size() < 2)
    {
        // Draw a single point if we only have one data point
        QPointF screenPoint = mapDataToScreen(visibleData[0].first, visibleData[0].second);
        QPen pointPen(seriesColor, 0); // No stroke (width 0)
        graphicsScene->addEllipse(screenPoint.x() - 2, screenPoint.y() - 2, 4, 4, pointPen);
        DEBUG_OUT() << "FDW data line drawn (dashed) for series" << seriesLabel << "with 1 visible point";
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

    // Draw the line with dashed style
    QPen linePen(seriesColor, 2);
    linePen.setStyle(Qt::DashLine);
    linePen.setDashPattern({8, 4}); // Custom dash pattern: 8px dash, 4px gap
    graphicsScene->addPath(path, linePen);

    // Draw data points if enabled
    if (plotPoints)
    {
        // Draw data points
        QPen pointPen(seriesColor, 0); // No stroke (width 0)
        for (const auto &dataPoint : visibleData)
        {
            QPointF point = mapDataToScreen(dataPoint.first, dataPoint.second);
            graphicsScene->addEllipse(point.x() - 1, point.y() - 1, 2, 2, pointPen);
        }
    }

    DEBUG_OUT() << "FDW data line drawn (dashed) for series" << seriesLabel << "with" << visibleData.size()
             << "visible points out of" << yData.size() << "total points";
}

/**
 * @brief Draw dashed white vertical line at 0 value
 *
 */
void FDWGraph::drawZeroAxis()
{
    if (!graphicsScene) {
        return;
    }

    // Map zero axis value (zoom panel middle sticker value) to screen coordinates using current time as timestamp
    QDateTime currentTime = QDateTime::currentDateTime();
    QPointF zeroPoint = mapDataToScreen(m_zeroAxisValue, currentTime);
    
    // Create vertical line from top to bottom of drawing area at x = 0
    QPointF topPoint(zeroPoint.x(), drawingArea.top());
    QPointF bottomPoint(zeroPoint.x(), drawingArea.bottom());
    
    // Create dashed white pen
    QPen zeroAxisPen(QColor(255, 255, 255), 1.0, Qt::DashLine); // White dashed line
    zeroAxisPen.setDashPattern({8, 4}); // Custom dash pattern: 8px dash, 4px gap
    
    // Draw the vertical line
    graphicsScene->addLine(QLineF(topPoint, bottomPoint), zeroAxisPen);
    
    DEBUG_OUT() << "FDW zero axis drawn at x:" << zeroPoint.x();
}
