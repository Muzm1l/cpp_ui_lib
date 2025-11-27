#include "btwgraph.h"
#include "btwinteractiveoverlay.h"
#include "interactivegraphicsitem.h"
#include <QDebug>
#include <QRandomGenerator>

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
    , m_interactiveOverlay(nullptr)
{
    qDebug() << "BTWGraph constructor called";
    
    // Setup interactive overlay
    setupInteractiveOverlay();
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

    // Clear existing items - ensure complete clearing before drawing
    // Automatic circle markers are in graphicsScene, so clearing graphicsScene removes them
    graphicsScene->clear();
    graphicsScene->update(); // Force immediate update to ensure clearing is visible
    
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
 * Adds a single interactive marker when clicking on the graph
 * Removes any existing marker before adding a new one
 *
 * @param scenePos Scene position of the click
 */
void BTWGraph::onMouseClick(const QPointF &scenePos)
{
    qDebug() << "BTWGraph mouse clicked at scene position:" << scenePos;
    
    // Check if we clicked on an existing interactive marker in the overlay scene
    // The overlay scene and graphics scene share the same coordinate system
    if (m_interactiveOverlay && m_interactiveOverlay->getOverlayScene()) {
        QGraphicsItem *itemAtPos = m_interactiveOverlay->getOverlayScene()->itemAt(scenePos, QTransform());
        // Filter out crosshair items - they should not prevent marker creation
        if (itemAtPos && itemAtPos != crosshairHorizontal && itemAtPos != crosshairVertical) {
            qDebug() << "BTWGraph: Clicked on existing interactive item:" << itemAtPos << "letting it handle the event";
            // Don't add a new marker, let the interactive item handle the click
            return;
        } else {
            qDebug() << "BTWGraph: No interactive item found at position:" << scenePos;
            qDebug() << "BTWGraph: Overlay scene items count:" << m_interactiveOverlay->getOverlayScene()->items().size();
        }
    }
    
    // Only add a marker if we clicked on empty space (no interactive items)
    if (m_interactiveOverlay) {
        // Clear any existing markers first (only allow one marker)
        m_interactiveOverlay->clearAllMarkers();
        
        // Convert scene position to overlay coordinates
        QPointF overlayPos = scenePos;
        
        // Add a data point marker at the clicked position
        QDateTime timestamp = QDateTime::currentDateTime();
        qreal value = 50.0; // Default value
        QString seriesLabel = "BTW-Click";
        
        m_interactiveOverlay->addDataPointMarker(overlayPos, timestamp, value, seriesLabel);
        
        qDebug() << "BTWGraph: Added new interactive marker at:" << overlayPos;
    }
    
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
                
                // Add blue text label with rectangular outline beside the marker
                DrawUtils::addBearingRateBoxToScene(graphicsScene, deltaValue, Qt::blue, screenPos, markerRadius, 1002);
                
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
            
            // Add blue text label with rectangular outline beside the marker
            QString prefix = (deltaValue >= 0) ? "R" : "L";
            QString displayValue = (deltaValue >= 0) ? QString::number(deltaValue, 'f', 1) : QString::number(-deltaValue, 'f', 1);
            QGraphicsTextItem *textLabel = new QGraphicsTextItem(prefix + displayValue);
            QFont font = textLabel->font();
            font.setPointSizeF(8.0);
            font.setBold(true);
            textLabel->setFont(font);
            textLabel->setDefaultTextColor(Qt::blue);
            
            // Position text label to the left of the marker
            QRectF textRect = textLabel->boundingRect();
            textLabel->setPos(screenPos.x() - textRect.width() - markerRadius - 5, 
                            screenPos.y() - textRect.height() / 2);
            textLabel->setZValue(1002);
            
            graphicsScene->addItem(textLabel);
            
            // Add rectangular outline around the text
            QGraphicsRectItem *textOutline = new QGraphicsRectItem();
            textOutline->setRect(textLabel->pos().x() - 2, textLabel->pos().y() - 2,
                               textRect.width() + 4, textRect.height() + 4);
            textOutline->setPen(QPen(Qt::blue, 1));
            textOutline->setBrush(QBrush(Qt::transparent));
            textOutline->setZValue(1001);
            
            graphicsScene->addItem(textOutline);
            
            markersDrawn++;
        }
    }
    
    qDebug() << "BTW: Drew" << markersDrawn << "delta-based circle markers";
}

/**
 * @brief Get the interactive overlay
 * @return Pointer to the interactive overlay
 */
BTWInteractiveOverlay* BTWGraph::getInteractiveOverlay() const
{
    return m_interactiveOverlay;
}

/**
 * @brief Override resize event to update overlay
 * @param event Resize event
 */
void BTWGraph::resizeEvent(QResizeEvent *event)
{
    WaterfallGraph::resizeEvent(event);
    
    // Update overlay after resize
    if (m_interactiveOverlay) {
        m_interactiveOverlay->updateOverlay();
    }
}

/**
 * @brief Setup interactive overlay
 */
void BTWGraph::setupInteractiveOverlay()
{
    m_interactiveOverlay = new BTWInteractiveOverlay(this, this);
    
    // Connect overlay signals
    connect(m_interactiveOverlay, &BTWInteractiveOverlay::markerAdded, 
            this, [this](InteractiveGraphicsItem *marker, BTWInteractiveOverlay::MarkerType type) {
                onMarkerAdded(marker, static_cast<int>(type));
            });
    connect(m_interactiveOverlay, &BTWInteractiveOverlay::markerRemoved, 
            this, [this](InteractiveGraphicsItem *marker, BTWInteractiveOverlay::MarkerType type) {
                onMarkerRemoved(marker, static_cast<int>(type));
            });
    connect(m_interactiveOverlay, &BTWInteractiveOverlay::markerMoved, 
            this, &BTWGraph::onMarkerMoved);
    connect(m_interactiveOverlay, &BTWInteractiveOverlay::markerRotated, 
            this, &BTWGraph::onMarkerRotated);
    connect(m_interactiveOverlay, &BTWInteractiveOverlay::markerClicked,
            this, &BTWGraph::onMarkerClicked);
    
    qDebug() << "BTWGraph: Interactive overlay setup complete";
}

void BTWGraph::deleteInteractiveMarkers()
{
    if (!m_interactiveOverlay) {
        qDebug() << "BTWGraph: deleteInteractiveMarkers called but overlay not available";
        return;
    }

    qDebug() << "BTWGraph: Clearing all interactive markers";
    m_interactiveOverlay->clearAllMarkers();
}


void BTWGraph::onMarkerAdded(InteractiveGraphicsItem *marker, int type)
{
    Q_UNUSED(marker);
    Q_UNUSED(type);
    qDebug() << "BTWGraph: Marker added, type:" << type;
}

void BTWGraph::onMarkerRemoved(InteractiveGraphicsItem *marker, int type)
{
    Q_UNUSED(marker);
    Q_UNUSED(type);
    qDebug() << "BTWGraph: Marker removed, type:" << type;
}

void BTWGraph::onMarkerMoved(InteractiveGraphicsItem *marker, const QPointF &newPosition)
{
    Q_UNUSED(marker);
    Q_UNUSED(newPosition);
    qDebug() << "BTWGraph: Marker moved to:" << newPosition;
}

void BTWGraph::onMarkerRotated(InteractiveGraphicsItem *marker, qreal angle)
{
    Q_UNUSED(marker);
    Q_UNUSED(angle);
    qDebug() << "BTWGraph: Marker rotated by:" << angle << "degrees";
}

void BTWGraph::onMarkerClicked(InteractiveGraphicsItem *marker, const QPointF &position)
{
    if (!marker) {
        qDebug() << "BTWGraph: Marker clicked - NULL marker";
        return;
    }
    
    // Get the marker's scene position (Y coordinate)
    QPointF scenePos = marker->scenePos();
    qreal yPos = scenePos.y();
    
    // Convert Y position to timestamp using mapScreenToTime
    QDateTime timestamp = mapScreenToTime(yPos);
    
    if (timestamp.isValid()) {
        qDebug() << "========================================";
        qDebug() << "BTW MARKER SELECTED - TIMESTAMP RETURNED";
        qDebug() << "========================================";
        qDebug() << "BTWGraph: Marker clicked at position:" << position;
        qDebug() << "BTWGraph: Marker scene position:" << scenePos;
        qDebug() << "BTWGraph: Marker Y position:" << yPos;
        qDebug() << "BTWGraph: TIMESTAMP:" << timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz");
        qDebug() << "========================================";
    } else {
        qDebug() << "BTWGraph: Marker clicked at:" << position << "- Could not determine timestamp (invalid)";
    }
}
