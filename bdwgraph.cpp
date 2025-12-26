#include "bdwgraph.h"
#include "debugutils.h"
#include <QDebug>

/**
 * @brief Construct a new BDWGraph::BDWGraph object
 *
 * @param parent Parent widget
 * @param enableGrid Whether to enable grid display
 * @param gridDivisions Number of grid divisions
 * @param timeInterval Time interval for the waterfall display
 */
BDWGraph::BDWGraph(QWidget *parent, bool enableGrid, int gridDivisions, TimeInterval timeInterval)
    : WaterfallGraph(parent, enableGrid, gridDivisions, timeInterval)
{
    DEBUG_OUT() << "BDWGraph constructor called";
}

/**
 * @brief Destroy the BDWGraph::BDWGraph object
 *
 */
BDWGraph::~BDWGraph()
{
    DEBUG_OUT() << "BDWGraph destructor called";
}

/**
 * @brief Override draw method to create scatterplots by default
 *
 */
void BDWGraph::draw()
{
    if (!graphicsScene)
        return;
    
    // Prevent concurrent drawing to avoid marker duplication
    if (isDrawing) {
        DEBUG_OUT() << "BDWGraph: draw() already in progress, skipping";
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

    // Draw dashed grey vertical axis at 0 value (only on full redraw)
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
        DEBUG_OUT() << "BDWGraph: Data source is empty, cleaned up all scatterplot items";
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
 * @brief Handle mouse click events specific to BDW graph
 *
 * @param scenePos Scene position of the click
 */
void BDWGraph::onMouseClick(const QPointF &scenePos)
{
    DEBUG_OUT() << "BDWGraph mouse clicked at scene position:" << scenePos;
    // Call parent implementation
    WaterfallGraph::onMouseClick(scenePos);
}

/**
 * @brief Handle mouse drag events specific to BDW graph
 *
 * @param scenePos Scene position of the drag
 */
void BDWGraph::onMouseDrag(const QPointF &scenePos)
{
    DEBUG_OUT() << "BDWGraph mouse dragged to scene position:" << scenePos;
    // Call parent implementation
    WaterfallGraph::onMouseDrag(scenePos);
}

/**
 * @brief Draw BDW-specific scatterplot
 *
 */
void BDWGraph::drawBDWScatterplot()
{
    // By default, create a scatterplot using the parent's scatterplot functionality
    // TODO: Change
    drawScatterplot(QString("BDW-1"), Qt::magenta, 4.0, Qt::white);

    DEBUG_OUT() << "BDW scatterplot drawn";
}

/**
 * @brief Draw dashed grey vertical axis at 0 value
 *
 */
void BDWGraph::drawZeroAxis()
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
    
    // Create dashed white pen with more spacing
    QPen zeroAxisPen(QColor(255, 255, 255), 1.0, Qt::DashLine); // White dashed line
    zeroAxisPen.setDashPattern({8, 4}); // Custom dash pattern: 8px dash, 4px gap
    
    // Draw the vertical line
    graphicsScene->addLine(QLineF(topPoint, bottomPoint), zeroAxisPen);
    
    DEBUG_OUT() << "BDW zero axis drawn at x:" << zeroPoint.x();
}

// drawDataLine() override removed - now uses base class which draws solid lines for ADOPTED
