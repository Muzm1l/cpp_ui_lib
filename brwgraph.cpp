#include "brwgraph.h"
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
    : WaterfallGraph(parent, enableGrid, gridDivisions, timeInterval)
{
    qDebug() << "BRWGraph constructor called";
}

/**
 * @brief Destroy the BRWGraph::BRWGraph object
 *
 */
BRWGraph::~BRWGraph()
{
    qDebug() << "BRWGraph destructor called";
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
        qDebug() << "BRWGraph: draw() already in progress, skipping";
        return;
    }
    
    isDrawing = true;

    graphicsScene->clear();
    graphicsScene->update(); // Force immediate update to ensure clearing is visible
    setupDrawingArea();

    if (gridEnabled)
    {
        drawGrid();
    }

    // Draw dashed white horizontal line at 0 value
    drawZeroAxis();

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
                    // Draw curve for ADOPTED series without points
                    drawDataLine(seriesLabel, false);
                }
                else
                {
                    // Draw scatterplot for other series
                    drawScatterplot(seriesLabel, seriesColor, 3.0, Qt::black);
                }
            }
        }
    }
    
    // Draw BTW symbols (magenta circles) if any exist in data source
    drawBTWSymbols();
    
    isDrawing = false;
}

/**
 * @brief Handle mouse click events specific to BRW graph
 *
 * @param scenePos Scene position of the click
 */
void BRWGraph::onMouseClick(const QPointF &scenePos)
{
    qDebug() << "BRWGraph mouse clicked at scene position:" << scenePos;
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
    qDebug() << "BRWGraph mouse dragged to scene position:" << scenePos;
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

    qDebug() << "BRW scatterplot drawn";
}

/**
 * @brief Draw dashed white vertical line at 0 value
 *
 */
void BRWGraph::drawZeroAxis()
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
    
    qDebug() << "BRW zero axis drawn at x:" << zeroPoint.x();
}
