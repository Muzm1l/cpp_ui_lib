#include "rtwgraph.h"
#include "waterfalldata.h"  // For RTWRMarkerData
#include "debugutils.h"
#include <QDebug>
#include <QGraphicsTextItem>
#include <QGraphicsPixmapItem>
#include <QFont>
#include <QTime>
#include <QtMath>

/**
 * @brief Construct a new RTWGraph::RTWGraph object
 *
 * @param parent Parent widget
 * @param enableGrid Whether to enable grid display
 * @param gridDivisions Number of grid divisions
 * @param timeInterval Time interval for the waterfall display
 */
RTWGraph::RTWGraph(QWidget *parent, bool enableGrid, int gridDivisions, TimeInterval timeInterval)
    : WaterfallGraph(parent, enableGrid, gridDivisions, timeInterval), symbols(40)
{
    // Set hard limits for RTW graph: 0 to 25
    setCustomYRange(0.0, 25.0);
    setRangeLimitingEnabled(true);
    
    DEBUG_OUT() << "RTWGraph constructor called with hard limits 0-25";
}

/**
 * @brief Destroy the RTWGraph::RTWGraph object
 *
 */
RTWGraph::~RTWGraph()
{
    DEBUG_OUT() << "RTWGraph destructor called";
}

/**
 * @brief Override draw method to create scatterplots by default
 *
 */
void RTWGraph::draw()
{
    DEBUG_OUT() << "RTW: draw() called";
    
    if (!graphicsScene) {
        DEBUG_OUT() << "RTW: draw() early return - no graphicsScene";
        return;
    }
    
    // Prevent concurrent drawing to avoid marker duplication
    if (isDrawing) {
        DEBUG_OUT() << "RTWGraph: draw() already in progress, skipping";
        return;
    }
    
    isDrawing = true;

    // Only perform full clear for FULL_REDRAW state
    bool needsFullClear = (m_renderState == RenderState::FULL_REDRAW);
    
    if (needsFullClear)
    {
        // Clear all item pointers since clear() will delete them
        // This prevents use-after-free in cleanup functions
        m_seriesScatterplotItems.clear();
        m_seriesPathItems.clear();
        m_seriesPointItems.clear();
        
        // CRITICAL FIX: R markers are now in overlayScene, not graphicsScene
        // They will be cleared when overlayScene items are removed, but we need to track them
        // (R markers are cleared in drawCustomRMarkers() before drawing new ones)
        
        graphicsScene->clear();
        graphicsScene->update(); // Force immediate update to ensure clearing is visible
        
        // Do not clear overlayScene wholesale. It owns persistent UI items
        // (crosshair/selection/etc.) and clearing it can delete pooled BTW symbols
        // behind m_btwSymbolItems, leaving stale pointers and causing crashes.
        clearBTWSymbolOverlayItems();
    }
    
    setupDrawingArea();

    if (needsFullClear && gridEnabled)
    {
        drawGrid();
    }

    if (dataSource && !dataSource->isEmpty())
    {
        DEBUG_OUT() << "RTW: draw() - dataSource available, updating ranges and drawing series";
        updateDataRanges();
        
        // Debug: Show current Y range
        DEBUG_OUT() << "RTW: Current Y range:" << yMin << "to" << yMax;
        
        // Debug: Show data source info
        DEBUG_OUT() << "RTW: Data source title:" << dataSource->getDataTitle();
        DEBUG_OUT() << "RTW: Data source empty?" << dataSource->isEmpty();
        
        // RTW should only have 1 series - get the first (and only) series
        std::vector<QString> seriesLabels = dataSource->getDataSeriesLabels();
        DEBUG_OUT() << "RTW: draw() - found" << seriesLabels.size() << "series labels";
        
        // Debug: Show all series labels
        for (const QString& label : seriesLabels) {
            DEBUG_OUT() << "RTW: Series label:" << label << "size:" << dataSource->getDataSeriesSize(label);
        }
        
        // Draw all series - ADOPTED as line, others as R markers
        for (const QString &seriesLabel : seriesLabels)
        {
            if (isSeriesVisible(seriesLabel))
            {
                if (seriesLabel == "ADOPTED")
                {
                    // Draw ADOPTED series as line
                    // CRITICAL FIX: Draw during both full redraw and incremental updates
                    // The line needs to update when data changes or time range changes
                    if (needsFullClear || m_renderState == RenderState::RANGE_UPDATE_ONLY || m_renderState == RenderState::INCREMENTAL_UPDATE)
                    {
                        DEBUG_OUT() << "RTW: draw() - drawing ADOPTED series as line";
                        drawDataLine(seriesLabel, false);
                    }
                }
                else
                {
                    // RTW R markers are now manually placed through data source - no automatic generation
                    // Draw data as scatterplot for other series - respects render state internally
                    drawScatterplot(seriesLabel, getSeriesColor(seriesLabel), 4.0, Qt::black);
                }
            }
        }
    }
    else if (dataSource && dataSource->isEmpty())
    {
        // Data source is empty - cleanup all scatterplot items to ensure they're removed
        cleanupAllScatterplotItems();
        
        // CRITICAL FIX: Clear data line paths (ADOPTED series line)
        // These paths are rendered in paintEvent() and may contain gaps from when
        // BTW symbols were present. When data is cleared, these old paths must be
        // cleared too, otherwise the line with gaps remains visible.
        m_dataLinePaths.clear();
        m_batchedLinePaths.clear();
        m_dataLineColors.clear();
        
        // Trigger repaint to clear the line from screen
        update();
        
        DEBUG_OUT() << "RTWGraph: Data source is empty, cleaned up all scatterplot items and data line paths";
    }
    else
    {
        DEBUG_OUT() << "RTW: draw() - no dataSource";
    }

    // These items need to be redrawn when time range changes or data updates
    // CRITICAL FIX: Always draw symbols and markers - they need to update positions
    // when time range changes (timer ticks, animation, zoom) or when new symbols are added
    // Symbols are positioned based on time range, so they must be redrawn whenever
    // the time range changes (INCREMENTAL_UPDATE, RANGE_UPDATE_ONLY, or FULL_REDRAW)
    if (needsFullClear || m_renderState == RenderState::RANGE_UPDATE_ONLY || m_renderState == RenderState::INCREMENTAL_UPDATE)
    {
        // Draw manually placed RTW R markers from data source
        drawCustomRMarkers();
        
        // Draw RTW symbols (will remove old ones if not full clear)
        drawRTWSymbols();
        
        // Draw BTW symbols (magenta circles from BTW graph markers) (will remove old ones if not full clear)
        drawBTWSymbols();

        // Draw ruler indicators (numbered circles) above symbols
        drawRulers();
    }
    
    // Reset render state to clean after drawing
    setRenderState(RenderState::CLEAN);
    
    isDrawing = false;
}

void RTWGraph::refreshOverlaysAfterVisibleTimeRangeChange()
{
    WaterfallGraph::refreshOverlaysAfterVisibleTimeRangeChange();
    augmentOverlayPassAfterSymbols();
}

void RTWGraph::augmentOverlayPassAfterSymbols()
{
    drawCustomRMarkers();
    drawRTWSymbols();
    drawRulers();
}

/**
 * @brief Handle mouse click events specific to RTW graph
 *
 * @param scenePos Scene position of the click
 */
void RTWGraph::onMouseClick(const QPointF &scenePos)
{
    DEBUG_OUT() << "RTWGraph mouse clicked at scene position:" << scenePos;
    
    // CRITICAL FIX: Check if we clicked on an R marker in overlayScene (not graphicsScene)
    // R markers are now in overlayScene, not graphicsScene
    if (overlayScene) {
        // Use a more robust detection method: check all items at the position
        // and also check items within a small bounding box around the click
        // This handles cases where the click is slightly off the text bounding box
        
        // First, try the exact position
        QGraphicsItem *itemAtPos = overlayScene->itemAt(scenePos, QTransform());
        
        // If no item found at exact position, try a small bounding box search
        // This helps when clicking near but not exactly on the text
        if (!itemAtPos) {
            const qreal searchRadius = 10.0; // Search within 10 pixels
            QRectF searchRect(scenePos.x() - searchRadius, scenePos.y() - searchRadius,
                           searchRadius * 2, searchRadius * 2);
            QList<QGraphicsItem*> itemsInArea = overlayScene->items(searchRect, Qt::IntersectsItemShape, Qt::DescendingOrder);
            
            // Look for R markers in the nearby items
            for (QGraphicsItem *item : itemsInArea) {
                QGraphicsTextItem *textItem = qgraphicsitem_cast<QGraphicsTextItem*>(item);
                if (textItem && textItem->toPlainText() == "R" && textItem->defaultTextColor() == Qt::yellow) {
                    itemAtPos = item;
                    DEBUG_OUT() << "RTWGraph: Found R marker using bounding box search";
                    break;
                }
            }
        }
        
        DEBUG_OUT() << "RTWGraph: itemAtPos:" << itemAtPos << "at scene position:" << scenePos;
        if (itemAtPos) {
            QGraphicsTextItem *textItem = qgraphicsitem_cast<QGraphicsTextItem*>(itemAtPos);
            DEBUG_OUT() << "RTWGraph: textItem:" << textItem;
            if (textItem) {
                QString text = textItem->toPlainText();
                DEBUG_OUT() << "RTWGraph: Text item text:" << text;
                if (text == "R" && textItem->defaultTextColor() == Qt::yellow) {
                    // This is an R marker - calculate timestamp from Y position
                    // Use the marker's actual Y position for more accuracy
                    qreal yPos = textItem->scenePos().y() + textItem->boundingRect().height() / 2.0;
                    QDateTime timestamp = mapScreenToTime(yPos);
                    
                    if (timestamp.isValid()) {
                        DEBUG_OUT() << "========================================";
                        DEBUG_OUT() << "RTW R MARKER SELECTED - TIMESTAMP RETURNED";
                        DEBUG_OUT() << "========================================";
                        DEBUG_OUT() << "RTWGraph: R marker clicked at scene position:" << scenePos;
                        DEBUG_OUT() << "RTWGraph: Marker Y position:" << yPos;
                        DEBUG_OUT() << "RTWGraph: TIMESTAMP:" << timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz");
                        DEBUG_OUT() << "========================================";
                        
                        // Emit signal for external integration
                        emit rMarkerTimestampCaptured(timestamp, scenePos);
                    } else {
                        DEBUG_OUT() << "RTWGraph: R marker clicked at:" << scenePos << "- Could not determine timestamp (invalid)";
                    }
                    // Don't call parent - we've handled the R marker click
                    return;
                }
            }
            
            // Check if we clicked on a ruler indicator (tagged pixmap item)
            QGraphicsPixmapItem *rulerCandidate = qgraphicsitem_cast<QGraphicsPixmapItem*>(itemAtPos);
            if (rulerCandidate && rulerCandidate->data(1).toString() == QStringLiteral("RULER")) {
                int rulerIndex = rulerCandidate->data(3).toInt();
                if (rulerIndex >= 0 && rulerIndex < RulerCount && m_rulers[rulerIndex].active) {
                    setSelectedRuler(rulerIndex);
                    DEBUG_OUT() << "RTW RULER SELECTED - index:" << rulerIndex;
                    emit rulerSelected(rulerIndex, m_rulers[rulerIndex].timestamp, m_rulers[rulerIndex].range);
                }
                // Don't call parent - we've handled the ruler click
                return;
            }

            // Check if we clicked on an RTW symbol (QGraphicsPixmapItem) in overlayScene
            QGraphicsPixmapItem *pixmapItem = qgraphicsitem_cast<QGraphicsPixmapItem*>(itemAtPos);
            if (pixmapItem) {
                // Check if this pixmap item has symbol data stored (data(0) = timestamp)
                QVariant timestampVariant = pixmapItem->data(0);
                QVariant symbolNameVariant = pixmapItem->data(1);
                
                if (timestampVariant.isValid() && timestampVariant.canConvert<QDateTime>() && 
                    symbolNameVariant.isValid()) {
                    // This is an RTW symbol - get timestamp and symbol name from stored data
                    QDateTime timestamp = timestampVariant.value<QDateTime>();
                    QString symbolName = symbolNameVariant.toString();
                    
                    if (timestamp.isValid()) {
                        DEBUG_OUT() << "========================================";
                        DEBUG_OUT() << "RTW SYMBOL SELECTED - TIMESTAMP RETURNED";
                        DEBUG_OUT() << "========================================";
                        DEBUG_OUT() << "RTWGraph: Symbol clicked at scene position:" << scenePos;
                        DEBUG_OUT() << "RTWGraph: Symbol name:" << symbolName;
                        DEBUG_OUT() << "RTWGraph: TIMESTAMP:" << timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz");
                        DEBUG_OUT() << "========================================";
                        
                        // Emit signal for external integration
                        emit rtwSymbolTimestampCaptured(timestamp, scenePos, symbolName);
                    } else {
                        DEBUG_OUT() << "RTWGraph: RTW symbol clicked but timestamp is invalid";
                    }
                    // Don't call parent - we've handled the symbol click
                    return;
                }
            }
        }
    }
    
    // Call parent implementation for other clicks
    WaterfallGraph::onMouseClick(scenePos);
}

/**
 * @brief Handle mouse drag events specific to RTW graph
 *
 * @param scenePos Scene position of the drag
 */
void RTWGraph::onMouseDrag(const QPointF &scenePos)
{
    DEBUG_OUT() << "RTWGraph mouse dragged to scene position:" << scenePos;
    // Call parent implementation
    WaterfallGraph::onMouseDrag(scenePos);
}

/**
 * @brief Draw manually placed RTW R markers from data source
 */
void RTWGraph::drawCustomRMarkers()
{
    if (!dataSource || !overlayScene) {  // Changed from graphicsScene to overlayScene
        DEBUG_OUT() << "RTW: drawCustomRMarkers early return - no dataSource or overlayScene";
        return;
    }

    // Get manually placed markers from data source
    std::vector<RTWRMarkerData> rMarkers = dataSource->getRTWRMarkers();
    
    if (rMarkers.empty()) {
        DEBUG_OUT() << "RTW: No manually placed R markers in data source";
        // CRITICAL FIX: Still need to clear old R markers even if data source is empty
        // This ensures R markers are removed when clearAllGraphs() clears the data source
        QList<QGraphicsItem*> allItems = overlayScene->items();
        QList<QGraphicsItem*> itemsToRemove;
        for (QGraphicsItem* item : allItems)
        {
            QGraphicsTextItem* textItem = qgraphicsitem_cast<QGraphicsTextItem*>(item);
            if (textItem && textItem->toPlainText() == "R" && textItem->defaultTextColor() == Qt::yellow)
            {
                itemsToRemove.append(item);
            }
        }
        for (QGraphicsItem* item : itemsToRemove)
        {
            overlayScene->removeItem(item);
            delete item;
        }
        return;
    }

    // Filter markers to only include those within the visible time range
    std::vector<RTWRMarkerData> visibleMarkers;
    bool timeRangeValid = timeMin.isValid() && timeMax.isValid() && timeMin <= timeMax;
    
    if (timeRangeValid) {
        for (const auto& markerData : rMarkers) {
            if (markerData.timestamp >= timeMin && markerData.timestamp <= timeMax) {
                visibleMarkers.push_back(markerData);
            }
        }
    } else {
        visibleMarkers = rMarkers;
    }

    // CRITICAL FIX: Always remove old R marker items before drawing new ones
    // This prevents duplicates when drawCustomRMarkers() is called multiple times
    // R markers are now in overlayScene, not graphicsScene, so they need explicit cleanup
    QList<QGraphicsItem*> allItems = overlayScene->items();
    QList<QGraphicsItem*> itemsToRemove;
    for (QGraphicsItem* item : allItems)
    {
        // Identify R marker items by checking if they're QGraphicsTextItem with text "R" and yellow color
        QGraphicsTextItem* textItem = qgraphicsitem_cast<QGraphicsTextItem*>(item);
        if (textItem && textItem->toPlainText() == "R" && textItem->defaultTextColor() == Qt::yellow)
        {
            itemsToRemove.append(item);
        }
    }
    // Remove items after iteration to avoid modifying list while iterating
    for (QGraphicsItem* item : itemsToRemove)
    {
        overlayScene->removeItem(item);
        delete item;
    }

    if (visibleMarkers.empty()) {
        DEBUG_OUT() << "RTW: No visible R markers within time range";
        return;
    }

    // Apply LOD (Level of Detail) for R markers when there are many markers
    // Uses symbol-specific LOD which is less aggressive than data line LOD
    size_t lodStep = calculateSymbolLODStep(visibleMarkers.size());
    
    // Draw yellow "R" markers for each visible marker (with LOD)
    int markersDrawn = 0;
    DEBUG_OUT() << "RTW: Drawing" << visibleMarkers.size() << "manually placed R markers (LOD step:" << lodStep << ")";
    
    for (size_t i = 0; i < visibleMarkers.size(); i += lodStep) {
        const auto& markerData = visibleMarkers[i];
        QDateTime timestamp = markerData.timestamp;
        qreal range = markerData.range;
        
        QPointF screenPos = mapDataToScreen(range, timestamp);
        
        // Check if point is within visible area
        if (drawingArea.contains(screenPos)) {
            // Calculate marker size based on window size - make it larger
            QSize windowSize = this->size();
            qreal markerSize = std::min(0.08 * windowSize.width(), 24.0); // Increased size, cap at 24 pixels
            
            // Create yellow "R" text marker
            QGraphicsTextItem *rMarker = new QGraphicsTextItem("R");
            QFont font = rMarker->font();
            font.setPointSizeF(markerSize);
            font.setBold(true);
            rMarker->setFont(font);
            rMarker->setDefaultTextColor(Qt::yellow);
            
            // Center the marker on the data point
            QRectF textRect = rMarker->boundingRect();
            rMarker->setPos(screenPos.x() - textRect.width()/2, screenPos.y() - textRect.height()/2);
            rMarker->setZValue(1000); // Very high z-value to ensure visibility
            
            // Make marker explicitly accept mouse events for reliable clicking
            rMarker->setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
            rMarker->setAcceptHoverEvents(true);
            
            // CRITICAL FIX: R markers are now in overlayScene (interactive overlay) instead of graphicsScene
            overlayScene->addItem(rMarker);
            markersDrawn++;
        }
    }
    
    DEBUG_OUT() << "RTW: Successfully drew" << markersDrawn << "manually placed yellow R markers";
}

/**
 * @brief Draw RTW-specific scatterplot
 *
 */
void RTWGraph::drawRTWScatterplot()
{
    // By default, create a scatterplot using the parent's scatterplot functionality
    drawScatterplot(QString("RTW-1"), Qt::blue, 4.0, Qt::white);

    DEBUG_OUT() << "RTW scatterplot drawn";
}

/**
 * @brief Add an RTW symbol to the graph
 *
 * @param symbolName Name of the symbol (e.g., "TM", "DP", "LY", "CircleI", etc.)
 * @param timestamp Timestamp when the symbol should be displayed
 * @param range Range value (Y-axis position) where the symbol should be displayed
 */
void RTWGraph::addRTWSymbol(const QString &symbolName, const QDateTime &timestamp, qreal range)
{
    // Store symbol in dataSource (WaterfallData) so it persists with track data
    // This follows the same pattern as R markers - symbols are part of the data source
    // R markers are drawn from dataSource in drawCustomRMarkers(), symbols are drawn from dataSource in drawRTWSymbols()
    if (!dataSource)
    {
        DEBUG_OUT() << "RTW: Cannot add symbol - no data source set";
        return;
    }
    
    dataSource->addRTWSymbol(symbolName, timestamp, range);
    
    DEBUG_OUT() << "RTW: Added symbol" << symbolName << "at timestamp" << timestamp.toString() << "with range" << range << "to data source";
    
    // CRITICAL FIX: Set render state to FULL_REDRAW to ensure symbols are drawn
    // When draw() is called, it checks needsFullClear || RANGE_UPDATE_ONLY to draw symbols
    // If render state is CLEAN, symbols won't be drawn, so we need to set it to FULL_REDRAW
    setRenderState(RenderState::FULL_REDRAW);
    
    // Trigger redraw - same pattern as when data is added via setData()
    // The symbol will be drawn in drawRTWSymbols() which is called from draw()
    draw();
}

/**
 * @brief Convert symbol name string to SymbolType enum
 *
 * @param symbolName The symbol name string
 * @return RTWSymbolDrawing::SymbolType The corresponding SymbolType enum value
 */
RTWSymbolDrawing::SymbolType RTWGraph::symbolNameToType(const QString &symbolName) const
{
    QString name = symbolName.toUpper().trimmed();
    
    // Map common symbol names to SymbolType enum
    if (name == "TM") return RTWSymbolDrawing::SymbolType::TM;
    if (name == "DP") return RTWSymbolDrawing::SymbolType::DP;
    if (name == "LY") return RTWSymbolDrawing::SymbolType::LY;
    if (name == "CIRCLEI" || name == "CIRCLE_I") return RTWSymbolDrawing::SymbolType::CircleI;
    if (name == "TRIANGLE") return RTWSymbolDrawing::SymbolType::Triangle;
    if (name == "RECTR" || name == "RECT_R") return RTWSymbolDrawing::SymbolType::RectR;
    if (name == "ELLIPSEPP" || name == "ELLIPSE_PP") return RTWSymbolDrawing::SymbolType::EllipsePP;
    if (name == "RECTX" || name == "RECT_X") return RTWSymbolDrawing::SymbolType::RectX;
    if (name == "RECTA" || name == "RECT_A") return RTWSymbolDrawing::SymbolType::RectA;
    if (name == "RECTAPURPLE" || name == "RECT_A_PURPLE") return RTWSymbolDrawing::SymbolType::RectAPurple;
    if (name == "RECTK" || name == "RECT_K") return RTWSymbolDrawing::SymbolType::RectK;
    if (name == "CIRCLERYELLOW" || name == "CIRCLE_R_YELLOW") return RTWSymbolDrawing::SymbolType::CircleRYellow;
    if (name == "DOUBLEBARYELLOW" || name == "DOUBLE_BAR_YELLOW") return RTWSymbolDrawing::SymbolType::DoubleBarYellow;
    if (name == "R") return RTWSymbolDrawing::SymbolType::R;
    if (name == "L") return RTWSymbolDrawing::SymbolType::L;
    if (name == "BOT") return RTWSymbolDrawing::SymbolType::BOT;
    if (name == "BOTC") return RTWSymbolDrawing::SymbolType::BOTC;
    if (name == "BOTF") return RTWSymbolDrawing::SymbolType::BOTF;
    if (name == "BOTD") return RTWSymbolDrawing::SymbolType::BOTD;
    if (name == "YELLOWCIRCLE1" || name == "YELLOW_CIRCLE_1" || name == "YC1") return RTWSymbolDrawing::SymbolType::YellowCircle1;
    if (name == "YELLOWCIRCLE2" || name == "YELLOW_CIRCLE_2" || name == "YC2") return RTWSymbolDrawing::SymbolType::YellowCircle2;
    if (name == "YELLOWCIRCLE3" || name == "YELLOW_CIRCLE_3" || name == "YC3") return RTWSymbolDrawing::SymbolType::YellowCircle3;
    if (name == "YELLOWCIRCLE4" || name == "YELLOW_CIRCLE_4" || name == "YC4") return RTWSymbolDrawing::SymbolType::YellowCircle4;
    if (name == "MAX" || name == "MAXSYMBOL" || name == "MAX_SYMBOL") return RTWSymbolDrawing::SymbolType::MaxSymbol;
    if (name == "MIN" || name == "MINSYMBOL" || name == "MIN_SYMBOL") return RTWSymbolDrawing::SymbolType::MinSymbol;
    
    // Default to R if symbol name is not recognized
    DEBUG_OUT() << "RTW: Unknown symbol name:" << symbolName << "- defaulting to R";
    return RTWSymbolDrawing::SymbolType::R;
}

/**
 * @brief Draw all stored RTW symbols on the graph
 *
 */
void RTWGraph::drawRTWSymbols()
{
    // Follow the same pattern as R markers - read symbols from dataSource
    // This ensures symbols persist with track changes and zoom customization
    // OPTIMIZATION: Use overlayScene instead of graphicsScene for symbols (interactive overlays)
    if (!overlayScene || !dataSource)
    {
        return;
    }
    
    // CRITICAL FIX: Always remove old RTW symbol items before drawing new ones
    // This prevents duplicates when time range changes and symbols are redrawn
    // Remove ALL items with z-value 1000, regardless of render state
    // Use two-pass approach to avoid iterator invalidation
    QList<QGraphicsItem*> allItems = overlayScene->items();
    QList<QGraphicsItem*> itemsToRemove;
    
    for (QGraphicsItem* item : allItems)
    {
        QGraphicsPixmapItem* pixmapItem = qgraphicsitem_cast<QGraphicsPixmapItem*>(item);
        if (pixmapItem && pixmapItem->zValue() == 1000)
        {
            // Remove ALL pixmap items with z-value 1000 (RTW symbols)
            // Don't check data(0) validity - some symbols might not have it set correctly
            itemsToRemove.append(pixmapItem);
        }
    }
    
    // Remove items in separate loop to avoid iterator invalidation
    for (QGraphicsItem* item : itemsToRemove)
    {
        overlayScene->removeItem(item);
        delete item;
    }
    
    // Get symbols from dataSource (same pattern as R markers get data from dataSource)
    std::vector<RTWSymbolData> rtwSymbols = dataSource->getRTWSymbols();
    
    DEBUG_OUT() << "RTW: drawRTWSymbols() - dataSource pointer:" << dataSource;
    DEBUG_OUT() << "RTW: drawRTWSymbols() - symbols count from dataSource:" << rtwSymbols.size();
    
    if (rtwSymbols.empty())
    {
        DEBUG_OUT() << "RTW: No symbols in dataSource (dataSource pointer:" << dataSource << ")";
        return;
    }
    
    // Check if time range is valid - if not, use symbol timestamps to set range
    bool timeRangeValid = timeMin.isValid() && timeMax.isValid() && timeMin <= timeMax;
    
    // Filter symbols to only include those within the visible time range (same as R markers)
    // If time range is not valid, draw all symbols (they will set the time range)
    std::vector<RTWSymbolData> visibleSymbols;
    if (timeRangeValid)
    {
        for (const auto& symbolData : rtwSymbols)
        {
            if (symbolData.timestamp >= timeMin && symbolData.timestamp <= timeMax)
            {
                visibleSymbols.push_back(symbolData);
            }
        }
    }
    else
    {
        // No valid time range - include all symbols and update time range from symbols
        DEBUG_OUT() << "RTW: No valid time range, using all symbols and updating time range";
        visibleSymbols = rtwSymbols;
        
        // Update time range from symbols if we have any
        if (!rtwSymbols.empty())
        {
            QDateTime symbolTimeMin = rtwSymbols[0].timestamp;
            QDateTime symbolTimeMax = rtwSymbols[0].timestamp;
            for (const auto& symbolData : rtwSymbols)
            {
                if (symbolData.timestamp < symbolTimeMin) symbolTimeMin = symbolData.timestamp;
                if (symbolData.timestamp > symbolTimeMax) symbolTimeMax = symbolData.timestamp;
            }
            
            // Set time range to include all symbols with some padding
            timeMax = symbolTimeMax.addSecs(60); // Add 1 minute padding
            timeMin = symbolTimeMin.addSecs(-60); // Subtract 1 minute padding
            DEBUG_OUT() << "RTW: Updated time range from symbols:" << timeMin.toString() << "to" << timeMax.toString();
        }
    }
    
    DEBUG_OUT() << "RTW: Time range filtering - Total symbols:" << rtwSymbols.size() 
             << "- Visible symbols:" << visibleSymbols.size()
             << "- Time range:" << timeMin.toString() << "to" << timeMax.toString()
             << "- Time range valid:" << timeRangeValid;
    
    if (visibleSymbols.empty())
    {
        DEBUG_OUT() << "RTW: No visible symbols after filtering";
        return;
    }
    
    // Draw symbols (same approach as R markers)
    int symbolsDrawn = 0;
    DEBUG_OUT() << "RTW: Drawing area:" << drawingArea;
    for (const auto& symbolData : visibleSymbols)
    {
        // Map symbol position to screen coordinates (same as R markers)
        QPointF screenPos = mapDataToScreen(symbolData.range, symbolData.timestamp);
        
        // Debug all symbols to diagnose issues
        DEBUG_OUT() << "RTW: Processing symbol" << symbolsDrawn << "- Name:" << symbolData.symbolName 
                 << "Range:" << symbolData.range << "Time:" << symbolData.timestamp.toString() 
                 << "Screen:" << screenPos << "In area:" << drawingArea.contains(screenPos)
                 << "Drawing area:" << drawingArea;
        
        // Check if point is within visible area (same check as R markers use)
        if (!drawingArea.contains(screenPos))
        {
            DEBUG_OUT() << "RTW: Symbol" << symbolData.symbolName << "outside drawing area, skipping";
            continue;
        }
        
        // Convert symbol name to SymbolType
        RTWSymbolDrawing::SymbolType symbolType = symbolNameToType(symbolData.symbolName);
        
        // Get the pixmap for this symbol type
        const QPixmap& symbolPixmap = symbols.get(symbolType);
        
        // Validate pixmap before using it
        if (symbolPixmap.isNull() || symbolPixmap.width() <= 0 || symbolPixmap.height() <= 0)
        {
            DEBUG_OUT() << "RTW: Invalid pixmap for symbol" << symbolData.symbolName << "type" << static_cast<int>(symbolType) << "- skipping";
            continue;
        }
        
        // Create a graphics pixmap item and add it to the scene
        QGraphicsPixmapItem* pixmapItem = new QGraphicsPixmapItem(symbolPixmap);
        
        // Validate pixmap item was created successfully
        if (!pixmapItem)
        {
            DEBUG_OUT() << "RTW: Failed to create pixmap item for symbol" << symbolData.symbolName << "- skipping";
            continue;
        }
        
        // Center the symbol on the data point
        QRectF pixmapRect = pixmapItem->boundingRect();
        if (pixmapRect.width() <= 0 || pixmapRect.height() <= 0)
        {
            DEBUG_OUT() << "RTW: Invalid pixmap rect for symbol" << symbolData.symbolName << "- skipping";
            delete pixmapItem;
            continue;
        }
        
        pixmapItem->setPos(screenPos.x() - pixmapRect.width() / 2, 
                          screenPos.y() - pixmapRect.height() / 2);
        pixmapItem->setZValue(1000); // High z-value to ensure visibility above other elements
        
        // Store symbol data in the pixmap item for click detection
        // Use data(0) for timestamp (QVariant can store QDateTime)
        pixmapItem->setData(0, symbolData.timestamp);
        // Use data(1) for symbol name
        pixmapItem->setData(1, symbolData.symbolName);
        // Use data(2) for range value
        pixmapItem->setData(2, symbolData.range);
        
        // Make pixmap item clickable (similar to R markers)
        pixmapItem->setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
        pixmapItem->setAcceptHoverEvents(true);
        
        // OPTIMIZATION: Add to overlayScene (interactive overlay) instead of graphicsScene (data rendering)
        overlayScene->addItem(pixmapItem);
        symbolsDrawn++;
    }
    
    if (symbolsDrawn > 0)
    {
        DEBUG_OUT() << "RTW: Drew" << symbolsDrawn << "RTW symbols out of" << rtwSymbols.size() << "total";
    }
}

/* ----------------- Ruler indicators ----------------- */

bool RTWGraph::isRulerActive(int index) const
{
    if (index < 0 || index >= RulerCount)
        return false;
    return m_rulers[index].active;
}

void RTWGraph::setRulerActive(int index, const QDateTime &timestamp, qreal range)
{
    if (index < 0 || index >= RulerCount)
    {
        DEBUG_OUT() << "RTW: setRulerActive - invalid index" << index;
        return;
    }

    m_rulers[index].active = true;
    m_rulers[index].timestamp = timestamp;
    m_rulers[index].range = range;

    setRenderState(RenderState::FULL_REDRAW);
    draw();
}

void RTWGraph::clearRuler(int index)
{
    if (index < 0 || index >= RulerCount)
        return;

    m_rulers[index].active = false;
    if (m_selectedRuler == index)
        m_selectedRuler = -1;

    setRenderState(RenderState::FULL_REDRAW);
    draw();
}

void RTWGraph::clearAllRulers()
{
    for (auto &ruler : m_rulers)
        ruler.active = false;
    m_selectedRuler = -1;

    setRenderState(RenderState::FULL_REDRAW);
    draw();
}

void RTWGraph::setSelectedRuler(int index)
{
    // -1 clears the selection; otherwise only active rulers may be selected.
    if (index >= 0 && index < RulerCount && !m_rulers[index].active)
    {
        DEBUG_OUT() << "RTW: setSelectedRuler - ruler" << index << "is not active, ignoring";
        return;
    }

    m_selectedRuler = (index >= 0 && index < RulerCount) ? index : -1;

    setRenderState(RenderState::FULL_REDRAW);
    draw();
}

RTWSymbolDrawing::SymbolType RTWGraph::rulerSymbolType(int index, bool selected) const
{
    // index is 0-based (0..3)
    if (selected)
    {
        switch (index)
        {
        case 0: return RTWSymbolDrawing::SymbolType::YellowCircle1;
        case 1: return RTWSymbolDrawing::SymbolType::YellowCircle2;
        case 2: return RTWSymbolDrawing::SymbolType::YellowCircle3;
        default: return RTWSymbolDrawing::SymbolType::YellowCircle4;
        }
    }

    switch (index)
    {
    case 0: return RTWSymbolDrawing::SymbolType::WhiteCircle1;
    case 1: return RTWSymbolDrawing::SymbolType::WhiteCircle2;
    case 2: return RTWSymbolDrawing::SymbolType::WhiteCircle3;
    default: return RTWSymbolDrawing::SymbolType::WhiteCircle4;
    }
}

void RTWGraph::removeRulerItems()
{
    if (!overlayScene)
        return;

    QList<QGraphicsItem*> allItems = overlayScene->items();
    QList<QGraphicsItem*> itemsToRemove;
    for (QGraphicsItem* item : allItems)
    {
        QGraphicsPixmapItem* pixmapItem = qgraphicsitem_cast<QGraphicsPixmapItem*>(item);
        if (pixmapItem && pixmapItem->data(1).toString() == QStringLiteral("RULER"))
            itemsToRemove.append(pixmapItem);
    }

    for (QGraphicsItem* item : itemsToRemove)
    {
        overlayScene->removeItem(item);
        delete item;
    }
}

void RTWGraph::drawRulers()
{
    if (!overlayScene)
        return;

    // Always remove old ruler items before redrawing to avoid duplicates
    removeRulerItems();

    for (int i = 0; i < RulerCount; ++i)
    {
        const RulerState &ruler = m_rulers[i];
        if (!ruler.active)
            continue;

        const bool selected = (m_selectedRuler == i);
        RTWSymbolDrawing::SymbolType type = rulerSymbolType(i, selected);

        const QPixmap &rulerPixmap = symbols.get(type);
        if (rulerPixmap.isNull() || rulerPixmap.width() <= 0 || rulerPixmap.height() <= 0)
        {
            DEBUG_OUT() << "RTW: Invalid pixmap for ruler" << i << "- skipping";
            continue;
        }

        QPointF screenPos = mapDataToScreen(ruler.range, ruler.timestamp);
        if (!drawingArea.contains(screenPos))
            continue;

        QGraphicsPixmapItem* item = new QGraphicsPixmapItem(rulerPixmap);
        QRectF pixmapRect = item->boundingRect();
        item->setPos(screenPos.x() - pixmapRect.width() / 2,
                     screenPos.y() - pixmapRect.height() / 2);
        // Above ordinary RTW symbols (z=1000) so rulers stay visible/clickable
        item->setZValue(1001);

        // Tag for click detection
        item->setData(1, QStringLiteral("RULER"));
        item->setData(3, i);

        item->setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
        item->setAcceptHoverEvents(true);

        overlayScene->addItem(item);
    }
}
