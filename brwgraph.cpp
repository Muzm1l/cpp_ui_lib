#include "brwgraph.h"
#include "debugutils.h"
#include <QDebug>

/**
 * @brief Construct a new BRWGraph::BRWGraph object
 *
 * @param parent Parent widget
 * @param enableGrid Whether to enable grid display
 * @param gridDivisions Number of grid divisions
 * @param timeInterval Time interval for the waterfall display
 */
BRWGraph::BRWGraph(QWidget *parent, bool enableGrid, int gridDivisions, TimeInterval timeInterval)
    : WaterfallGraph(parent, enableGrid, gridDivisions, timeInterval),
      m_zeroAxisLineItem(nullptr)
{
    DEBUG_OUT() << "BRWGraph constructor called";
}

/**
 * @brief Destroy the BRWGraph::BRWGraph object
 *
 */
BRWGraph::~BRWGraph()
{
    DEBUG_OUT() << "BRWGraph destructor called";
}

/**
 * @brief Override draw method to create scatterplots by default
 *
 */
void BRWGraph::draw()
{
    if (!graphicsScene)
        return;
    
    // Prevent concurrent drawing to avoid marker duplication
    if (isDrawing) {
        DEBUG_OUT() << "BRWGraph: draw() already in progress, skipping";
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
        
        // Clear zero axis line from overlayScene on full redraw (layout changes, graph switches)
        if (m_zeroAxisLineItem) {
            QGraphicsScene *itemScene = m_zeroAxisLineItem->scene();
            if (itemScene && itemScene == overlayScene) {
                overlayScene->removeItem(m_zeroAxisLineItem);
            }
            delete m_zeroAxisLineItem;
            m_zeroAxisLineItem = nullptr;
        }
        
        graphicsScene->clear();
        graphicsScene->update(); // Force immediate update to ensure clearing is visible
    }
    
    setupDrawingArea();

    if (needsFullClear && gridEnabled)
    {
        drawGrid();
    }

    // Draw dashed white vertical line at 0 value (update on full redraw, range updates, incremental updates, and when line doesn't exist)
    // Range updates happen when zoom panel changes, so we need to update the line position
    // Incremental updates happen when time range changes (timer ticks, animation), so line position needs updating
    // Also draw if line doesn't exist yet (e.g., when graph is first selected)
    if (needsFullClear || m_renderState == RenderState::RANGE_UPDATE_ONLY || m_renderState == RenderState::INCREMENTAL_UPDATE || !m_zeroAxisLineItem)
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
                    // Draw ADOPTED series as solid line (no points)
                    // Draw during both full redraw and incremental updates
                    if (needsFullClear || m_renderState == RenderState::RANGE_UPDATE_ONLY || m_renderState == RenderState::INCREMENTAL_UPDATE)
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
        DEBUG_OUT() << "BRWGraph: Data source is empty, cleaned up all scatterplot items";
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
 * @brief Handle mouse click events specific to BRW graph
 *
 * @param scenePos Scene position of the click
 */
void BRWGraph::onMouseClick(const QPointF &scenePos)
{
    DEBUG_OUT() << "BRWGraph mouse clicked at scene position:" << scenePos;
    // Call parent implementation
    WaterfallGraph::onMouseClick(scenePos);
}

/**
 * @brief Handle mouse drag events specific to BRW graph
 *
 * @param scenePos Scene position of the drag
 */
void BRWGraph::onMouseDrag(const QPointF &scenePos)
{
    DEBUG_OUT() << "BRWGraph mouse dragged to scene position:" << scenePos;
    // Call parent implementation
    WaterfallGraph::onMouseDrag(scenePos);
}

/**
 * @brief Draw BRW-specific scatterplot
 *
 */
void BRWGraph::drawBRWScatterplot()
{
    // By default, create a scatterplot using the parent's scatterplot functionality
    drawScatterplot(QString("BRW-1"), Qt::yellow, 4.0, Qt::black);

    DEBUG_OUT() << "BRW scatterplot drawn";
}

/**
 * @brief Draw dashed white vertical line at 0 value
 *
 */
void BRWGraph::drawZeroAxis()
{
    if (!overlayScene) {
        return;
    }

    // Remove old line if it exists to prevent duplication
    if (m_zeroAxisLineItem) {
        // Safety check: Verify item is still valid and in the scene before removing
        // Check if item has a scene and it matches overlayScene
        QGraphicsScene *itemScene = m_zeroAxisLineItem->scene();
        if (itemScene && itemScene == overlayScene) {
            overlayScene->removeItem(m_zeroAxisLineItem);
        }
        delete m_zeroAxisLineItem;
        m_zeroAxisLineItem = nullptr;
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
    
    // Draw the vertical line and store reference for future updates
    m_zeroAxisLineItem = overlayScene->addLine(QLineF(topPoint, bottomPoint), zeroAxisPen);
    
    DEBUG_OUT() << "BRW zero axis drawn at x:" << zeroPoint.x();
}