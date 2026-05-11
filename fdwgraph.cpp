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
    : WaterfallGraph(parent, enableGrid, gridDivisions, timeInterval),
      m_zeroAxisLineItem(nullptr)
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
        // CRITICAL FIX: When data is empty, force full clear to ensure graphics scene is cleared
        // This prevents old drawn elements from remaining visible when empty data is passed
        if (!needsFullClear)
        {
            // Clear all item pointers since we're about to clear the scene
            m_seriesScatterplotItems.clear();
            m_seriesPathItems.clear();
            m_seriesPointItems.clear();
            
            // Clear graphics scene to remove all drawn elements
            graphicsScene->clear();
            graphicsScene->update(); // Force immediate update to ensure clearing is visible
        }
        
        // Data source is empty - cleanup all scatterplot items to ensure they're removed
        cleanupAllScatterplotItems();
        
        // CRITICAL FIX: Clear data line paths (ADOPTED series line)
        // These paths are rendered in paintEvent() and may contain gaps from when
        // BTW symbols were present. When data is cleared, these old paths must be
        // cleared too, otherwise the line with gaps remains visible.
        m_dataLinePaths.clear();
        m_batchedLinePaths.clear();
        m_dataLineColors.clear();
        
        // Trigger repaint to clear the line from screen
        update();
        
        DEBUG_OUT() << "FDWGraph: Data source is empty, forced full clear and cleaned up all scatterplot items and data line paths";
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

// drawDataLine() override removed - now uses base class which draws solid lines for ADOPTED

/**
 * @brief Draw / reposition the dashed white vertical axis at m_zeroAxisValue.
 *
 * Reuses the existing QGraphicsLineItem via setLine() when possible, so the
 * per-frame zoom path only pays an O(1) bounding-rect update instead of a
 * scene-graph tear-down + addLine() on every tick.
 */
void FDWGraph::drawZeroAxis()
{
    if (!overlayScene) {
        return;
    }

    const QDateTime currentTime = QDateTime::currentDateTime();
    const QPointF zeroPoint = mapDataToScreen(m_zeroAxisValue, currentTime);
    const QLineF newLine(QPointF(zeroPoint.x(), drawingArea.top()),
                         QPointF(zeroPoint.x(), drawingArea.bottom()));

    if (m_zeroAxisLineItem && m_zeroAxisLineItem->scene() == overlayScene) {
        m_zeroAxisLineItem->setLine(newLine);
        return;
    }

    if (m_zeroAxisLineItem) {
        delete m_zeroAxisLineItem;
        m_zeroAxisLineItem = nullptr;
    }

    QPen zeroAxisPen(QColor(255, 255, 255), 1.0, Qt::DashLine);
    zeroAxisPen.setDashPattern({8, 4});
    m_zeroAxisLineItem = overlayScene->addLine(newLine, zeroAxisPen);

    DEBUG_OUT() << "FDW zero axis drawn at x:" << zeroPoint.x();
}

void FDWGraph::refreshOverlaysAfterVisibleTimeRangeChange()
{
    // Hit by ZoomPanel live drag via the RANGE_UPDATE_ONLY branch.
    drawZeroAxis();
}

void FDWGraph::augmentOverlayPassAfterSymbols()
{
    // Hit by INCREMENTAL_UPDATE / FULL_REDRAW branches after BTW symbols.
    drawZeroAxis();
}
