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

    graphicsScene->clear();
    setupDrawingArea();

    if (gridEnabled)
    {
        drawGrid();
    }

    if (dataSource && !dataSource->isEmpty())
    {
        updateDataRanges();
        drawAllDataSeries(); // Draw all series with their respective colors
    }
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
    drawScatterplot(Qt::yellow, 4.0, Qt::black);

    qDebug() << "BRW scatterplot drawn";
}
