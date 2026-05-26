#ifndef BTWGRAPH_H
#define BTWGRAPH_H

#include "waterfallgraph.h"
#include "btwsymboldrawing.h"
#include "waterfalldata.h"  // For BTWSymbolData
#include <QPushButton>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QDateTime>
#include <QMap>
#include <QUuid>
#include <QGraphicsPolygonItem>
#include <vector>
#include <mutex>

// Forward declarations to avoid circular dependency
class BTWInteractiveOverlay;
class InteractiveGraphicsItem;
class ZoomPanel;
class GraphContainer;
struct BTWSyncMarkerData;
struct ShadedRegionSyncData;

// Structure to store shaded region data
struct ShadedRegionData
{
    qreal startX;  // Starting X value (left range boundary)
    qreal endX;    // Ending X value (right range boundary)
    QDateTime startY;  // Starting Y value (timestamp) - currently not used, region spans full height
    
    ShadedRegionData() : startX(0.0), endX(0.0) {}
    ShadedRegionData(qreal xStart, qreal xEnd, const QDateTime &y) : startX(xStart), endX(xEnd), startY(y) {}
};

/**
 * @brief BTW Graph component that inherits from waterfallgraph
 *
 * This component creates scatterplots by default and can be extended
 * for specific BTW (Bit Time Waterfall) functionality.
 * Now includes interactive overlay capabilities.
 */
class BTWGraph : public WaterfallGraph
{
    Q_OBJECT

public:
    explicit BTWGraph(QWidget *parent = nullptr, bool enableGrid = false, int gridDivisions = 10, TimeInterval timeInterval = TimeInterval::FifteenMinutes);
    ~BTWGraph();

    // Interactive overlay access
    BTWInteractiveOverlay* getInteractiveOverlay() const;

    /**
     * @brief Get this graph's visible bearing (X) range from the zoom panel
     *
     * Used for local bearing-rate box display: the value shown in the manual marker
     * box is scaled to this graph's visible range (e.g. 0-360 vs 330-360).
     * @param outMin Receives the left (min) bearing value
     * @param outMax Receives the right (max) bearing value
     */
    void getVisibleBearingRange(qreal &outMin, qreal &outMax) const;

    /**
     * @brief Get all timestamps from automatic markers
     * @return Vector of timestamps from all automatic markers that were created
     */
    std::vector<QDateTime> getAutomaticMarkerTimestamps() const;
    
    /**
     * @brief Add a manual marker programmatically via API call
     * @param timestamp The timestamp for the marker (Y-axis position)
     * @param rangeValue The range value for the marker (X-axis position)
     * @param bearingRate Optional bearing rate (rotation angle / 10.0). Defaults to 0.0
     * @return Pointer to the created marker, or nullptr if creation failed
     */
    InteractiveGraphicsItem* addBTWManualMarker(const QDateTime &timestamp, qreal rangeValue, qreal bearingRate = 0.0);
    
    // ========== Marker Sync Methods ==========
    
    /**
     * @brief Create a marker from sync data (called when syncing from another container)
     * @param markerData The BTW marker data
     * @return true if marker was created successfully
     */
    bool createMarkerFromSyncData(const BTWSyncMarkerData &markerData);
    
    /**
     * @brief Update a marker from sync data
     * @param markerData The updated marker data
     * @return true if marker was found and updated
     */
    bool updateMarkerFromSyncData(const BTWSyncMarkerData &markerData);
    
    /**
     * @brief Delete a marker by its sync ID
     * @param markerId The unique ID of the marker to delete
     * @return true if marker was found and deleted
     */
    bool deleteMarkerBySyncId(const QUuid &markerId);
    
    /**
     * @brief Check if a marker with the given sync ID exists
     * @param markerId The unique ID to check
     * @return true if marker exists
     */
    bool hasMarkerWithSyncId(const QUuid &markerId) const;
    
    // ========== Shaded Region Sync Methods ==========
    
    /**
     * @brief Create a shaded region from sync data (called when syncing from another container)
     * @param regionData The shaded region sync data
     * @return The local region ID, or -1 if creation failed
     */
    int createShadedRegionFromSyncData(const ShadedRegionSyncData &regionData);
    
    /**
     * @brief Delete a shaded region by its sync ID
     * @param syncId The global sync ID of the region to delete
     * @return true if region was found and deleted
     */
    bool deleteShadedRegionBySyncId(const QUuid &syncId);
    
    /**
     * @brief Check if a shaded region with the given sync ID exists
     * @param syncId The global sync ID to check
     * @return true if region exists
     */
    bool hasShadedRegionWithSyncId(const QUuid &syncId) const;
    
    /**
     * @brief Add a BTW symbol to the graph
     * @param symbolName Name of the symbol (e.g., "MagentaCircle", "YellowCircle1", "WhiteCircle2")
     * @param timestamp Timestamp when the symbol should be displayed
     * @param range Range value (Y-axis position) where the symbol should be displayed
     */
    void addBTWSymbol(const QString &symbolName, const QDateTime &timestamp, qreal range);

    /**
     * @brief Add a predefined BTW symbol by type (pixmap cache lookup)
     * @param symbolType Registered BTWSymbolDrawing::SymbolType
     * @param timestamp Timestamp when the symbol should be displayed
     * @param range Bearing/range value (X-axis) where the symbol should be displayed
     */
    void addBTWSymbol(BTWSymbolDrawing::SymbolType symbolType, const QDateTime &timestamp, qreal range);
    
    /**
     * @brief Add a shaded region to the graph
     * The region will be drawn as a vertical band spanning from top to bottom (all timestamps),
     * with horizontal boundaries defined by the X range values
     * @param startX Starting X value (left range boundary, e.g., 30.0)
     * @param endX Ending X value (right range boundary, e.g., 40.0)
     * @param startY Starting Y value (timestamp) - currently stored but region spans full height
     * @return Unique identifier for the shaded region
     */
    int addShadedRegion(qreal startX, qreal endX, const QDateTime &startY);
    
    /**
     * @brief Remove a shaded region by its identifier
     * @param regionId The identifier returned by addShadedRegion
     */
    void removeShadedRegion(int regionId);
    
    /**
     * @brief Clear all shaded regions
     */
    void clearShadedRegions();

    // ========== Horizontal Line Management ==========
    
    /**
     * @brief Enum for horizontal line interaction modes
     */
    enum class HorizontalLineMode {
        Normal,      // Normal marker mode (default)
        DrawLine,    // Draw lines mode: clicking adds lines, clicking on existing line deletes it
        DeleteLine   // Delete mode: clicking only deletes lines, doesn't add new ones
    };
    
    /**
     * @brief Set horizontal line mode
     * @param mode The mode to set (Normal, DrawLine, or DeleteLine)
     */
    void setHorizontalLineMode(HorizontalLineMode mode);
    
    /**
     * @brief Set horizontal line mode (legacy boolean interface for backward compatibility)
     * @param enabled True to enable draw line mode, false for normal mode
     */
    void setHorizontalLineMode(bool enabled);
    
    /**
     * @brief Get current horizontal line mode
     * @return Current mode
     */
    HorizontalLineMode getHorizontalLineMode() const;
    
    /**
     * @brief Check if horizontal line mode is enabled (legacy method)
     * @return True if in DrawLine or DeleteLine mode
     */
    bool isHorizontalLineMode() const;
    
    /**
     * @brief Add a horizontal line at a specific time
     * @param timestamp The time when the line should be drawn (horizontal line = constant time)
     * @param color The color of the line (default: white)
     * @param width The width of the line (default: 2.0)
     * @return Unique identifier for the line
     */
    QUuid addHorizontalLine(const QDateTime &timestamp, const QColor &color = Qt::white, qreal width = 2.0);
    
    /**
     * @brief Get the timestamp of a horizontal line by its ID
     * @param lineId The unique identifier of the line
     * @return The timestamp of the line, or invalid QDateTime if not found
     */
    QDateTime getHorizontalLineTimestamp(const QUuid &lineId) const;
    
    /**
     * @brief Get the timestamp of the first horizontal line (if any).
     * @return The timestamp of the first line, or invalid QDateTime if none
     */
    QDateTime getFirstHorizontalLineTimestamp() const;
    
    /**
     * @brief Get the timestamp of the latest horizontal line (most recently added).
     * Used e.g. to define history selection from real time to BTW line.
     * @return The timestamp of the last line, or invalid QDateTime if none
     */
    QDateTime getLatestHorizontalLineTimestamp() const;
    
    /**
     * @brief Remove a horizontal line by its ID
     * @param lineId The unique identifier of the line to remove
     * @return True if the line was found and removed
     */
    bool removeHorizontalLine(const QUuid &lineId);
    
    /**
     * @brief Remove horizontal lines by timestamp (for syncing)
     * @param timestamp The timestamp to match
     * @param toleranceMs Time tolerance in milliseconds (default: 1ms)
     * @return Number of lines removed
     */
    int removeHorizontalLineByTimestamp(const QDateTime &timestamp, qreal toleranceMs = 0.001);
    
    /**
     * @brief Clear all horizontal lines
     */
    void clearHorizontalLines();

public slots:
    void deleteInteractiveMarkers();

protected:
    // Override the draw method to create scatterplots by default
    void draw() override;
    void refreshOverlaysAfterVisibleTimeRangeChange() override;
    void drawBTWSymbols() override;
    void augmentOverlayPassAfterSymbols() override;

    // Override mouse event handlers to add interactive markers on click
    void onMouseClick(const QPointF &scenePos) override;
    void onMouseDrag(const QPointF &scenePos) override;

    // Override resize event to update overlay
    void resizeEvent(QResizeEvent *event) override;

private slots:
    // Interactive overlay event slots
    void onMarkerAdded(InteractiveGraphicsItem *marker, int type);
    void onMarkerRemoved(InteractiveGraphicsItem *marker, int type);
    void onMarkerMoved(InteractiveGraphicsItem *marker, const QPointF &newPosition);
    void onMarkerRotated(InteractiveGraphicsItem *marker, qreal angle);
    void onMarkerClicked(InteractiveGraphicsItem *marker, const QPointF &position);

private:
    // BTW-specific properties and methods can be added here
    void drawBTWScatterplot();
    void drawCustomCircleMarkers();
    void addBTWSymbolToOtherGraphs(const QDateTime &timestamp, qreal btwValue);
    
    // Interactive overlay setup
    void setupInteractiveOverlay();
    
    // Interactive overlay
    BTWInteractiveOverlay *m_interactiveOverlay;
    
    // BTW symbol drawing utility (symbols are stored in WaterfallData)
    BTWSymbolDrawing symbols;
    
    // Store timestamps from automatic markers
    std::vector<QDateTime> m_automaticMarkerTimestamps;
    
    // Shaded regions storage (key: region ID, value: region data and graphics item)
    struct ShadedRegionItem
    {
        ShadedRegionData data;
        QGraphicsPolygonItem *polygonItem;
        QUuid syncId;  // Global sync identifier for syncing across containers
        
        ShadedRegionItem() : polygonItem(nullptr) {}
        ShadedRegionItem(const ShadedRegionData &d) : data(d), polygonItem(nullptr), syncId(QUuid::createUuid()) {}
    };
    QMap<int, ShadedRegionItem> m_shadedRegions;
    QMap<QUuid, int> m_syncIdToRegionId;  // Reverse lookup: sync ID to local region ID
    int m_nextRegionId;
    
    // Helper method to get zoom panel from parent GraphContainer
    ZoomPanel* getZoomPanel() const;
    
    // Method to draw shaded regions
    void drawShadedRegions();
    
    // Static cached hatch brush for shaded regions (shared across all instances)
    static QBrush getCachedHatchBrush();
    
    // Method to draw horizontal lines (cached)
    void drawHorizontalLines();
    
    // Horizontal line storage structure
    struct HorizontalLineItem
    {
        QDateTime timestamp;  // Time when the line should be drawn (horizontal line = constant time)
        QColor color;  // Line color
        qreal width;   // Line width
        QUuid id;      // Unique identifier
        QGraphicsLineItem *lineItem;  // Cached graphics item
        
        HorizontalLineItem() : color(Qt::white), width(2.0), lineItem(nullptr) {}
        HorizontalLineItem(const QDateTime &ts, const QColor &c, qreal w) 
            : timestamp(ts), color(c), width(w), id(QUuid::createUuid()), lineItem(nullptr) {}
    };
    
    QList<HorizontalLineItem> m_horizontalLines;  // Store horizontal lines
    HorizontalLineMode m_horizontalLineMode;  // Current horizontal line interaction mode
    
    // Window size cache (Issue #3: Performance optimization)
    QSize m_cachedWindowSize;      // Cached window size
    qreal m_cachedMarkerRadius;    // Cached marker radius based on window size
    bool m_windowSizeCacheValid;   // Flag to track cache validity

    /**
     * Snap a manual marker to the visible series whose interpolated trace is horizontally
     * nearest the click at the given time (clicked Y → timestamp).
     */
    bool snapManualMarkerToNearestSeriesAtTime(const QPointF &scenePos, const QDateTime &timestamp,
                                               qreal &outRange, QString &outSeriesLabel) const;
    
    // Cache update function (Issue #3)
    void updateWindowSizeCache();

signals:
    /**
     * @brief Emitted when a manual marker is placed
     * @param timestamp The timestamp of the placed marker
     * @param position The scene position where the marker was placed
     */
    void manualMarkerPlaced(const QDateTime &timestamp, const QPointF &position);
    
    /**
     * @brief Emitted when a manual marker is clicked
     * @param timestamp The timestamp of the clicked marker
     * @param position The scene position where the marker was clicked
     */
    void manualMarkerClicked(const QDateTime &timestamp, const QPointF &position);
    
    /**
     * @brief Emitted when a marker is clicked with full data
     * 
     * This signal provides all marker data for external integration:
     * - timestamp: When the marker is positioned in time
     * - rangeValue: The X-axis range value (horizontal position)
     * - bearingRate: The bearing rate value shown in the box (rotation angle / 10)
     * 
     * @param timestamp The timestamp of the marker
     * @param rangeValue The range value (X-axis position)
     * @param bearingRate The bearing rate value (from the box display)
     */
    void markerClickedWithData(const QDateTime &timestamp, qreal rangeValue, qreal bearingRate);
    
    // ========== Marker Sync Signals ==========
    
    /**
     * @brief Emitted when a marker's data changes and needs to be synced
     * @param markerData The current state of the marker
     */
    void markerSyncDataChanged(const BTWSyncMarkerData &markerData);
    
    /**
     * @brief Emitted when a marker is deleted and needs to be synced
     * @param markerId The unique ID of the deleted marker
     */
    void markerSyncDeleted(const QUuid &markerId);
    
    // ========== Shaded Region Sync Signals ==========
    
    /**
     * @brief Emitted when a shaded region is added and needs to be synced
     * @param regionData The shaded region data to sync
     */
    void shadedRegionAdded(const ShadedRegionSyncData &regionData);
    
    /**
     * @brief Emitted when a shaded region is removed and needs to be synced
     * @param syncId The global sync ID of the removed region
     */
    void shadedRegionRemoved(const QUuid &syncId);
    
    /**
     * @brief Emitted when all shaded regions are cleared
     */
    void shadedRegionsCleared();
    
    // ========== Horizontal Line Signals ==========
    
    /**
     * @brief Emitted when a horizontal line is placed
     * @param lineId The unique identifier of the line
     * @param timestamp The time when the line was placed
     */
    void horizontalLinePlaced(const QUuid &lineId, const QDateTime &timestamp);
    
    /**
     * @brief Emitted when a horizontal line is removed
     * @param lineId The unique identifier of the removed line
     * @param timestamp The timestamp of the removed line
     */
    void horizontalLineRemoved(const QUuid &lineId, const QDateTime &timestamp);
};

#endif // BTWGRAPH_H
