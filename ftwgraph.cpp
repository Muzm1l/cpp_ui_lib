#include "ftwgraph.h"
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
    qDebug() << "FTWGraph constructor called";
}

/**
 * @brief Destroy the FTWGraph::FTWGraph object
 *
 */
FTWGraph::~FTWGraph()
{
    qDebug() << "FTWGraph destructor called";
}

/**
 * @brief Override draw method to create scatterplots by default
 *
 */
void FTWGraph::draw()
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
 * @brief Handle mouse click events specific to FTW graph
 *
 * @param scenePos Scene position of the click
 */
void FTWGraph::onMouseClick(const QPointF& scenePos)
{
    qDebug() << "FTWGraph mouse clicked at scene position:" << scenePos;
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
    qDebug() << "FTWGraph mouse dragged to scene position:" << scenePos;
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

    qDebug() << "FTW scatterplot drawn";
}
