#include "fdwgraph.h"
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
    : WaterfallGraph(parent, enableGrid, gridDivisions, timeInterval)
{
    qDebug() << "FDWGraph constructor called";
}

/**
 * @brief Destroy the FDWGraph::FDWGraph object
 * 
 */
FDWGraph::~FDWGraph()
{
    qDebug() << "FDWGraph destructor called";
}

/**
 * @brief Override draw method to create scatterplots by default
 * 
 */
void FDWGraph::draw()
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
        drawScatterplot(Qt::cyan, 3.0, Qt::white); // Default scatterplot for FDW
    }
}

/**
 * @brief Handle mouse click events specific to FDW graph
 * 
 * @param scenePos Scene position of the click
 */
void FDWGraph::onMouseClick(const QPointF& scenePos)
{
    qDebug() << "FDWGraph mouse clicked at scene position:" << scenePos;
    // Call parent implementation
    WaterfallGraph::onMouseClick(scenePos);
}

/**
 * @brief Handle mouse drag events specific to FDW graph
 * 
 * @param scenePos Scene position of the drag
 */
void FDWGraph::onMouseDrag(const QPointF& scenePos)
{
    qDebug() << "FDWGraph mouse dragged to scene position:" << scenePos;
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
    drawScatterplot(Qt::cyan, 4.0, Qt::white);
    
    qDebug() << "FDW scatterplot drawn";
}
