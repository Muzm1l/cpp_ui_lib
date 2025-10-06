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
#include <QtMath>

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
    qDebug() << "LTW: draw() called";
    
    if (!graphicsScene) {
        qDebug() << "LTW: draw() early return - no graphicsScene";
        return;
    }

    graphicsScene->clear();
    setupDrawingArea();

    if (gridEnabled)
    {
        drawGrid();
    }

    if (dataSource && !dataSource->isEmpty())
    {
        qDebug() << "LTW: draw() - dataSource available, updating ranges and drawing series";
        updateDataRanges();
        
        // Draw custom markers for each series with their respective colors
        std::vector<QString> seriesLabels = dataSource->getDataSeriesLabels();
        qDebug() << "LTW: draw() - found" << seriesLabels.size() << "series labels";
        
        for (const QString &seriesLabel : seriesLabels)
        {
            qDebug() << "LTW: draw() - processing series:" << seriesLabel << "visible:" << isSeriesVisible(seriesLabel);
            if (isSeriesVisible(seriesLabel))
            {
                QColor seriesColor = getSeriesColor(seriesLabel);
                
                if (seriesLabel == "ADOPTED")
                {
                    // Draw curve for ADOPTED series without points
                    qDebug() << "LTW: draw() - drawing ADOPTED series as line";
                    drawDataLine(seriesLabel, false);
                }
                else
                {
                    // Draw custom markers for other series with adaptive sampling
                    qDebug() << "LTW: draw() - drawing custom markers for series:" << seriesLabel;
                    drawCustomMarkers(seriesLabel, seriesColor);
                }
            }
        }
    }
    else
    {
        qDebug() << "LTW: draw() - no dataSource or dataSource is empty";
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
 * @brief Draw custom markers for LTW graph with adaptive time-based binning
 * Uses 1/5 of the current time interval as the bin duration for sampling
 *
 * @param seriesLabel The series label to draw markers for
 * @param markerColor The color for the markers
 */
void LTWGraph::drawCustomMarkers(const QString &seriesLabel, const QColor &markerColor)
{
    if (!dataSource || !graphicsScene) {
        qDebug() << "LTW: drawCustomMarkers early return - no dataSource or graphicsScene";
        return;
    }

    // Get total data size for comparison
    size_t totalDataSize = dataSource->getDataSeriesSize(seriesLabel);
    qDebug() << "LTW: drawCustomMarkers called for series" << seriesLabel << "with total data size:" << totalDataSize;

    if (totalDataSize == 0) {
        qDebug() << "LTW: No data available for series" << seriesLabel;
        return;
    }

    // Use the static binning method to sample data based on time intervals
    // Set sampling rate to 1/5 of current time interval
    qint64 currentIntervalMs = getTimeIntervalMs();
    qint64 samplingIntervalMs = currentIntervalMs / 5; // 1/5 of current time interval
    
    // Convert to QTime for the binning method
    QTime binDuration = QTime(0, 0, 0).addMSecs(samplingIntervalMs);
    
    // Get raw data and use static binning method
    const std::vector<qreal>& yData = dataSource->getYDataSeries(seriesLabel);
    const std::vector<QDateTime>& timestamps = dataSource->getTimestampsSeries(seriesLabel);
    std::vector<std::pair<qreal, QDateTime>> binnedData = WaterfallData::binDataByTime(yData, timestamps, binDuration);
    
    // Filter binned data to only include points within the visible time range
    std::vector<std::pair<qreal, QDateTime>> visibleBinnedData;
    for (const auto& point : binnedData) {
        if (point.second >= timeMin && point.second <= timeMax) {
            visibleBinnedData.push_back(point);
        }
    }
    
    qDebug() << "LTW: Time range filtering - Total binned:" << binnedData.size() 
             << "- Visible binned:" << visibleBinnedData.size()
             << "- Time range:" << timeMin.toString() << "to" << timeMax.toString();

    qDebug() << "LTW: Binning completed for series" << seriesLabel 
             << "- Total data:" << totalDataSize 
             << "- Binned data:" << binnedData.size()
             << "- Visible binned data:" << visibleBinnedData.size()
             << "- Bin duration:" << samplingIntervalMs << "ms";

    if (visibleBinnedData.empty()) {
        qDebug() << "LTW: No visible binned data available for series" << seriesLabel;
        return;
    }

    // Draw markers for each visible binned point
    int markersDrawn = 0;
    for (const auto& point : visibleBinnedData) {
        qreal yValue = point.first;
        QDateTime timestamp = point.second;
        QPointF screenPos = mapDataToScreen(yValue, timestamp);
        
        // Check if point is within visible area
        if (drawingArea.contains(screenPos)) {
            // Calculate marker sizes
            QSize windowSize = this->size();
            qreal squareSize = std::min(0.05 * windowSize.width(), 12.0); // Cap at 12 pixels
            qreal triangleSize = squareSize * 0.5; // Triangle is half the width of square
            
            // Draw cyan square border
            QGraphicsRectItem *square = new QGraphicsRectItem(
                screenPos.x() - squareSize/2, 
                screenPos.y() - squareSize/2, 
                squareSize, 
                squareSize
            );
            square->setPen(QPen(Qt::white, 1.0));
            square->setBrush(QBrush(Qt::transparent));
            square->setZValue(500); // Lower z-value than triangle
            graphicsScene->addItem(square);
            
            // Draw cyan triangle
            QPolygonF triangle;
            triangle << QPointF(screenPos.x(), screenPos.y() - triangleSize/2)  // Top point
                     << QPointF(screenPos.x() - triangleSize/2, screenPos.y() + triangleSize/2)  // Bottom left
                     << QPointF(screenPos.x() + triangleSize/2, screenPos.y() + triangleSize/2); // Bottom right
            
            QGraphicsPolygonItem *triangleItem = new QGraphicsPolygonItem(triangle);
            triangleItem->setPen(QPen(Qt::white, 1.0));
            triangleItem->setBrush(QBrush(Qt::white));
            triangleItem->setZValue(600); // Higher z-value than square
            graphicsScene->addItem(triangleItem);
            
            markersDrawn++;
        }
    }
    
    qDebug() << "LTW: Successfully drew" << markersDrawn << "markers for series" << seriesLabel;
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
