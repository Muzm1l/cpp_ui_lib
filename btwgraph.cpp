#include "btwgraph.h"
#include "btwinteractiveoverlay.h"
#include "interactivegraphicsitem.h"
#include "graphcontainer.h"
#include "graphlayout.h"
#include "waterfalldata.h"
#include "graphtype.h"
#include "zoompanel.h"
#include "sharedsyncstate.h"
#include "debugutils.h"
#include <QRandomGenerator>
#include <QPainter>
#include <QPixmap>
#include <mutex>

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
    , m_horizontalLineMode(HorizontalLineMode::Normal)
    , m_cachedWindowSize(QSize())
    , m_cachedMarkerRadius(0.0)
    , m_windowSizeCacheValid(false)
{
    // Setup interactive overlay
    setupInteractiveOverlay();
}

/**
 * @brief Destroy the BTWGraph::BTWGraph object
 *
 */
BTWGraph::~BTWGraph()
{
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
        return;
    }
    
    isDrawing = true;

    // Only perform full clear for FULL_REDRAW state
    // For INCREMENTAL_UPDATE or RANGE_UPDATE_ONLY, keep existing items and just update positions
    bool needsFullClear = (m_renderState == RenderState::FULL_REDRAW);
    
    if (needsFullClear)
    {
        // Clear existing items - ensure complete clearing before drawing
        // Automatic circle markers are in graphicsScene, so clearing graphicsScene removes them
        // Shaded region polygon items will be recreated in drawShadedRegions() from stored data
        // Clear polygon item pointers since clear() will delete them
        for (auto it = m_shadedRegions.begin(); it != m_shadedRegions.end(); ++it) {
            it.value().polygonItem = nullptr;
        }
        
        // Clear horizontal line items from scene before clearing
        // Note: graphicsScene->clear() will delete all items, so we need to null out pointers
        // and recreate them in drawHorizontalLines()
        for (auto &line : m_horizontalLines) {
            if (line.lineItem) {
                // Remove from scene before clear() deletes it
                if (graphicsScene->items().contains(line.lineItem)) {
                    graphicsScene->removeItem(line.lineItem);
                }
                // Delete the item since clear() will delete it anyway
                delete line.lineItem;
                line.lineItem = nullptr;  // Will be recreated in drawHorizontalLines()
            }
        }
        
        // Clear all item pointers since clear() will delete them
        // This prevents use-after-free in cleanup functions
        m_seriesScatterplotItems.clear();
        m_seriesPathItems.clear();
        m_seriesPointItems.clear();
        
        graphicsScene->clear();
        graphicsScene->update(); // Force immediate update to ensure clearing is visible
        
        // Clear stored timestamps when redrawing (markers will be recreated)
        m_automaticMarkerTimestamps.clear();
    }
    
    setupDrawingArea();

    // Grid only needs to be redrawn on full redraw
    if (needsFullClear && gridEnabled)
    {
        drawGrid();
    }

    if (dataSource && !dataSource->isEmpty())
    {
        updateDataRanges();
        
        // Draw scatterplots for each series with their respective colors
        // drawScatterplot() respects the render state internally
        std::vector<QString> seriesLabels = dataSource->getDataSeriesLabels();
        for (const QString &seriesLabel : seriesLabels)
        {
            if (isSeriesVisible(seriesLabel))
            {
                QColor seriesColor = getSeriesColor(seriesLabel);
                
                if (seriesLabel == "ADOPTED")
                {
                    // Draw curve for ADOPTED series without points
                    // Only redraw line on full clear
                    if (needsFullClear)
                    {
                        drawDataLine(seriesLabel, false);
                    }
                }
                else
                {
                    // Draw scatterplot for other series
                    // This respects render state and does incremental updates when possible
                    drawScatterplot(seriesLabel, seriesColor, 4.0, Qt::black);
                }

                // BTW markers are now manually placed through data source - no automatic generation
            }
        }
    }
    else if (dataSource && dataSource->isEmpty())
    {
        // Data source is empty - cleanup all scatterplot items to ensure they're removed
        cleanupAllScatterplotItems();
        DEBUG_OUT() << "BTWGraph: Data source is empty, cleaned up all scatterplot items";
    }
    
    // These items need to be redrawn when time range changes or data updates
    if (needsFullClear)
    {
        // Draw BTW symbols (magenta circles from other graphs)
        drawBTWSymbols();
        
        // Draw manually placed BTW markers from data source
        drawCustomCircleMarkers();
        
        // Draw shaded regions
        drawShadedRegions();
        
        // Draw horizontal lines
        drawHorizontalLines();
    }
    else
    {
        // Even on incremental updates, we need to update horizontal lines
        // because they can be added/removed dynamically
        drawHorizontalLines();
        
        // CRITICAL FIX: Also update shaded regions during incremental updates
        // Shaded regions need to be redrawn when time range changes (RANGE_UPDATE_ONLY)
        // or when data is added (INCREMENTAL_UPDATE) because their Y coordinates
        // depend on the time range. Without this, shaded regions don't appear
        // until a manual marker triggers a full redraw.
        drawShadedRegions();
        
        // CRITICAL FIX: Also update BTW symbols during incremental updates
        // Symbols need to be redrawn when time range changes (timer ticks, animation, zoom)
        // because their Y positions depend on the time range
        drawBTWSymbols();
    }
    
    // Sync interactive overlay markers with the new time range
    // This updates marker Y positions to stay in sync with the timeline
    if (m_interactiveOverlay) {
        m_interactiveOverlay->syncMarkersWithTimeline();
    }
    
    // Reset render state to clean after drawing
    setRenderState(RenderState::CLEAN);
    
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
    // Check if we clicked on an existing interactive marker in the overlay scene
    // The overlay scene and graphics scene share the same coordinate system
    if (m_interactiveOverlay && m_interactiveOverlay->getOverlayScene()) {
        QGraphicsItem *itemAtPos = m_interactiveOverlay->getOverlayScene()->itemAt(scenePos, QTransform());
        // Filter out crosshair items - they should not prevent marker creation
        if (itemAtPos && itemAtPos != crosshairHorizontal && itemAtPos != crosshairVertical) {
            // Don't add a new marker, let the interactive item handle the click
            return;
        }
    }
    
    // Horizontal line mode: draw line at clicked Y position (time)
    // Note: In BTW graphs, a "horizontal line" means constant time (horizontal on screen)
    // We use the Y position to determine the time, then draw a line spanning full width
    if (m_horizontalLineMode != HorizontalLineMode::Normal) {
        // Check if click is on an existing horizontal line (click-to-delete)
        // Use cached lineItem for efficient hit detection
        const qreal hitThreshold = 5.0; // pixels - click within 5px of line to delete
        qreal clickedY = scenePos.y();
        
        for (int i = 0; i < m_horizontalLines.size(); ++i) {
            if (m_horizontalLines[i].lineItem) {
                // Get line Y position directly from cached graphics item
                qreal lineY = m_horizontalLines[i].lineItem->line().y1();
                if (qAbs(clickedY - lineY) <= hitThreshold) {
                    // Click is on existing line - delete it
                    QUuid lineId = m_horizontalLines[i].id;
                    QDateTime timestamp = m_horizontalLines[i].timestamp; // Get timestamp before removal
                    removeHorizontalLine(lineId);
                    emit horizontalLineRemoved(lineId, timestamp);
                    DEBUG_OUT() << "BTWGraph: Deleted horizontal line by click, ID:" << lineId.toString() << "at" << timestamp.toString();
                    
                    // Redraw to remove the line
                    draw();
                    return;
                }
            }
        }
        
        // No existing line at click position
        if (m_horizontalLineMode == HorizontalLineMode::DeleteLine) {
            // In delete mode, only delete lines - don't add new ones
            DEBUG_OUT() << "BTWGraph: Delete mode - no line found at click position, ignoring";
            return;
        }
        
        // DrawLine mode: add new line
        QDateTime timestamp = mapScreenToTime(scenePos.y());
        if (!timestamp.isValid()) {
            timestamp = QDateTime::currentDateTime();
        }
        
        // Add horizontal line at this timestamp
        QUuid lineId = addHorizontalLine(timestamp);
        
        // Emit signal for horizontal line placement
        emit horizontalLinePlaced(lineId, timestamp);
        
        // Force full redraw to show the new line
        forceFullRedraw();
        return;  // Don't add marker in line mode
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
        }
        
        // Get value from X position (range value)
        qreal value = mapScreenXToRange(scenePos.x());
        QString seriesLabel = "BTW-Click";
        
        m_interactiveOverlay->addDataPointMarker(overlayPos, timestamp, value, seriesLabel);
        
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
}

/**
 * @brief Draw manually placed BTW circle markers from data source
 * Circle outline with a line whose angle is calculated from delta values
 */
void BTWGraph::drawCustomCircleMarkers()
{
    if (!dataSource || !graphicsScene) {
        return;
    }

    // Get manually placed markers from data source (filtered by time range using binary search)
    // Note: BTWMarkerData here is from waterfalldata.h (not BTWSyncMarkerData)
    std::vector<BTWMarkerData> visibleMarkers;
    bool timeRangeValid = timeMin.isValid() && timeMax.isValid() && timeMin <= timeMax;
    
    if (timeRangeValid) {
        // Use binary search-based filtering for O(log n) performance
        visibleMarkers = dataSource->getBTWMarkersWithinTimeRange(timeMin, timeMax);
    } else {
        visibleMarkers = dataSource->getBTWMarkers();
    }

    if (visibleMarkers.empty()) {
        return;
    }

    // Draw circle markers for each visible marker
    for (const auto& markerData : visibleMarkers) {
        QDateTime timestamp = markerData.timestamp;
        qreal range = markerData.range;
        qreal deltaValue = markerData.delta;
        
        QPointF screenPos = mapDataToScreen(range, timestamp);
        
        // Check if point is within visible area
        if (drawingArea.contains(screenPos)) {
            // Use cached window size and marker radius (Issue #3)
            // Cache is updated in resizeEvent() and initialized on first use
            if (!m_windowSizeCacheValid) {
                updateWindowSizeCache();
            }
            qreal markerRadius = m_cachedMarkerRadius; // Use cached radius instead of recalculating
            
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
        }
    }
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
 * @brief Update window size cache (Issue #3)
 */
void BTWGraph::updateWindowSizeCache()
{
    m_cachedWindowSize = this->size();
    // Calculate marker radius based on cached window size
    m_cachedMarkerRadius = std::min(0.04 * m_cachedWindowSize.width(), 12.0);
    m_windowSizeCacheValid = true;
}

void BTWGraph::resizeEvent(QResizeEvent *event)
{
    WaterfallGraph::resizeEvent(event);
    
    // Invalidate and update window size cache (Issue #3)
    m_windowSizeCacheValid = false;
    updateWindowSizeCache();
    
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
}

void BTWGraph::deleteInteractiveMarkers()
{
    if (!m_interactiveOverlay) {
        return;
    }

    m_interactiveOverlay->clearAllMarkers();
}


void BTWGraph::onMarkerAdded(InteractiveGraphicsItem *marker, int type)
{
    Q_UNUSED(type);
    if (!marker) {
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
        // Emit signal for external integration
        emit manualMarkerPlaced(timestamp, marker->scenePos());
    }
}

void BTWGraph::onMarkerRemoved(InteractiveGraphicsItem *marker, int type)
{
    Q_UNUSED(marker);
    Q_UNUSED(type);
}

void BTWGraph::onMarkerMoved(InteractiveGraphicsItem *marker, const QPointF &newPosition)
{
    if (!marker) {
        return;
    }
    
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
    }
}

void BTWGraph::onMarkerRotated(InteractiveGraphicsItem *marker, qreal angle)
{
    Q_UNUSED(angle);
    if (!marker) {
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
    
    // Update magenta circles when green marker is rotated (position might have changed)
    if (timestamp.isValid()) {
        emit manualMarkerPlaced(timestamp, marker->scenePos());
    }
}

void BTWGraph::onMarkerClicked(InteractiveGraphicsItem *marker, const QPointF &position)
{
    Q_UNUSED(position);
    if (!marker) {
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
        // Emit signal for external integration (legacy)
        emit manualMarkerClicked(timestamp, scenePos);
        
        // Emit signal for marker timestamp and value (legacy - propagates through GraphContainer -> GraphLayout)
        emit markerTimestampValueChanged(timestamp, rangeValue);
        
        // Emit new comprehensive signal with all three values
        emit markerClickedWithData(timestamp, rangeValue, bearingRate);
    }
}

std::vector<QDateTime> BTWGraph::getAutomaticMarkerTimestamps() const
{
    return m_automaticMarkerTimestamps;
}

/**
 * @brief Add a manual marker programmatically via API call
 * 
 * This method allows creating BTW manual markers programmatically,
 * similar to how they are created via mouse clicks.
 * 
 * @param timestamp The timestamp for the marker (Y-axis position)
 * @param rangeValue The range value for the marker (X-axis position)
 * @param bearingRate Optional bearing rate (rotation angle / 10.0). Defaults to 0.0
 * @return Pointer to the created marker, or nullptr if creation failed
 */
InteractiveGraphicsItem* BTWGraph::addBTWManualMarker(const QDateTime &timestamp, qreal rangeValue, qreal bearingRate)
{
    if (!m_interactiveOverlay) {
        DEBUG_OUT() << "BTWGraph::addBTWManualMarker: Interactive overlay not available";
        return nullptr;
    }
    
    if (!timestamp.isValid()) {
        DEBUG_OUT() << "BTWGraph::addBTWManualMarker: Invalid timestamp provided";
        return nullptr;
    }
    
    // Convert data coordinates (timestamp, rangeValue) to screen position
    QPointF screenPos = mapDataToScreen(rangeValue, timestamp);
    
    // Validate screen position
    if (screenPos.isNull() || !qIsFinite(screenPos.x()) || !qIsFinite(screenPos.y())) {
        DEBUG_OUT() << "BTWGraph::addBTWManualMarker: Failed to map data coordinates to screen position";
        return nullptr;
    }
    
    // Check if position is within visible drawing area
    if (!drawingArea.contains(screenPos)) {
        DEBUG_OUT() << "BTWGraph::addBTWManualMarker: Marker position is outside visible area";
        // Still create the marker, but warn about visibility
    }
    
    QString seriesLabel = "BTW-API"; // Distinguish API-created markers from click-created ones
    
    // Create the marker using the interactive overlay
    InteractiveGraphicsItem* marker = m_interactiveOverlay->addDataPointMarker(screenPos, timestamp, rangeValue, seriesLabel);
    
    if (marker) {
        // Set bearing rate (rotation) if provided
        if (bearingRate != 0.0) {
            marker->setRotation(bearingRate * 10.0);
            // Also store bearing rate in marker data for consistency
            marker->setData(1, QVariant::fromValue(bearingRate));
        }
        
        // Emit signal for marker timestamp and value (same as click handler)
        emit markerTimestampValueChanged(timestamp, rangeValue);
        
        // Emit manualMarkerPlaced signal (same as click handler)
        emit manualMarkerPlaced(timestamp, screenPos);
        
        DEBUG_OUT() << "BTWGraph::addBTWManualMarker: Marker created at timestamp" 
                 << timestamp.toString("yyyy-MM-dd HH:mm:ss.zzz")
                 << "range:" << rangeValue << "bearingRate:" << bearingRate;
    } else {
        DEBUG_OUT() << "BTWGraph::addBTWManualMarker: Failed to create marker";
    }
    
    return marker;
}

void BTWGraph::addBTWSymbol(const QString &symbolName, const QDateTime &timestamp, qreal range)
{
    // Store symbol in dataSource (WaterfallData) so it persists with track data
    if (!dataSource)
    {
        return;
    }
    
    dataSource->addBTWSymbol(symbolName, timestamp, range);
    
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
    
    // CRITICAL FIX: Remove old BTW symbol items (magenta circles) before drawing new ones
    // This prevents duplicates when time range changes and symbols are redrawn
    // Only remove if not doing a full clear (full clear already cleared the scene)
    if (m_renderState != RenderState::FULL_REDRAW)
    {
        // Remove all QGraphicsPixmapItem objects with z-value 1003 (BTW symbols/magenta circles)
        QList<QGraphicsItem*> allItems = graphicsScene->items();
        for (QGraphicsItem* item : allItems)
        {
            QGraphicsPixmapItem* pixmapItem = qgraphicsitem_cast<QGraphicsPixmapItem*>(item);
            if (pixmapItem && pixmapItem->zValue() == 1003)
            {
                graphicsScene->removeItem(pixmapItem);
                delete pixmapItem;
            }
        }
    }
    
    // Get symbols from dataSource (filtered by time range using binary search)
    std::vector<BTWSymbolData> visibleSymbols;
    bool timeRangeValid = timeMin.isValid() && timeMax.isValid() && timeMin <= timeMax;
    
    if (timeRangeValid)
    {
        // Use binary search-based filtering for O(log n) performance
        visibleSymbols = dataSource->getBTWSymbolsWithinTimeRange(timeMin, timeMax);
    }
    else
    {
        visibleSymbols = dataSource->getBTWSymbols();
    }
    
    if (visibleSymbols.empty())
    {
        return;
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
        
        // Validate pixmap before using it (Issue #4: Use pixmap dimensions directly)
        if (symbolPixmap.isNull() || symbolPixmap.width() <= 0 || symbolPixmap.height() <= 0)
        {
            continue;
        }
        
        // Create a graphics pixmap item and add it to the scene
        QGraphicsPixmapItem* pixmapItem = new QGraphicsPixmapItem(symbolPixmap);
        
        // Center the symbol on the data point (Issue #4: Use pixmap dimensions directly instead of boundingRect)
        // boundingRect() is expensive - use pixmap dimensions directly
        qreal pixmapWidth = symbolPixmap.width();
        qreal pixmapHeight = symbolPixmap.height();
        pixmapItem->setPos(screenPos.x() - pixmapWidth/2, screenPos.y() - pixmapHeight/2);
        pixmapItem->setZValue(1003); // Above markers but below interactive items
        
        graphicsScene->addItem(pixmapItem);
    }
}

void BTWGraph::addBTWSymbolToOtherGraphs(const QDateTime &timestamp, qreal btwValue)
{
    Q_UNUSED(btwValue);
    
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
            // Use binary search to check symbols within a small time window (100ms tolerance)
            QDateTime checkStart = timestamp.addMSecs(-100);
            QDateTime checkEnd = timestamp.addMSecs(100);
            std::vector<BTWSymbolData> nearbySymbols = dataSource->getBTWSymbolsWithinTimeRange(checkStart, checkEnd);
            bool symbolExists = false;
            for (const auto& existingSymbol : nearbySymbols)
            {
                // Check if symbol exists at the same timestamp (within 100ms tolerance) and same name
                if (existingSymbol.symbolName == "MagentaCircle")
                {
                    symbolExists = true;
                    break;
                }
            }
            
            if (symbolExists) continue; // Skip if symbol already exists
            
            // Check if there's a datapoint at this timestamp using binary search
            bool hasDataPoint = false;
            qreal dataValue = 0.0;
            size_t unusedIndex;
            
            // Check all series in the data source using binary search
            std::vector<QString> seriesLabels = dataSource->getDataSeriesLabels();
            for (const QString &seriesLabel : seriesLabels)
            {
                // Use binary search to find closest data point (within 1 second = 1000ms)
                if (dataSource->findClosestDataPoint(seriesLabel, timestamp, 1000, dataValue, unusedIndex))
                {
                    hasDataPoint = true;
                    break;
                }
            }
            
            // Only add symbol if there's a datapoint
            if (hasDataPoint)
            {
                // Add magenta circle symbol to this graph's data source
                dataSource->addBTWSymbol("MagentaCircle", timestamp, dataValue);
                
                // Trigger full redraw of the graph since we added new data
                WaterfallGraph *graph = otherContainer->getCurrentWaterfallGraph();
                if (graph && graph->getDataSource() == dataSource)
                {
                    graph->forceFullRedraw();
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
    Q_UNUSED(startY);
    
    int regionId = m_nextRegionId++;
    ShadedRegionData regionData(startX, endX, startY);
    ShadedRegionItem regionItem(regionData);
    m_shadedRegions[regionId] = regionItem;
    
    // Store reverse lookup for sync ID
    m_syncIdToRegionId[regionItem.syncId] = regionId;
    
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
    
    // Trigger redraw
    draw();
    
    // Emit sync signal
    emit shadedRegionsCleared();
}

/**
 * @brief Draw all shaded regions using zoom panel sticker values
 */
// Static cached hatch brush for shaded regions (shared across all instances)
QBrush BTWGraph::getCachedHatchBrush()
{
    static QBrush s_cachedHatchBrush;
    static std::once_flag brushInitFlag;
    
    std::call_once(brushInitFlag, []() {
        // Create hatch pattern using a custom QPixmap
        // This is created once and reused for all shaded regions
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
        s_cachedHatchBrush = QBrush(patternPixmap);
    });
    
    return s_cachedHatchBrush;
}

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
        return;
    }
    
    // OPTIMIZATION: Cache Y coordinates (same for all regions since they span full height)
    // Only need to map one point to get Y coordinates, then reuse for all regions
    QPointF topLeftBase = mapDataToScreen(0.0, topTime);
    QPointF bottomLeftBase = mapDataToScreen(0.0, bottomTime);
    qreal topY = topLeftBase.y();
    qreal bottomY = bottomLeftBase.y();
    
    // Get cached hatch brush (created once, reused for all regions)
    QBrush hatchBrush = getCachedHatchBrush();
    QColor borderColor(150, 150, 150, 200);  // Medium gray border
    
    // Draw each shaded region
    for (auto it = m_shadedRegions.begin(); it != m_shadedRegions.end(); ++it) {
        ShadedRegionItem &item = it.value();
        const ShadedRegionData &data = item.data;
        
        // Ensure valid X range
        if (data.startX >= data.endX) {
            // Remove item if it exists and region is invalid
            if (item.polygonItem) {
                graphicsScene->removeItem(item.polygonItem);
                delete item.polygonItem;
                item.polygonItem = nullptr;
            }
            continue;
        }
        
        // OPTIMIZATION: Only calculate X coordinates (Y is same for all regions)
        // Map startX and endX to screen coordinates - only need X values
        QPointF startScreen = mapDataToScreen(data.startX, topTime);
        QPointF endScreen = mapDataToScreen(data.endX, topTime);
        qreal startX = startScreen.x();
        qreal endX = endScreen.x();
        
        // OPTIMIZATION: Simplified visibility check using X coordinates only
        // Since regions span full height, only need to check if X range intersects drawing area
        bool shouldDraw = (endX >= drawingArea.left() && startX <= drawingArea.right());
        
        if (shouldDraw) {
            // OPTIMIZATION: Update existing polygon instead of recreating
            if (item.polygonItem) {
                // Update polygon coordinates
                QPolygonF polygon;
                polygon << QPointF(startX, topY)
                        << QPointF(endX, topY)
                        << QPointF(endX, bottomY)
                        << QPointF(startX, bottomY);
                item.polygonItem->setPolygon(polygon);
            } else {
                // Create new item only if it doesn't exist
                QPolygonF polygon;
                polygon << QPointF(startX, topY)
                        << QPointF(endX, topY)
                        << QPointF(endX, bottomY)
                        << QPointF(startX, bottomY);
                
                QGraphicsPolygonItem *polygonItem = new QGraphicsPolygonItem(polygon);
                polygonItem->setBrush(hatchBrush);
                polygonItem->setPen(QPen(borderColor, 1));
                polygonItem->setZValue(500);  // Below markers but above grid
                
                graphicsScene->addItem(polygonItem);
                item.polygonItem = polygonItem;
            }
        } else {
            // Remove item if region is not visible
            if (item.polygonItem) {
                graphicsScene->removeItem(item.polygonItem);
                delete item.polygonItem;
                item.polygonItem = nullptr;
            }
        }
    }
}

// ========== Marker Sync Methods Implementation ==========

bool BTWGraph::createMarkerFromSyncData(const BTWSyncMarkerData &markerData)
{
    if (!m_interactiveOverlay) {
        return false;
    }
    
    // Check if marker already exists
    if (m_interactiveOverlay->hasMarker(markerData.id)) {
        return updateMarkerFromSyncData(markerData);
    }
    
    InteractiveGraphicsItem *marker = m_interactiveOverlay->createMarkerFromData(markerData);
    if (!marker) {
        return false;
    }
    
    return true;
}

bool BTWGraph::updateMarkerFromSyncData(const BTWSyncMarkerData &markerData)
{
    if (!m_interactiveOverlay) {
        return false;
    }
    
    return m_interactiveOverlay->updateMarkerFromData(markerData);
}

bool BTWGraph::deleteMarkerBySyncId(const QUuid &markerId)
{
    if (!m_interactiveOverlay) {
        return false;
    }
    
    return m_interactiveOverlay->removeMarkerById(markerId);
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
        return m_syncIdToRegionId[regionData.syncId];
    }
    
    // Create the region without emitting sync signal (to avoid loop)
    int regionId = m_nextRegionId++;
    ShadedRegionData data(regionData.startX, regionData.endX, QDateTime());  // No timestamp needed
    ShadedRegionItem regionItem(data);
    regionItem.syncId = regionData.syncId;  // Use the provided sync ID
    
    m_shadedRegions[regionId] = regionItem;
    m_syncIdToRegionId[regionData.syncId] = regionId;
    
    // Trigger redraw to show the new region
    draw();
    
    return regionId;
}

bool BTWGraph::deleteShadedRegionBySyncId(const QUuid &syncId)
{
    if (!m_syncIdToRegionId.contains(syncId)) {
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

// ========== Horizontal Line Management ==========

void BTWGraph::setHorizontalLineMode(HorizontalLineMode mode)
{
    m_horizontalLineMode = mode;
    const char* modeStr = (mode == HorizontalLineMode::Normal) ? "Normal" :
                          (mode == HorizontalLineMode::DrawLine) ? "DrawLine" : "DeleteLine";
    DEBUG_OUT() << "BTWGraph: Horizontal line mode set to" << modeStr;
}

void BTWGraph::setHorizontalLineMode(bool enabled)
{
    // Legacy boolean interface for backward compatibility
    setHorizontalLineMode(enabled ? HorizontalLineMode::DrawLine : HorizontalLineMode::Normal);
}

BTWGraph::HorizontalLineMode BTWGraph::getHorizontalLineMode() const
{
    return m_horizontalLineMode;
}

bool BTWGraph::isHorizontalLineMode() const
{
    // Legacy method - returns true if in DrawLine or DeleteLine mode
    return m_horizontalLineMode != HorizontalLineMode::Normal;
}

QUuid BTWGraph::addHorizontalLine(const QDateTime &timestamp, const QColor &color, qreal width)
{
    HorizontalLineItem lineItem(timestamp, color, width);
    m_horizontalLines.append(lineItem);
    
    DEBUG_OUT() << "BTWGraph: Added horizontal line at time:" << timestamp.toString() << "ID:" << lineItem.id.toString();
    
    // New line doesn't have a cached item yet - will be created in next draw()
    // No need to invalidate existing cached items
    
    return lineItem.id;
}

QDateTime BTWGraph::getHorizontalLineTimestamp(const QUuid &lineId) const
{
    for (const auto &line : m_horizontalLines)
    {
        if (line.id == lineId)
        {
            return line.timestamp;
        }
    }
    
    return QDateTime(); // Return invalid QDateTime if not found
}

bool BTWGraph::removeHorizontalLine(const QUuid &lineId)
{
    for (int i = 0; i < m_horizontalLines.size(); ++i) {
        if (m_horizontalLines[i].id == lineId) {
            // Remove graphics item if it exists
            if (m_horizontalLines[i].lineItem) {
                if (graphicsScene) {
                    graphicsScene->removeItem(m_horizontalLines[i].lineItem);
                }
                delete m_horizontalLines[i].lineItem;
            }
            
            m_horizontalLines.removeAt(i);
            DEBUG_OUT() << "BTWGraph: Removed horizontal line ID:" << lineId.toString();
            return true;
        }
    }
    
    return false;
}

int BTWGraph::removeHorizontalLineByTimestamp(const QDateTime &timestamp, qreal toleranceMs)
{
    int removedCount = 0;
    qint64 toleranceMicroseconds = static_cast<qint64>(toleranceMs * 1000.0);
    
    // Iterate backwards to safely remove items
    for (int i = m_horizontalLines.size() - 1; i >= 0; --i) {
        qint64 timeDiff = qAbs(m_horizontalLines[i].timestamp.msecsTo(timestamp));
        if (timeDiff <= toleranceMicroseconds) {
            QUuid lineId = m_horizontalLines[i].id;
            
            // Remove graphics item if it exists
            if (m_horizontalLines[i].lineItem) {
                if (graphicsScene) {
                    graphicsScene->removeItem(m_horizontalLines[i].lineItem);
                }
                delete m_horizontalLines[i].lineItem;
            }
            
            m_horizontalLines.removeAt(i);
            removedCount++;
            DEBUG_OUT() << "BTWGraph: Removed horizontal line by timestamp:" << timestamp.toString() << "ID:" << lineId.toString();
        }
    }
    
    return removedCount;
}

void BTWGraph::clearHorizontalLines()
{
    // Remove all graphics items
    for (auto &line : m_horizontalLines) {
        if (line.lineItem) {
            if (graphicsScene) {
                graphicsScene->removeItem(line.lineItem);
            }
            delete line.lineItem;
            line.lineItem = nullptr;
        }
    }
    
    m_horizontalLines.clear();
    DEBUG_OUT() << "BTWGraph: Cleared all horizontal lines";
}

void BTWGraph::drawHorizontalLines()
{
    if (!graphicsScene || !dataRangesValid || drawingArea.isEmpty()) {
        return;
    }
    
    // Update or create cached line items
    for (auto &line : m_horizontalLines) {
        // Calculate screen Y position from timestamp (horizontal line = constant time)
        qreal screenY = mapTimeToY(line.timestamp);
        
        // Skip if timestamp is outside visible range
        if (screenY < 0 || screenY < drawingArea.top() || screenY > drawingArea.bottom()) {
            // Line is outside visible range, but keep the cached item
            // Just hide it or remove it from scene
            if (line.lineItem && graphicsScene->items().contains(line.lineItem)) {
                graphicsScene->removeItem(line.lineItem);
            }
            continue;
        }
        
        // Clamp to drawing area
        screenY = qMax(drawingArea.top(), qMin(drawingArea.bottom(), screenY));
        
        if (line.lineItem) {
            // Update existing cached line - just update position
            line.lineItem->setLine(drawingArea.left(), screenY, drawingArea.right(), screenY);
            
            // Re-add to scene if it was removed
            if (!graphicsScene->items().contains(line.lineItem)) {
                graphicsScene->addItem(line.lineItem);
            }
        } else {
            // Create new cached line item (horizontal line spanning full width)
            line.lineItem = new QGraphicsLineItem(drawingArea.left(), screenY, drawingArea.right(), screenY);
            line.lineItem->setPen(QPen(line.color, line.width));
            line.lineItem->setZValue(1000);  // Above data, below markers
            
            graphicsScene->addItem(line.lineItem);
        }
    }
}
