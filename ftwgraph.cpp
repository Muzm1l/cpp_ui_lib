#include "ftwgraph.h"
#include "debugutils.h"
#include <QDebug>

/**
 * @brief Construct a new FTWGraph::FTWGraph object
 *
 * @param parent Parent widget
 * @param enableGrid Whether to enable grid display
 * @param gridDivisions Number of grid divisions
 * @param timeInterval Time interval for the waterfall display
 */
FTWGraph::FTWGraph(QWidget* parent, bool enableGrid, int gridDivisions, TimeInterval timeInterval)
    : WaterfallGraph(parent, enableGrid, gridDivisions, timeInterval)
{
    DEBUG_OUT() << "FTWGraph constructor called";
}

/**
 * @brief Destroy the FTWGraph::FTWGraph object
 *
 */
FTWGraph::~FTWGraph()
{
    DEBUG_OUT() << "FTWGraph destructor called";
}

/**
 * @brief Override draw method to create scatterplots by default
 *
 */
void FTWGraph::draw()
{
    if (!graphicsScene)
        return;
    
    // Prevent concurrent drawing to avoid marker duplication
    if (isDrawing) {
        DEBUG_OUT() << "FTWGraph: draw() already in progress, skipping";
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
        updateDataRanges();
        
        // Draw scatterplots for each series with their respective colors
        std::vector<QString> seriesLabels = dataSource->getDataSeriesLabels();
        for (const QString &seriesLabel : seriesLabels)
        {
            if (isSeriesVisible(seriesLabel))
            {
                QColor seriesColor = getSeriesColor(seriesLabel);
                
                if (shouldRenderSeriesAsLine(seriesLabel))
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
        
        DEBUG_OUT() << "FTWGraph: Data source is empty, forced full clear and cleaned up all scatterplot items and data line paths";
    }
    
    // Draw BTW symbols (magenta circles) if any exist in data source
    // CRITICAL FIX: Draw during both full redraw and incremental updates
    // Symbols need to be redrawn when time range changes (timer ticks, animation, zoom)
    // because their Y positions depend on the time range
    if (needsFullClear || m_renderState == RenderState::RANGE_UPDATE_ONLY || m_renderState == RenderState::INCREMENTAL_UPDATE)
    {
        drawBTWSymbols();
        augmentOverlayPassAfterSymbols();
    }
    
    // Reset render state to clean after drawing
    setRenderState(RenderState::CLEAN);
    
    isDrawing = false;
}

/**
 * @brief Handle mouse click events specific to FTW graph
 *
 * @param scenePos Scene position of the click
 */
void FTWGraph::onMouseClick(const QPointF& scenePos)
{
    DEBUG_OUT() << "FTWGraph mouse clicked at scene position:" << scenePos;
    // Call parent implementation
    WaterfallGraph::onMouseClick(scenePos);
}

/**
 * @brief Handle mouse drag events specific to FTW graph
 *
 * @param scenePos Scene position of the drag
 */
void FTWGraph::onMouseDrag(const QPointF& scenePos)
{
    DEBUG_OUT() << "FTWGraph mouse dragged to scene position:" << scenePos;
    // Call parent implementation
    WaterfallGraph::onMouseDrag(scenePos);
}

/**
 * @brief Draw FTW-specific scatterplot
 *
 */
void FTWGraph::drawFTWScatterplot()
{
    // By default, create a scatterplot using the parent's scatterplot functionality
    drawScatterplot(QString("FTW-1"), Qt::white, 4.0, Qt::black);

    DEBUG_OUT() << "FTW scatterplot drawn";
}
