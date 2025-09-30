#include "rtwgraph.h"
#include <QDebug>

/**
 * @brief Construct a new RTWGraph::RTWGraph object
 *
 * @param parent Parent widget
 * @param enableGrid Whether to enable grid display
 * @param gridDivisions Number of grid divisions
 * @param timeInterval Time interval for the waterfall display
 */
RTWGraph::RTWGraph(QWidget *parent, bool enableGrid, int gridDivisions, TimeInterval timeInterval)
    : WaterfallGraph(parent, enableGrid, gridDivisions, timeInterval)
{
    qDebug() << "RTWGraph constructor called";
}

/**
 * @brief Destroy the RTWGraph::RTWGraph object
 *
 */
RTWGraph::~RTWGraph()
{
    qDebug() << "RTWGraph destructor called";
}

/**
 * @brief Override draw method to create scatterplots by default
 *
 */
void RTWGraph::draw()
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
        drawScatterplot(Qt::blue, 3.0, Qt::white); // Default scatterplot for RTW
    }
}

/**
 * @brief Handle mouse click events specific to RTW graph
 *
 * @param scenePos Scene position of the click
 */
void RTWGraph::onMouseClick(const QPointF &scenePos)
{
    qDebug() << "RTWGraph mouse clicked at scene position:" << scenePos;
    // Call parent implementation
    WaterfallGraph::onMouseClick(scenePos);
}

/**
 * @brief Handle mouse drag events specific to RTW graph
 *
 * @param scenePos Scene position of the drag
 */
void RTWGraph::onMouseDrag(const QPointF &scenePos)
{
    qDebug() << "RTWGraph mouse dragged to scene position:" << scenePos;
    // Call parent implementation
    WaterfallGraph::onMouseDrag(scenePos);
}

/**
 * @brief Draw RTW-specific scatterplot
 *
 */
void RTWGraph::drawRTWScatterplot()
{
    // By default, create a scatterplot using the parent's scatterplot functionality
    drawScatterplot(Qt::blue, 4.0, Qt::white);

    qDebug() << "RTW scatterplot drawn";
}
