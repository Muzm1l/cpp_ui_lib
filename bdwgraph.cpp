#include "bdwgraph.h"
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
    qDebug() << "BDWGraph constructor called";
}

/**
 * @brief Destroy the BDWGraph::BDWGraph object
 *
 */
BDWGraph::~BDWGraph()
{
    qDebug() << "BDWGraph destructor called";
}

/**
 * @brief Override draw method to create scatterplots by default
 *
 */
void BDWGraph::draw()
{
    if (!graphicsScene)
        return;

    graphicsScene->clear();
    setupDrawingArea();

    if (gridEnabled)
    {
        drawGrid();
    }

    // Draw dashed grey vertical axis at 0 value
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
}

/**
 * @brief Handle mouse click events specific to BDW graph
 *
 * @param scenePos Scene position of the click
 */
void BDWGraph::onMouseClick(const QPointF &scenePos)
{
    qDebug() << "BDWGraph mouse clicked at scene position:" << scenePos;
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
    qDebug() << "BDWGraph mouse dragged to scene position:" << scenePos;
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

    qDebug() << "BDW scatterplot drawn";
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

    // Map 0 value to screen coordinates using current time as timestamp
    QDateTime currentTime = QDateTime::currentDateTime();
    QPointF zeroPoint = mapDataToScreen(0.0, currentTime);
    
    // Create vertical line from top to bottom of drawing area at x = 0
    QPointF topPoint(zeroPoint.x(), drawingArea.top());
    QPointF bottomPoint(zeroPoint.x(), drawingArea.bottom());
    
    // Create dashed white pen with more spacing
    QPen zeroAxisPen(QColor(255, 255, 255), 1.0, Qt::DashLine); // White dashed line
    zeroAxisPen.setDashPattern({8, 4}); // Custom dash pattern: 8px dash, 4px gap
    
    // Draw the vertical line
    graphicsScene->addLine(QLineF(topPoint, bottomPoint), zeroAxisPen);
    
    qDebug() << "BDW zero axis drawn at x:" << zeroPoint.x();
}
