#include "btwgraph.h"
#include "btwinteractiveoverlay.h"
#include "interactivegraphicsitem.h"
#include "graphcontainer.h"
#include "graphlayout.h"
#include "waterfalldata.h"
#include "graphtype.h"
#include "zoompanel.h"
#include "sharedsyncstate.h"
#include <QDebug>
#include <QRandomGenerator>
#include <QPainter>
#include <QPixmap>

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
    , symbols(40)  // Initialize BTW symbol cache
    , m_nextRegionId(1)
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
    
    // Prevent concurrent drawing to avoid marker duplication
    if (isDrawing) {
        qDebug() << "BTWGraph: draw() already in progress, skipping";
        return;
    }
    
    isDrawing = true;

    // Clear existing items - ensure complete clearing before drawing
    // Automatic circle markers are in graphicsScene, so clearing graphicsScene removes them
    // Shaded region polygon items will be recreated in drawShadedRegions() from stored data
    // Clear polygon item pointers since clear() will delete them
    for (auto it = m_shadedRegions.begin(); it != m_shadedRegions.end(); ++it) {
        it.value().polygonItem = nullptr;
    }
    
    graphicsScene->clear();
    graphicsScene->update(); // Force immediate update to ensure clearing is visible
    
    // Clear stored timestamps when redrawing (markers will be recreated)
    m_automaticMarkerTimestamps.clear();
    
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
                    drawScatterplot(seriesLabel, seriesColor, 4.0, Qt::black);
                }

                // BTW markers are now manually placed through data source - no automatic generation
            }
        }
    }
    
    // Draw BTW symbols (magenta circles from other graphs)
    drawBTWSymbols();
    
    // Draw manually placed BTW markers from data source
    drawCustomCircleMarkers();
    
    // Draw shaded regions
    drawShadedRegions();
    
    // Sync interactive overlay markers with the new time range
    // This updates marker Y positions to stay in sync with the timeline
    if (m_interactiveOverlay) {
        qDebug() << "BTWGraph::draw() - calling syncMarkersWithTimeline, timeMin:" << timeMin.toString() << "timeMax:" << timeMax.toString();
        m_interactiveOverlay->syncMarkersWithTimeline();
    }
    
    isDrawing = false;
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
        // Convert scene position to overlay coordinates
        QPointF overlayPos = scenePos;
        
        // Calculate timestamp from Y position (this represents the time at that position on the graph)
        QDateTime timestamp = mapScreenToTime(scenePos.y());
        
        // If timestamp is invalid, fallback to current time
        if (!timestamp.isValid()) {
            timestamp = QDateTime::currentDateTime();
            qDebug() << "BTWGraph: Could not map Y position to timestamp, using current time";
        }
        
        // Get value from X position (range value)
        qreal value = mapScreenXToRange(scenePos.x());
        QString seriesLabel = "BTW-Click";
        
        m_interactiveOverlay->addDataPointMarker(overlayPos, timestamp, value, seriesLabel);
        
        qDebug() << "BTWGraph: Added new interactive marker at:" << overlayPos << "with timestamp:" << timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz") << "value:" << value;
        
        // Emit signal for marker timestamp and value
        if (timestamp.isValid()) {
            emit markerTimestampValueChanged(timestamp, value);
        }
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
 * @brief Draw manually placed BTW circle markers from data source
 * Circle outline with a line whose angle is calculated from delta values
 */
void BTWGraph::drawCustomCircleMarkers()
{
    if (!dataSource || !graphicsScene) {
        qDebug() << "BTW: drawCustomCircleMarkers early return - no dataSource or graphicsScene";
        return;
    }

    // Get manually placed markers from data source
    // Note: BTWMarkerData here is from waterfalldata.h (not BTWSyncMarkerData)
    std::vector<BTWMarkerData> btwMarkers = dataSource->getBTWMarkers();
    
    if (btwMarkers.empty()) {
        qDebug() << "BTW: No manually placed markers in data source";
        return;
    }

    // Filter markers to only include those within the visible time range
    std::vector<BTWMarkerData> visibleMarkers;
    bool timeRangeValid = timeMin.isValid() && timeMax.isValid() && timeMin <= timeMax;
    
    if (timeRangeValid) {
        for (const auto& markerData : btwMarkers) {
            if (markerData.timestamp >= timeMin && markerData.timestamp <= timeMax) {
                visibleMarkers.push_back(markerData);
            }
        }
    } else {
        visibleMarkers = btwMarkers;
    }

    if (visibleMarkers.empty()) {
        qDebug() << "BTW: No visible markers within time range";
        return;
    }

    // Draw circle markers for each visible marker
    int markersDrawn = 0;
    qDebug() << "BTW: Drawing" << visibleMarkers.size() << "manually placed markers";
    
    for (const auto& markerData : visibleMarkers) {
        QDateTime timestamp = markerData.timestamp;
        qreal range = markerData.range;
        qreal deltaValue = markerData.delta;
        
        QPointF screenPos = mapDataToScreen(range, timestamp);
        
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
    
    qDebug() << "BTW: Drew" << markersDrawn << "manually placed circle markers";
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
    
    // Connect sync signals to forward to GraphContainer
    connect(m_interactiveOverlay, &BTWInteractiveOverlay::markerDataChanged,
            this, &BTWGraph::markerSyncDataChanged);
    connect(m_interactiveOverlay, &BTWInteractiveOverlay::markerDeleted,
            this, &BTWGraph::markerSyncDeleted);
    
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
    if (!marker) {
        qDebug() << "BTWGraph: Marker added - NULL marker, type:" << type;
        return;
    }
    
    // Extract timestamp from marker's stored data
    QVariant timestampVariant = marker->data(0);
    QDateTime timestamp;
    
    if (timestampVariant.isValid() && timestampVariant.canConvert<QDateTime>()) {
        timestamp = timestampVariant.value<QDateTime>();
    } else {
        // Fallback: calculate timestamp from marker's Y position
        QPointF scenePos = marker->scenePos();
        qreal yPos = scenePos.y();
        timestamp = mapScreenToTime(yPos);
    }
    
    if (timestamp.isValid()) {
        qDebug() << "========================================";
        qDebug() << "BTW MANUAL MARKER PLACED - TIMESTAMP RETURNED";
        qDebug() << "========================================";
        qDebug() << "BTWGraph: Marker added, type:" << type;
        qDebug() << "BTWGraph: Marker scene position:" << marker->scenePos();
        qDebug() << "BTWGraph: TIMESTAMP:" << timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz");
        qDebug() << "========================================";
        
        // Emit signal for external integration
        emit manualMarkerPlaced(timestamp, marker->scenePos());
    } else {
        qDebug() << "BTWGraph: Marker added, type:" << type << "- Could not determine timestamp (invalid)";
    }
}

void BTWGraph::onMarkerRemoved(InteractiveGraphicsItem *marker, int type)
{
    Q_UNUSED(marker);
    Q_UNUSED(type);
    qDebug() << "BTWGraph: Marker removed, type:" << type;
}

void BTWGraph::onMarkerMoved(InteractiveGraphicsItem *marker, const QPointF &newPosition)
{
    if (!marker) {
        qDebug() << "BTWGraph: Marker moved - NULL marker";
        return;
    }
    
    qDebug() << "BTWGraph: Marker moved to:" << newPosition;
    
    // Extract timestamp from marker's stored data
    QVariant timestampVariant = marker->data(0);
    QDateTime timestamp;
    
    if (timestampVariant.isValid() && timestampVariant.canConvert<QDateTime>()) {
        timestamp = timestampVariant.value<QDateTime>();
    } else {
        // Fallback: calculate timestamp from marker's Y position
        qreal yPos = newPosition.y();
        timestamp = mapScreenToTime(yPos);
    }
    
    // Update magenta circles when green marker is moved
    if (timestamp.isValid()) {
        emit manualMarkerPlaced(timestamp, newPosition);
        qDebug() << "BTWGraph: Emitted manualMarkerPlaced signal for moved marker at timestamp" << timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz");
    }
}

void BTWGraph::onMarkerRotated(InteractiveGraphicsItem *marker, qreal angle)
{
    if (!marker) {
        qDebug() << "BTWGraph: Marker rotated - NULL marker";
        return;
    }
    
    qDebug() << "BTWGraph: Marker rotated by:" << angle << "degrees";
    
    // Extract timestamp from marker's stored data
    QVariant timestampVariant = marker->data(0);
    QDateTime timestamp;
    
    if (timestampVariant.isValid() && timestampVariant.canConvert<QDateTime>()) {
        timestamp = timestampVariant.value<QDateTime>();
    } else {
        // Fallback: calculate timestamp from marker's Y position
        QPointF scenePos = marker->scenePos();
        qreal yPos = scenePos.y();
        timestamp = mapScreenToTime(yPos);
    }
    
    // Update magenta circles when green marker is rotated (position might have changed)
    if (timestamp.isValid()) {
        emit manualMarkerPlaced(timestamp, marker->scenePos());
        qDebug() << "BTWGraph: Emitted manualMarkerPlaced signal for rotated marker at timestamp" << timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz");
    }
}

void BTWGraph::onMarkerClicked(InteractiveGraphicsItem *marker, const QPointF &position)
{
    if (!marker) {
        qDebug() << "BTWGraph: Marker clicked - NULL marker";
        return;
    }
    
    // First try to get timestamp from marker's stored data (key 0)
    QVariant timestampVariant = marker->data(0);
    QDateTime timestamp;
    
    if (timestampVariant.isValid() && timestampVariant.canConvert<QDateTime>()) {
        timestamp = timestampVariant.value<QDateTime>();
    } else {
        // Fallback: calculate timestamp from marker's Y position
        QPointF scenePos = marker->scenePos();
        qreal yPos = scenePos.y();
        timestamp = mapScreenToTime(yPos);
    }
    
    // Get range value from marker's X position
    QPointF scenePos = marker->scenePos();
    qreal rangeValue = mapScreenXToRange(scenePos.x());
    
    // Get bearing rate from marker's rotation
    // The bearing rate is stored as rotation / 10.0 (same as delta value)
    qreal bearingRate = marker->rotation() / 10.0;
    
    // Also check if there's a stored delta value (key 1)
    QVariant deltaVariant = marker->data(1);
    if (deltaVariant.isValid() && deltaVariant.canConvert<qreal>()) {
        // If delta was stored, use it (might be more accurate)
        bearingRate = deltaVariant.value<qreal>();
    }
    
    if (timestamp.isValid()) {
        qDebug() << "========================================";
        qDebug() << "BTW MANUAL MARKER CLICKED - FULL DATA RETURNED";
        qDebug() << "========================================";
        qDebug() << "BTWGraph: Marker clicked at position:" << position;
        qDebug() << "BTWGraph: Marker scene position:" << scenePos;
        qDebug() << "BTWGraph: TIMESTAMP:" << timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz");
        qDebug() << "BTWGraph: RANGE VALUE:" << rangeValue;
        qDebug() << "BTWGraph: BEARING RATE (Box Value):" << bearingRate;
        qDebug() << "========================================";
        
        // Emit signal for external integration (legacy)
        emit manualMarkerClicked(timestamp, scenePos);
        
        // Emit signal for marker timestamp and value (legacy - propagates through GraphContainer -> GraphLayout)
        emit markerTimestampValueChanged(timestamp, rangeValue);
        
        // Emit new comprehensive signal with all three values
        emit markerClickedWithData(timestamp, rangeValue, bearingRate);
        
        qDebug() << "BTWGraph: All marker click signals emitted successfully";
    } else {
        qDebug() << "BTWGraph: Marker clicked at:" << position << "- Could not determine timestamp (invalid)";
        qDebug() << "BTWGraph: Range value:" << rangeValue << "Bearing rate:" << bearingRate;
    }
}

std::vector<QDateTime> BTWGraph::getAutomaticMarkerTimestamps() const
{
    return m_automaticMarkerTimestamps;
}

void BTWGraph::addBTWSymbol(const QString &symbolName, const QDateTime &timestamp, qreal range)
{
    // Store symbol in dataSource (WaterfallData) so it persists with track data
    if (!dataSource)
    {
        qDebug() << "BTW: Cannot add symbol - no data source set";
        return;
    }
    
    dataSource->addBTWSymbol(symbolName, timestamp, range);
    
    qDebug() << "BTW: Added symbol" << symbolName << "at timestamp" << timestamp.toString() << "with range" << range << "to data source";
    
    // Trigger redraw
    draw();
}

BTWSymbolDrawing::SymbolType BTWGraph::symbolNameToType(const QString &symbolName) const
{
    QString name = symbolName.toUpper();
    if (name == "MAGENTACIRCLE") return BTWSymbolDrawing::SymbolType::MagentaCircle;
    
    // Default to MagentaCircle
    return BTWSymbolDrawing::SymbolType::MagentaCircle;
}

void BTWGraph::drawBTWSymbols()
{
    // Follow the same pattern as RTW symbols - read symbols from dataSource
    if (!graphicsScene || !dataSource)
    {
        return;
    }
    
    // Get symbols from dataSource
    std::vector<BTWSymbolData> btwSymbols = dataSource->getBTWSymbols();
    
    if (btwSymbols.empty())
    {
        return;
    }
    
    // Filter symbols to only include those within the visible time range
    std::vector<BTWSymbolData> visibleSymbols;
    bool timeRangeValid = timeMin.isValid() && timeMax.isValid() && timeMin <= timeMax;
    
    if (timeRangeValid)
    {
        for (const auto& symbolData : btwSymbols)
        {
            if (symbolData.timestamp >= timeMin && symbolData.timestamp <= timeMax)
            {
                visibleSymbols.push_back(symbolData);
            }
        }
    }
    else
    {
        visibleSymbols = btwSymbols;
    }
    
    // Draw symbols
    for (const auto& symbolData : visibleSymbols)
    {
        // Map symbol position to screen coordinates
        QPointF screenPos = mapDataToScreen(symbolData.range, symbolData.timestamp);
        
        // Check if point is within visible area
        if (!drawingArea.contains(screenPos))
        {
            continue;
        }
        
        // Convert symbol name to SymbolType
        BTWSymbolDrawing::SymbolType symbolType = symbolNameToType(symbolData.symbolName);
        
        // Get the pixmap for this symbol type
        const QPixmap& symbolPixmap = symbols.get(symbolType);
        
        // Validate pixmap before using it
        if (symbolPixmap.isNull() || symbolPixmap.width() <= 0 || symbolPixmap.height() <= 0)
        {
            continue;
        }
        
        // Create a graphics pixmap item and add it to the scene
        QGraphicsPixmapItem* pixmapItem = new QGraphicsPixmapItem(symbolPixmap);
        
        // Center the symbol on the data point
        QRectF pixmapRect = pixmapItem->boundingRect();
        pixmapItem->setPos(screenPos.x() - pixmapRect.width()/2, screenPos.y() - pixmapRect.height()/2);
        pixmapItem->setZValue(1003); // Above markers but below interactive items
        
        graphicsScene->addItem(pixmapItem);
    }
}

void BTWGraph::addBTWSymbolToOtherGraphs(const QDateTime &timestamp, qreal btwValue)
{
    // Find parent GraphContainer to access GraphLayout
    QWidget *parent = this->parentWidget();
    if (!parent) return;
    
    // Try to find GraphContainer
    GraphContainer *container = qobject_cast<GraphContainer*>(parent);
    if (!container) return;
    
    // Try to find GraphLayout (parent of GraphContainer)
    QWidget *layoutWidget = container->parentWidget();
    if (!layoutWidget) return;
    
    GraphLayout *layout = qobject_cast<GraphLayout*>(layoutWidget);
    if (!layout) return;
    
    // Get all graph containers in the layout
    // We need to access m_graphContainers, but it's private, so we'll use a different approach
    // Get all containers using findChildren
    QList<GraphContainer*> allContainers = layout->findChildren<GraphContainer*>();
    
    for (GraphContainer *otherContainer : allContainers)
    {
        if (otherContainer == container) continue; // Skip self
        
        // Get all graph types in this container
        std::vector<GraphType> graphTypes = getAllGraphTypes();
        
        for (GraphType graphType : graphTypes)
        {
            if (graphType == GraphType::BTW) continue; // Skip BTW graphs
            
            // Check if this container has this graph type
            if (!otherContainer->hasDataOption(graphType)) continue;
            
            // Get the data source for this graph type
            WaterfallData *dataSource = layout->getDataSource(graphType);
            if (!dataSource) continue;
            
            // Check if symbol already exists at this timestamp (deduplication)
            // This prevents adding duplicate symbols when draw() is called multiple times
            std::vector<BTWSymbolData> existingSymbols = dataSource->getBTWSymbols();
            bool symbolExists = false;
            for (const auto& existingSymbol : existingSymbols)
            {
                // Check if symbol exists at the same timestamp (within 100ms tolerance)
                qint64 timeDiff = qAbs(existingSymbol.timestamp.msecsTo(timestamp));
                if (timeDiff < 100 && existingSymbol.symbolName == "MagentaCircle")
                {
                    symbolExists = true;
                    break;
                }
            }
            
            if (symbolExists) continue; // Skip if symbol already exists
            
            // Check if there's a datapoint at this timestamp
            bool hasDataPoint = false;
            qreal dataValue = 0.0;
            
            // Check all series in the data source
            std::vector<QString> seriesLabels = dataSource->getDataSeriesLabels();
            for (const QString &seriesLabel : seriesLabels)
            {
                // Get data points near this timestamp (within 1 second)
                const std::vector<QDateTime> &timestamps = dataSource->getTimestampsSeries(seriesLabel);
                const std::vector<qreal> &yData = dataSource->getYDataSeries(seriesLabel);
                
                for (size_t i = 0; i < timestamps.size(); ++i)
                {
                    qint64 timeDiff = qAbs(timestamps[i].msecsTo(timestamp));
                    if (timeDiff < 1000) // Within 1 second
                    {
                        hasDataPoint = true;
                        dataValue = yData[i];
                        break;
                    }
                }
                if (hasDataPoint) break;
            }
            
            // Only add symbol if there's a datapoint
            if (hasDataPoint)
            {
                // Add magenta circle symbol to this graph's data source
                dataSource->addBTWSymbol("MagentaCircle", timestamp, dataValue);
                
                // Trigger redraw of the graph
                WaterfallGraph *graph = otherContainer->getCurrentWaterfallGraph();
                if (graph && graph->getDataSource() == dataSource)
                {
                    graph->draw();
                }
            }
        }
    }
}

/**
 * @brief Get zoom panel from parent GraphContainer
 * @return Pointer to ZoomPanel, or nullptr if not found
 */
ZoomPanel* BTWGraph::getZoomPanel() const
{
    // Find parent GraphContainer
    QWidget *parent = this->parentWidget();
    if (!parent) {
        return nullptr;
    }
    
    // Try to find GraphContainer
    GraphContainer *container = qobject_cast<GraphContainer*>(parent);
    if (!container) {
        // If parent is not GraphContainer, try to find it in the widget hierarchy
        container = parent->findChild<GraphContainer*>();
    }
    
    if (!container) {
        return nullptr;
    }
    
    // Get zoom panel from GraphContainer (we need to access it through a public method)
    // Since GraphContainer doesn't expose getZoomPanel, we'll use findChild
    return container->findChild<ZoomPanel*>();
}

/**
 * @brief Add a shaded region to the graph
 * @param startX Starting X value (range value)
 * @param startY Starting Y value (timestamp)
 * @return Unique identifier for the shaded region
 */
int BTWGraph::addShadedRegion(qreal startX, qreal endX, const QDateTime &startY)
{
    int regionId = m_nextRegionId++;
    ShadedRegionData regionData(startX, endX, startY);
    ShadedRegionItem regionItem(regionData);
    m_shadedRegions[regionId] = regionItem;
    
    // Store reverse lookup for sync ID
    m_syncIdToRegionId[regionItem.syncId] = regionId;
    
    qDebug() << "BTWGraph: Added shaded region" << regionId << "syncId:" << regionItem.syncId.toString()
             << "X range:" << startX << "to" << endX << "Y:" << startY.toString("yyyy-MM-dd hh:mm:ss.zzz");
    
    // Trigger redraw to show the new region
    draw();
    
    // Emit sync signal
    ShadedRegionSyncData syncData(regionId, startX, endX);
    syncData.syncId = regionItem.syncId;  // Use the same sync ID
    emit shadedRegionAdded(syncData);
    
    return regionId;
}

/**
 * @brief Remove a shaded region by its identifier
 * @param regionId The identifier returned by addShadedRegion
 */
void BTWGraph::removeShadedRegion(int regionId)
{
    if (m_shadedRegions.contains(regionId)) {
        ShadedRegionItem &item = m_shadedRegions[regionId];
        QUuid syncId = item.syncId;  // Save sync ID before removal
        
        // Remove graphics item from scene if it exists
        if (item.polygonItem && graphicsScene) {
            graphicsScene->removeItem(item.polygonItem);
            delete item.polygonItem;
            item.polygonItem = nullptr;
        }
        
        // Remove from reverse lookup
        m_syncIdToRegionId.remove(syncId);
        
        m_shadedRegions.remove(regionId);
        qDebug() << "BTWGraph: Removed shaded region" << regionId << "syncId:" << syncId.toString();
        
        // Trigger redraw
        draw();
        
        // Emit sync signal
        emit shadedRegionRemoved(syncId);
    }
}

/**
 * @brief Clear all shaded regions
 */
void BTWGraph::clearShadedRegions()
{
    // Remove all graphics items from scene
    for (auto it = m_shadedRegions.begin(); it != m_shadedRegions.end(); ++it) {
        if (it.value().polygonItem && graphicsScene) {
            graphicsScene->removeItem(it.value().polygonItem);
            delete it.value().polygonItem;
        }
    }
    m_shadedRegions.clear();
    m_syncIdToRegionId.clear();
    qDebug() << "BTWGraph: Cleared all shaded regions";
    
    // Trigger redraw
    draw();
    
    // Emit sync signal
    emit shadedRegionsCleared();
}

/**
 * @brief Draw all shaded regions using zoom panel sticker values
 */
void BTWGraph::drawShadedRegions()
{
    if (!graphicsScene || m_shadedRegions.isEmpty() || !dataRangesValid) {
        return;
    }
    
    // Get time range for Y direction - region spans from top to bottom (all visible timestamps)
    QDateTime topTime = timeMin.isValid() ? timeMin : QDateTime::currentDateTime();
    QDateTime bottomTime = timeMax.isValid() ? timeMax : QDateTime::currentDateTime();
    
    // Ensure valid time range
    if (topTime >= bottomTime) {
        qDebug() << "BTWGraph: Invalid time range for shaded region";
        return;
    }
    
    // Draw each shaded region
    for (auto it = m_shadedRegions.begin(); it != m_shadedRegions.end(); ++it) {
        int regionId = it.key();
        ShadedRegionItem &item = it.value();
        const ShadedRegionData &data = item.data;
        
        // Remove existing polygon item if it exists
        if (item.polygonItem) {
            graphicsScene->removeItem(item.polygonItem);
            delete item.polygonItem;
            item.polygonItem = nullptr;
        }
        
        // Ensure valid X range
        if (data.startX >= data.endX) {
            qDebug() << "BTWGraph: Invalid X range for shaded region" << regionId << "- startX:" << data.startX << "endX:" << data.endX;
            continue;
        }
        
        // Create polygon for vertical shaded region
        // Region spans from data.startX to data.endX in X (horizontal boundaries)
        // and from topTime to bottomTime in Y (vertical, spans all timestamps from top to bottom)
        QVector<QPointF> polygonPoints;
        
        // Top-left corner: (startX, topTime)
        QPointF topLeft = mapDataToScreen(data.startX, topTime);
        // Top-right corner: (endX, topTime)
        QPointF topRight = mapDataToScreen(data.endX, topTime);
        // Bottom-right corner: (endX, bottomTime)
        QPointF bottomRight = mapDataToScreen(data.endX, bottomTime);
        // Bottom-left corner: (startX, bottomTime)
        QPointF bottomLeft = mapDataToScreen(data.startX, bottomTime);
        
        // Only draw if all points are within or near the drawing area
        bool shouldDraw = false;
        if (drawingArea.contains(topLeft) || drawingArea.contains(topRight) ||
            drawingArea.contains(bottomLeft) || drawingArea.contains(bottomRight)) {
            shouldDraw = true;
        } else {
            // Check if region intersects with drawing area
            QRectF regionRect = QRectF(
                qMin(qMin(topLeft.x(), topRight.x()), qMin(bottomLeft.x(), bottomRight.x())),
                qMin(qMin(topLeft.y(), topRight.y()), qMin(bottomLeft.y(), bottomRight.y())),
                qMax(qMax(topLeft.x(), topRight.x()), qMax(bottomLeft.x(), bottomRight.x())) - qMin(qMin(topLeft.x(), topRight.x()), qMin(bottomLeft.x(), bottomRight.x())),
                qMax(qMax(topLeft.y(), topRight.y()), qMax(bottomLeft.y(), bottomRight.y())) - qMin(qMin(topLeft.y(), topRight.y()), qMin(bottomLeft.y(), bottomRight.y()))
            );
            shouldDraw = drawingArea.intersects(regionRect);
        }
        
        if (shouldDraw) {
            polygonPoints.append(topLeft);
            polygonPoints.append(topRight);
            polygonPoints.append(bottomRight);
            polygonPoints.append(bottomLeft);
            
            // Create polygon item with hatch pattern (45 degree diagonal lines)
            QPolygonF polygon(polygonPoints);
            QGraphicsPolygonItem *polygonItem = new QGraphicsPolygonItem(polygon);
            
            // Create hatch pattern using a custom QPixmap
            // This is efficient - pattern is created once and Qt handles tiling
            const int patternSize = 10;  // Size of the pattern tile (controls line spacing)
            QPixmap patternPixmap(patternSize, patternSize);
            patternPixmap.fill(Qt::transparent);  // Transparent background
            
            QPainter patternPainter(&patternPixmap);
            patternPainter.setRenderHint(QPainter::Antialiasing, true);
            
            // Draw single diagonal line for hatch effect
            QColor lineColor(100, 100, 100, 180);  // Dark gray lines
            QPen linePen(lineColor, 1);
            patternPainter.setPen(linePen);
            
            // Forward diagonal line (/) - from bottom-left to top-right
            patternPainter.drawLine(0, patternSize, patternSize, 0);
            
            patternPainter.end();
            
            // Create brush with the hatch pattern
            QBrush hatchBrush(patternPixmap);
            
            // Set appearance with hatch fill and light border
            QColor borderColor(150, 150, 150, 200);  // Medium gray border
            
            polygonItem->setBrush(hatchBrush);
            polygonItem->setPen(QPen(borderColor, 1));
            polygonItem->setZValue(500);  // Below markers but above grid
            
            graphicsScene->addItem(polygonItem);
            item.polygonItem = polygonItem;
            
            qDebug() << "BTWGraph: Drew hatched shaded region" << regionId << "X range:" << data.startX 
                     << "to" << data.endX << "Y: full height (top to bottom)";
        }
    }
}

// ========== Marker Sync Methods Implementation ==========

bool BTWGraph::createMarkerFromSyncData(const BTWSyncMarkerData &markerData)
{
    if (!m_interactiveOverlay) {
        qDebug() << "BTWGraph: Cannot create marker from sync data - no overlay";
        return false;
    }
    
    // Check if marker already exists
    if (m_interactiveOverlay->hasMarker(markerData.id)) {
        qDebug() << "BTWGraph: Marker already exists, updating instead - ID:" << markerData.id.toString();
        return updateMarkerFromSyncData(markerData);
    }
    
    InteractiveGraphicsItem *marker = m_interactiveOverlay->createMarkerFromData(markerData);
    if (!marker) {
        qDebug() << "BTWGraph: Failed to create marker from sync data - ID:" << markerData.id.toString();
        return false;
    }
    
    qDebug() << "BTWGraph: Created marker from sync data - ID:" << markerData.id.toString()
             << "timestamp:" << markerData.timestamp.toString();
    
    return true;
}

bool BTWGraph::updateMarkerFromSyncData(const BTWSyncMarkerData &markerData)
{
    if (!m_interactiveOverlay) {
        qDebug() << "BTWGraph: Cannot update marker from sync data - no overlay";
        return false;
    }
    
    bool result = m_interactiveOverlay->updateMarkerFromData(markerData);
    if (result) {
        qDebug() << "BTWGraph: Updated marker from sync data - ID:" << markerData.id.toString();
    } else {
        qDebug() << "BTWGraph: Failed to update marker from sync data - ID:" << markerData.id.toString();
    }
    
    return result;
}

bool BTWGraph::deleteMarkerBySyncId(const QUuid &markerId)
{
    if (!m_interactiveOverlay) {
        qDebug() << "BTWGraph: Cannot delete marker - no overlay";
        return false;
    }
    
    bool result = m_interactiveOverlay->removeMarkerById(markerId);
    if (result) {
        qDebug() << "BTWGraph: Deleted marker by sync ID:" << markerId.toString();
    } else {
        qDebug() << "BTWGraph: Failed to delete marker by sync ID:" << markerId.toString();
    }
    
    return result;
}

bool BTWGraph::hasMarkerWithSyncId(const QUuid &markerId) const
{
    if (!m_interactiveOverlay) {
        return false;
    }
    
    return m_interactiveOverlay->hasMarker(markerId);
}

// ========== Shaded Region Sync Methods Implementation ==========

int BTWGraph::createShadedRegionFromSyncData(const ShadedRegionSyncData &regionData)
{
    // Check if region with this sync ID already exists
    if (m_syncIdToRegionId.contains(regionData.syncId)) {
        qDebug() << "BTWGraph: Shaded region with sync ID already exists:" << regionData.syncId.toString();
        return m_syncIdToRegionId[regionData.syncId];
    }
    
    // Create the region without emitting sync signal (to avoid loop)
    int regionId = m_nextRegionId++;
    ShadedRegionData data(regionData.startX, regionData.endX, QDateTime());  // No timestamp needed
    ShadedRegionItem regionItem(data);
    regionItem.syncId = regionData.syncId;  // Use the provided sync ID
    
    m_shadedRegions[regionId] = regionItem;
    m_syncIdToRegionId[regionData.syncId] = regionId;
    
    qDebug() << "BTWGraph: Created shaded region from sync data - localId:" << regionId 
             << "syncId:" << regionData.syncId.toString() << "X range:" << regionData.startX << "to" << regionData.endX;
    
    // Trigger redraw to show the new region
    draw();
    
    return regionId;
}

bool BTWGraph::deleteShadedRegionBySyncId(const QUuid &syncId)
{
    if (!m_syncIdToRegionId.contains(syncId)) {
        qDebug() << "BTWGraph: Cannot delete shaded region - sync ID not found:" << syncId.toString();
        return false;
    }
    
    int regionId = m_syncIdToRegionId[syncId];
    
    if (m_shadedRegions.contains(regionId)) {
        ShadedRegionItem &item = m_shadedRegions[regionId];
        
        // Remove graphics item from scene if it exists
        if (item.polygonItem && graphicsScene) {
            graphicsScene->removeItem(item.polygonItem);
            delete item.polygonItem;
            item.polygonItem = nullptr;
        }
        
        // Remove from both maps
        m_syncIdToRegionId.remove(syncId);
        m_shadedRegions.remove(regionId);
        
        qDebug() << "BTWGraph: Deleted shaded region by sync ID:" << syncId.toString();
        
        // Trigger redraw
        draw();
        
        return true;
    }
    
    return false;
}

bool BTWGraph::hasShadedRegionWithSyncId(const QUuid &syncId) const
{
    return m_syncIdToRegionId.contains(syncId);
}
