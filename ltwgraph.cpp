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
#include "ltwgraph.h"
#include <QDebug>
#include <QMouseEvent>
#include <QEnterEvent>

/**
 * @brief Construct a new LTWGraph::LTWGraph object
 *
 * @param parent Parent widget
 * @param enableGrid Whether to enable grid display
 * @param gridDivisions Number of grid divisions
 * @param timeInterval Time interval for the waterfall display
 */
LTWGraph::LTWGraph(QWidget *parent, bool enableGrid, int gridDivisions, TimeInterval timeInterval)
    : WaterfallGraph(parent, enableGrid, gridDivisions, timeInterval),
      crosshairHorizontal(nullptr),
      crosshairVertical(nullptr),
      crosshairVisible(false)
{
    qDebug() << "LTWGraph constructor called";
    
    // Setup crosshair after the parent constructor has initialized the overlay scene
    setupCrosshair();
}

/**
 * @brief Destroy the LTWGraph::LTWGraph object
 *
 */
LTWGraph::~LTWGraph()
{
    qDebug() << "LTWGraph destructor called";
    
    // Clean up crosshair items
    if (crosshairHorizontal) {
        delete crosshairHorizontal;
        crosshairHorizontal = nullptr;
    }
    if (crosshairVertical) {
        delete crosshairVertical;
        crosshairVertical = nullptr;
    }
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

    if (gridEnabled)
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
 * @brief Handle mouse click events specific to LTW graph
 *
 * @param scenePos Scene position of the click
 */
void LTWGraph::onMouseClick(const QPointF &scenePos)
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
void LTWGraph::onMouseDrag(const QPointF &scenePos)
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
    drawScatterplot(QString("LTW-1"), Qt::green, 4.0, Qt::white);

    qDebug() << "LTW scatterplot drawn";
}

/**
 * @brief Handle mouse move events to update crosshair position
 *
 * @param event Mouse move event
 */
void LTWGraph::mouseMoveEvent(QMouseEvent *event)
{
    // Call parent implementation first
    WaterfallGraph::mouseMoveEvent(event);
    
    // Update crosshair position for all mouse movement (not just dragging)
    if (crosshairVisible && overlayScene) {
        QPointF scenePos = overlayView->mapToScene(event->pos());
        updateCrosshair(scenePos);
    }
}

/**
 * @brief Handle mouse enter events to show crosshair
 *
 * @param event Mouse enter event
 */
void LTWGraph::enterEvent(QEnterEvent *event)
{
    // Call parent implementation first
    WaterfallGraph::enterEvent(event);
    
    // Show crosshair when mouse enters
    showCrosshair();
}

/**
 * @brief Handle mouse leave events to hide crosshair
 *
 * @param event Mouse leave event
 */
void LTWGraph::leaveEvent(QEvent *event)
{
    // Call parent implementation first
    WaterfallGraph::leaveEvent(event);
    
    // Hide crosshair when mouse leaves
    hideCrosshair();
}

/**
 * @brief Setup crosshair graphics items
 *
 */
void LTWGraph::setupCrosshair()
{
    if (!overlayScene) {
        qDebug() << "LTWGraph: Overlay scene not available for crosshair setup";
        return;
    }
    
    // Create horizontal crosshair line
    crosshairHorizontal = new QGraphicsLineItem();
    crosshairHorizontal->setPen(QPen(Qt::cyan, 1.0, Qt::SolidLine));
    crosshairHorizontal->setZValue(1000); // High z-value to appear on top
    crosshairHorizontal->setVisible(false);
    overlayScene->addItem(crosshairHorizontal);
    
    // Create vertical crosshair line
    crosshairVertical = new QGraphicsLineItem();
    crosshairVertical->setPen(QPen(Qt::cyan, 1.0, Qt::SolidLine));
    crosshairVertical->setZValue(1000); // High z-value to appear on top
    crosshairVertical->setVisible(false);
    overlayScene->addItem(crosshairVertical);
    
    qDebug() << "LTWGraph: Crosshair setup completed";
}

/**
 * @brief Update crosshair position based on mouse position
 *
 * @param mousePos Mouse position in scene coordinates
 */
void LTWGraph::updateCrosshair(const QPointF &mousePos)
{
    if (!crosshairHorizontal || !crosshairVertical || !overlayScene) {
        return;
    }
    
    // Get the scene rectangle
    QRectF sceneRect = overlayScene->sceneRect();
    
    // Update horizontal line (left to right)
    crosshairHorizontal->setLine(sceneRect.left(), mousePos.y(), sceneRect.right(), mousePos.y());
    
    // Update vertical line (top to bottom)
    crosshairVertical->setLine(mousePos.x(), sceneRect.top(), mousePos.x(), sceneRect.bottom());
}

/**
 * @brief Show the crosshair
 *
 */
void LTWGraph::showCrosshair()
{
    if (crosshairHorizontal && crosshairVertical) {
        crosshairHorizontal->setVisible(true);
        crosshairVertical->setVisible(true);
        crosshairVisible = true;
        qDebug() << "LTWGraph: Crosshair shown";
    }
}

/**
 * @brief Hide the crosshair
 *
 */
void LTWGraph::hideCrosshair()
{
    if (crosshairHorizontal && crosshairVertical) {
        crosshairHorizontal->setVisible(false);
        crosshairVertical->setVisible(false);
        crosshairVisible = false;
        qDebug() << "LTWGraph: Crosshair hidden";
    }
}
