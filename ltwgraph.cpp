#include "ltwgraph.h"
#include <QDebug>

/**
 * @brief Construct a new LTWGraph::LTWGraph object
 * 
 * @param parent Parent widget
 * @param enableGrid Whether to enable grid display
 * @param gridDivisions Number of grid divisions
 * @param timeInterval Time interval for the waterfall display
 */
LTWGraph::LTWGraph(QWidget *parent, bool enableGrid, int gridDivisions, TimeInterval timeInterval)
    : WaterfallGraph(parent, enableGrid, gridDivisions, timeInterval)
{
    qDebug() << "LTWGraph constructor called";
}

/**
 * @brief Destroy the LTWGraph::LTWGraph object
 * 
 */
LTWGraph::~LTWGraph()
{
    qDebug() << "LTWGraph destructor called";
}

/**
 * @brief Override draw method to create scatterplots by default
 * 
 */
void LTWGraph::draw()
{
    if (!graphicsScene)
        return;

    graphicsScene->clear();
    setupDrawingArea();

    if (gridEnabled) {
        drawGrid();
    }

    if (dataSource && !dataSource->isEmpty()) {
        updateDataRanges();
        drawScatterplot(Qt::green, 3.0, Qt::white); // Default scatterplot for LTW
    }
}

/**
 * @brief Handle mouse click events specific to LTW graph
 * 
 * @param scenePos Scene position of the click
 */
void LTWGraph::onMouseClick(const QPointF& scenePos)
{
    qDebug() << "LTWGraph mouse clicked at scene position:" << scenePos;
    // Call parent implementation
    WaterfallGraph::onMouseClick(scenePos);
}

/**
 * @brief Handle mouse drag events specific to LTW graph
 * 
 * @param scenePos Scene position of the drag
 */
void LTWGraph::onMouseDrag(const QPointF& scenePos)
{
    qDebug() << "LTWGraph mouse dragged to scene position:" << scenePos;
    // Call parent implementation
    WaterfallGraph::onMouseDrag(scenePos);
}

/**
 * @brief Draw LTW-specific scatterplot
 * 
 */
void LTWGraph::drawLTWScatterplot()
{
    // By default, create a scatterplot using the parent's scatterplot functionality
    drawScatterplot(Qt::green, 4.0, Qt::white);
    
    qDebug() << "LTW scatterplot drawn";
}
