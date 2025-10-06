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
        
        // Draw custom markers for each series with their respective colors
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
                    // Draw custom markers for other series with 1/5 sampling
                    drawCustomMarkers(seriesLabel, seriesColor);
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
 * @brief Draw custom markers for LTW graph with 1/5 sampling
 *
 * @param seriesLabel The series label to draw markers for
 * @param markerColor The color for the markers
 */
void LTWGraph::drawCustomMarkers(const QString &seriesLabel, const QColor &markerColor)
{
    if (!dataSource || !graphicsScene) {
        return;
    }

    // Get data for the series
    std::vector<qreal> yData = dataSource->getYDataSeries(seriesLabel);
    std::vector<QDateTime> timestamps = dataSource->getTimestampsSeries(seriesLabel);

    if (yData.empty() || timestamps.empty()) {
        return;
    }

    // Apply 1/5 sampling (every 5th point)
    std::vector<qreal> sampledYData;
    std::vector<QDateTime> sampledTimestamps;
    
    for (size_t i = 0; i < yData.size(); i += 5) {
        sampledYData.push_back(yData[i]);
        sampledTimestamps.push_back(timestamps[i]);
    }

    qDebug() << "LTW: Drawing custom markers for series" << seriesLabel 
             << "with" << sampledYData.size() << "sampled points out of" << yData.size() << "total points";

    // Draw markers for each sampled point
    for (size_t i = 0; i < sampledYData.size(); ++i) {
        QPointF screenPos = mapDataToScreen(sampledYData[i], sampledTimestamps[i]);
        
        // Check if point is within visible area
        if (drawingArea.contains(screenPos)) {
            // Calculate marker sizes
            qreal squareSize = 12.0; // Base square size (3x larger: 4.0 * 3)
            qreal triangleSize = squareSize * 0.5; // Triangle is half the width of square
            qreal padding = triangleSize * 0.05; // 5% padding
            
            // Draw cyan square border
            QGraphicsRectItem *square = new QGraphicsRectItem(
                screenPos.x() - squareSize/2, 
                screenPos.y() - squareSize/2, 
                squareSize, 
                squareSize
            );
            square->setPen(QPen(Qt::cyan, 1.0));
            square->setBrush(QBrush(Qt::transparent));
            square->setZValue(500); // Lower z-value than triangle
            graphicsScene->addItem(square);
            
            // Draw cyan triangle
            QPolygonF triangle;
            triangle << QPointF(screenPos.x(), screenPos.y() - triangleSize/2)  // Top point
                     << QPointF(screenPos.x() - triangleSize/2, screenPos.y() + triangleSize/2)  // Bottom left
                     << QPointF(screenPos.x() + triangleSize/2, screenPos.y() + triangleSize/2); // Bottom right
            
            QGraphicsPolygonItem *triangleItem = new QGraphicsPolygonItem(triangle);
            triangleItem->setPen(QPen(Qt::cyan, 1.0));
            triangleItem->setBrush(QBrush(Qt::cyan));
            triangleItem->setZValue(600); // Higher z-value than square
            graphicsScene->addItem(triangleItem);
        }
    }
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
