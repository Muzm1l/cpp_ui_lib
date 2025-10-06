#include "btwgraph.h"
#include <QDebug>

/**
 * @brief Construct a new BTWGraph::BTWGraph object
 *
 * @param parent Parent widget
 * @param enableGrid Whether to enable grid display
 * @param gridDivisions Number of grid divisions
 * @param timeInterval Time interval for the waterfall display
 */
BTWGraph::BTWGraph(QWidget *parent, bool enableGrid, int gridDivisions, TimeInterval timeInterval)
    : WaterfallGraph(parent, enableGrid, gridDivisions, timeInterval)
{
    qDebug() << "BTWGraph constructor called";
}

/**
 * @brief Destroy the BTWGraph::BTWGraph object
 *
 */
BTWGraph::~BTWGraph()
{
    qDebug() << "BTWGraph destructor called";
}

/**
 * @brief Override draw method to create scatterplots by default
 *
 */
void BTWGraph::draw()
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

                if (seriesLabel == "BTW-1")
                {
                    // Draw custom circle markers for BTW-1 series with delta-calculated angles
                    drawCustomCircleMarkers(seriesLabel);
                }
            }
        }
    }
}

/**
 * @brief Handle mouse click events specific to BTW graph
 *
 * @param scenePos Scene position of the click
 */
void BTWGraph::onMouseClick(const QPointF &scenePos)
{
    qDebug() << "BTWGraph mouse clicked at scene position:" << scenePos;
    // Call parent implementation
    WaterfallGraph::onMouseClick(scenePos);
}

/**
 * @brief Handle mouse drag events specific to BTW graph
 *
 * @param scenePos Scene position of the drag
 */
void BTWGraph::onMouseDrag(const QPointF &scenePos)
{
    qDebug() << "BTWGraph mouse dragged to scene position:" << scenePos;
    // Call parent implementation
    WaterfallGraph::onMouseDrag(scenePos);
}

/**
 * @brief Draw BTW-specific scatterplot
 *
 */
void BTWGraph::drawBTWScatterplot()
{
    // By default, create a scatterplot using the parent's scatterplot functionality
    drawScatterplot(QString("BTW-1"), Qt::red, 4.0, Qt::white);

    qDebug() << "BTW scatterplot drawn";
}

/**
 * @brief Draw custom circle markers with delta-calculated line for BTW graph
 * Circle outline with a line whose angle is calculated from delta values between consecutive points
 *
 * @param seriesLabel The series label to draw markers for
 */
void BTWGraph::drawCustomCircleMarkers(const QString &seriesLabel)
{
    if (!dataSource || !graphicsScene) {
        qDebug() << "BTW: drawCustomCircleMarkers early return - no dataSource or graphicsScene";
        return;
    }

    // Get BTW-1 series data to calculate deltas
    const std::vector<qreal>& btw1Data = dataSource->getYDataSeries("BTW-1");
    const std::vector<QDateTime>& btw1Timestamps = dataSource->getTimestampsSeries("BTW-1");
    
    if (btw1Data.size() < 2) {
        qDebug() << "BTW: Not enough BTW-1 data points for delta calculation";
        return;
    }

    // Calculate delta values (differences between consecutive points)
    std::vector<qreal> deltaValues;
    std::vector<QDateTime> deltaTimestamps;
    
    // First delta is 0 (no previous point)
    deltaValues.push_back(0.0);
    deltaTimestamps.push_back(btw1Timestamps[0]);
    
    // Calculate deltas for remaining points
    for (size_t i = 1; i < btw1Data.size(); ++i) {
        qreal delta = btw1Data[i] - btw1Data[i - 1];
        deltaValues.push_back(delta);
        deltaTimestamps.push_back(btw1Timestamps[i]);
    }

    qDebug() << "BTW: Calculated" << deltaValues.size() << "delta values for custom markers";

    // Use 1/5 of current time interval for sampling (following pattern from RTW/LTW)
    qint64 samplingIntervalMs = 300000; // 3 seconds

    // Convert to QTime for the binning method
    QTime binDuration = QTime(0, 0, 0).addMSecs(samplingIntervalMs);
    
    // Use BTW-1 data for binning (for positioning), but keep delta values for angle calculation
    std::vector<std::pair<qreal, QDateTime>> binnedData = WaterfallData::binDataByTime(btw1Data, btw1Timestamps, binDuration);
    
    // Filter binned data to only include points within the visible time range
    std::vector<std::pair<qreal, QDateTime>> visibleBinnedData;
    for (const auto& point : binnedData) {
        if (point.second >= timeMin && point.second <= timeMax) {
            visibleBinnedData.push_back(point);
        }
    }

    // Helper function to find delta value for a given timestamp
    auto findDeltaForTimestamp = [&](const QDateTime& timestamp) -> qreal {
        // Find the closest timestamp in deltaTimestamps
        qint64 minDiff = LLONG_MAX;
        qreal closestDelta = 0.0;
        
        for (size_t i = 0; i < deltaTimestamps.size(); ++i) {
            qint64 diff = qAbs(deltaTimestamps[i].msecsTo(timestamp));
            if (diff < minDiff) {
                minDiff = diff;
                closestDelta = deltaValues[i];
            }
        }
        return closestDelta;
    };
    
    qDebug() << "BTW: Time range filtering - Total binned:" << binnedData.size() 
             << "- Visible binned:" << visibleBinnedData.size()
             << "- Time range:" << timeMin.toString() << "to" << timeMax.toString();

    qDebug() << "BTW: Binning completed for delta values" 
             << "- Total delta data:" << deltaValues.size() 
             << "- Binned data:" << binnedData.size()
             << "- Visible binned data:" << visibleBinnedData.size()
             << "- Sampling interval:" << samplingIntervalMs << "ms";

    if (visibleBinnedData.empty()) {
        qDebug() << "BTW: No visible binned data available for series" << seriesLabel;
        qDebug() << "BTW: Trying fallback - drawing markers for raw data";
        
        // Fallback: draw markers for raw data if binning produces no visible results
        int fallbackMarkersDrawn = 0;
        for (size_t i = 0; i < deltaValues.size() && i < 10; ++i) { // Limit to first 10 points
            qreal deltaValue = deltaValues[i];
            qreal btw1Value = btw1Data[i]; // Use BTW-1 value for positioning
            QDateTime timestamp = deltaTimestamps[i];
            QPointF screenPos = mapDataToScreen(btw1Value, timestamp); // Position based on BTW-1 value
            
            if (drawingArea.contains(screenPos)) {
                QSize windowSize = this->size();
                qreal markerRadius = std::min(0.04 * windowSize.width(), 12.0);
                
                // Draw circle outline
                QGraphicsEllipseItem *circleOutline = new QGraphicsEllipseItem();
                circleOutline->setRect(screenPos.x() - markerRadius, screenPos.y() - markerRadius, 
                                     2 * markerRadius, 2 * markerRadius);
                circleOutline->setPen(QPen(Qt::blue, 2));
                circleOutline->setBrush(QBrush(Qt::transparent));
                circleOutline->setZValue(1000);
                
                graphicsScene->addItem(circleOutline);
                
                // Draw angled line (5x radius on both sides)
                qreal lineLength = 5 * markerRadius;
                
                // Calculate angle from delta value
                // Map delta value to angle: positive delta = positive angle (clockwise), negative delta = negative angle (counterclockwise)
                qreal angleDegrees = deltaValue * 10.0; // Scale factor to convert delta to meaningful angle
                qreal angleRadians = qDegreesToRadians(angleDegrees);
                
                // Calculate line endpoints based on angle
                // For true north (0°), line points up/down (vertical)
                qreal deltaX = lineLength * qSin(angleRadians);
                qreal deltaY = -lineLength * qCos(angleRadians); // Negative because Y increases downward
                
                QGraphicsLineItem *angledLine = new QGraphicsLineItem();
                angledLine->setLine(screenPos.x() - deltaX, screenPos.y() - deltaY,
                                  screenPos.x() + deltaX, screenPos.y() + deltaY);
                angledLine->setPen(QPen(Qt::blue, 2));
                angledLine->setZValue(1001);
                
                graphicsScene->addItem(angledLine);
                
                fallbackMarkersDrawn++;
            }
        }
        qDebug() << "BTW: Fallback drew" << fallbackMarkersDrawn << "blue circle markers";
        return;
    }

    // Draw circle markers for each visible binned point
    int markersDrawn = 0;
    qDebug() << "BTW: Drawing area:" << drawingArea;
    for (const auto& point : visibleBinnedData) {
        qreal btw1Value = point.first; // BTW-1 value for positioning
        QDateTime timestamp = point.second;
        qreal deltaValue = findDeltaForTimestamp(timestamp); // Delta value for angle calculation
        QPointF screenPos = mapDataToScreen(btw1Value, timestamp); // Position based on BTW-1 value
        
        // Only debug first few points to avoid spam
        if (markersDrawn < 3) {
            qDebug() << "BTW: Point" << markersDrawn << "- BTW1:" << btw1Value << "Delta:" << deltaValue << "Time:" << timestamp.toString() << "Screen:" << screenPos << "In area:" << drawingArea.contains(screenPos);
        }
        
        // Check if point is within visible area
        if (drawingArea.contains(screenPos)) {
            // Calculate marker size based on window size
            QSize windowSize = this->size();
            qreal markerRadius = std::min(0.04 * windowSize.width(), 12.0); // Circle radius, cap at 12 pixels
            
            // Draw circle outline
            QGraphicsEllipseItem *circleOutline = new QGraphicsEllipseItem();
            circleOutline->setRect(screenPos.x() - markerRadius, screenPos.y() - markerRadius, 
                                 2 * markerRadius, 2 * markerRadius);
            circleOutline->setPen(QPen(Qt::blue, 2));
            circleOutline->setBrush(QBrush(Qt::transparent));
            circleOutline->setZValue(1000);
            
            graphicsScene->addItem(circleOutline);
            
            // Draw angled line (5x radius on both sides)
            qreal lineLength = 5 * markerRadius;
            
            // Calculate angle from delta value
            // Map delta value to angle: positive delta = positive angle (clockwise), negative delta = negative angle (counterclockwise)
            qreal angleDegrees = deltaValue * 10.0; // Scale factor to convert delta to meaningful angle
            qreal angleRadians = qDegreesToRadians(angleDegrees);
            
            // Calculate line endpoints based on angle
            // For true north (0°), line points up/down (vertical)
            qreal deltaX = lineLength * qSin(angleRadians);
            qreal deltaY = -lineLength * qCos(angleRadians); // Negative because Y increases downward
            
            QGraphicsLineItem *angledLine = new QGraphicsLineItem();
            angledLine->setLine(screenPos.x() - deltaX, screenPos.y() - deltaY,
                              screenPos.x() + deltaX, screenPos.y() + deltaY);
            angledLine->setPen(QPen(Qt::blue, 2));
            angledLine->setZValue(1001);
            
            graphicsScene->addItem(angledLine);
            
            markersDrawn++;
        }
    }
    
    qDebug() << "BTW: Drew" << markersDrawn << "delta-based circle markers";
}
