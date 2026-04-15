#ifndef WATERFALLGRAPH_H
#define WATERFALLGRAPH_H

#include "drawutils.h"
#include "timelineutils.h"
#include "waterfalldata.h"
#include "btwsymboldrawing.h"
#include "circularbuffer.h"
#include <cstdint>
#include <QColor>
#include <QCoreApplication>
#include <QFont>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QPixmap>
#include <QMap>
#include <QPair>
#include <QMouseEvent>
#include <QPainterPath>
#include <QPalette>
#include <QPolygonF>
#include <QEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QPaintEvent>
#include <QTime>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QCursor>
#include <map>
#include <set>
#include <vector>
#include <functional>
#include "sharedsyncstate.h"

// Forward declaration
class GraphEngine;

class WaterfallGraph : public QWidget
{
    Q_OBJECT

public:
    explicit WaterfallGraph(QWidget *parent = nullptr, bool enableGrid = false, int gridDivisions = 10, TimeInterval timeInterval = TimeInterval::FifteenMinutes);
    ~WaterfallGraph();

    // Data source management
    void setDataSource(WaterfallData &dataSource);
    WaterfallData *getDataSource() const;
    
    // Engine management (NEW - attach/detach pattern)
    void attachEngine(GraphEngine *engine);
    void detachEngine();
    GraphEngine* getEngine() const { return m_engine; }

    // Time interval configuration
    void setTimeInterval(TimeInterval interval);
    TimeInterval getTimeInterval() const;
    qint64 getTimeIntervalMs() const;

    // Grid configuration
    void setGridEnabled(bool enabled);
    bool isGridEnabled() const;
    void setGridDivisions(int divisions);
    int getGridDivisions() const;

    // Line vs scatterplot drawing configuration
    void setUseLineDrawing(bool useLines);
    bool getUseLineDrawing() const;

    // Data handling (delegates to data source)
    void setData(const QString &seriesLabel, const std::vector<float> &yData, const std::vector<QDateTime> &timestamps);
    void setData(const WaterfallData &data);
    void clearData();

    // Incremental data addition methods (delegates to data source)
    void addDataPoint(const QString &seriesLabel, float yValue, const QDateTime &timestamp);
    void addDataPoints(const QString &seriesLabel, const std::vector<float> &yValues, const std::vector<QDateTime> &timestamps);

    // Data access methods (delegates to data source)
    WaterfallData getData(const QString &seriesLabel) const;
    std::vector<std::pair<qreal, QDateTime>> getDataWithinYExtents(const QString &seriesLabel, qreal yMin, qreal yMax) const;
    std::vector<std::pair<qreal, QDateTime>> getDataWithinTimeRange(const QString &seriesLabel, const QDateTime &startTime, const QDateTime &endTime) const;

    // Direct access to data vectors (delegates to data source)
    const std::vector<qreal> &getYData(const QString &seriesLabel) const;
    const std::vector<QDateTime> &getTimestamps(const QString &seriesLabel) const;

    // Mouse event handlers (virtual so they can be overridden in derived classes)
    virtual void onMouseClick(const QPointF &scenePos);
    virtual void onMouseDrag(const QPointF &scenePos);

    // Auto-update Y range methods
    void setAutoUpdateYRange(bool enabled);
    bool getAutoUpdateYRange() const;

    // Zero axis value (for BDW, BRW, FDW graphs)
    void setZeroAxisValue(qreal value);
    qreal getZeroAxisValue() const;
    
    // Application start time - timestamps before this should not be displayed
    void setApplicationStartTime(const QDateTime& time);
    QDateTime getApplicationStartTime() const;

    // Interactive update API for real-time updates during drag operations
    /**
     * @brief Update data with fast incremental rendering (for interactive drag operations).
     * 
     * This method updates data and triggers fast incremental rendering without:
     * - Range recalculation
     * - Cache clearing
     * - Full scene clear
     * 
     * Use this for high-frequency updates during drag operations.
     * Call normal setData() or ensure full redraw after drag ends.
     * 
     * @param seriesLabel The series to update
     * @param yData Y-axis values
     * @param timestamps Corresponding timestamps
     */
    void setDataInteractive(const QString &seriesLabel, const std::vector<float> &yData, const std::vector<QDateTime> &timestamps);
    
    /**
     * @brief Trigger incremental redraw for a series (data already updated in dataSource).
     * 
     * Use this when data has already been updated in the dataSource (e.g., via GraphEngine)
     * and you just need to trigger an incremental redraw.
     * 
     * @param seriesLabel The series to redraw
     */
    void triggerIncrementalRedraw(const QString &seriesLabel);

    // Fast track switching API - marks track change for visible-window-first rendering
    /**
     * @brief Mark that a track change has occurred.
     * 
     * This method enables fast track switching by:
     * - Clearing all caches to prevent stale data
     * - Triggering visible-window-first rendering (only builds geometry for current visible window)
     * - Ensuring immediate visual feedback without blocking on full historical rebuild
     * 
     * The track change mode is automatically reset after rendering completes,
     * and normal operation resumes.
     * 
     * Call this when switching between tracks in a multi-track system (e.g., 64-track system).
     */
    void markTrackChanged();

protected:
    // Override mouse events
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    // Override mouse move to track cursor for crosshair
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

    // Override resize event
    void resizeEvent(QResizeEvent *event) override;

    // Override show event
    void showEvent(QShowEvent *event) override;

    // Override paint event for direct rendering (replaces QGraphicsScene)
    void paintEvent(QPaintEvent *event) override;

    // Auto-update Y range flag
    bool autoUpdateYRange;

protected:
    QGraphicsView *graphicsView;
    QGraphicsScene *graphicsScene;

    // Overlay scene for interactive elements
    QGraphicsView *overlayView;
    QGraphicsScene *overlayScene;

    // Cursor layer for dedicated cursor rendering
    QGraphicsView *cursorView;
    QGraphicsScene *cursorScene;
    QTimer *cursorUpdateTimer;
    QGraphicsLineItem *cursorCrosshairHorizontal;
    QGraphicsLineItem *cursorCrosshairVertical;
    QGraphicsLineItem *cursorTimeAxisLine;
    GraphContainerSyncState *m_cursorSyncState;
    QPointF m_lastMousePos;
    bool m_cursorLayerEnabled;

    // Drawing area and grid
    QRectF drawingArea;
    bool gridEnabled;
    int gridDivisions;
    void setupDrawingArea();
    virtual void drawGrid();
    void updateGraphicsDimensions();

    // Data plotting methods
    virtual void drawDataLine(const QString &seriesLabel, bool plotPoints = true);
    void buildBatchedLinePaths(const QString &seriesLabel,
                               const CircularBuffer<std::pair<float, qint64>> &visibleData,
                               size_t lodStep,
                               const QColor &seriesColor);
    virtual void drawAllDataSeries();
    virtual void drawDataSeries(const QString &seriesLabel);
    void drawIncremental();
    void scheduleRedraw();
    void onScheduledRedraw();
    void drawBTWSymbols();
    
    // Cached BTW symbol drawing (magenta circles)
    BTWSymbolDrawing m_btwSymbols;

    // State machine for rendering
    enum class RenderState {
        CLEAN,
        RANGE_UPDATE_ONLY,
        INCREMENTAL_UPDATE,
        FULL_REDRAW
    };
    void setRenderState(RenderState newState);
    void markSeriesDirty(const QString &seriesLabel);
    void markAllSeriesDirty();
    void markRangeUpdateNeeded();
    void transitionToAppropriateState();
    void updateDataRanges();
    void updateYRange();
    void updateYRangeFromData();
    void updateYRangeFromCustom();
    void forceRangeUpdate();

    // Visible data cache management (Plan 2: Incremental Rendering)
    void invalidateVisibleDataCache(const QString &seriesLabel);
    void invalidateAllVisibleDataCache();
    void resetViewState(); // Reset view state when engine changes
    void updateVisibleDataCacheIncremental(const QString &seriesLabel);
    void updateVisibleDataCacheFull(const QString &seriesLabel);
    bool isVisibleDataCacheValid(const QString &seriesLabel) const;
    size_t findFirstVisibleIndex(const std::vector<QDateTime> &timestamps, const QDateTime &timeMin) const;
    size_t findLastVisibleIndex(const std::vector<QDateTime> &timestamps, const QDateTime &timeMax) const;

    // Incremental graphics item management (State Machine Based)
    // Use epoch milliseconds to avoid QDateTime timezone conversion in hot path
    void updateScatterplotItemsIncremental(const QString &seriesLabel, 
                                           const CircularBuffer<std::pair<float, qint64>> &newVisibleData,
                                           const QColor &pointColor, qreal pointSize);
    void updateScatterplotItemsFull(const QString &seriesLabel,
                                    const CircularBuffer<std::pair<float, qint64>> &visibleData,
                                    const QColor &pointColor, qreal pointSize);
    void removeScatterplotItemsOutsideRange(const QString &seriesLabel, 
                                            const QDateTime &oldTimeMin, const QDateTime &newTimeMin);
    void updateScatterplotItemPositions(const QString &seriesLabel,
                                        const CircularBuffer<std::pair<float, qint64>> &visibleData,
                                        qreal pointSize);
    void cleanupScatterplotItems(const QString &seriesLabel);
    void cleanupAllScatterplotItems();
    void cleanupSeriesItems(const QString &seriesLabel); // Cleanup ellipse and path items for a series

    // Data range tracking
    qreal yMin, yMax;
    QDateTime timeMin, timeMax;
    bool dataRangesValid;

    // Zero axis value (used for BDW, BRW, FDW graphs)
    qreal m_zeroAxisValue;

    // Range limiting properties
    bool rangeLimitingEnabled;
    qreal customYMin, customYMax;

    // Time range management
    bool customTimeRangeEnabled;
    QDateTime customTimeMin, customTimeMax;
    
    // Application start time - timestamps before this should not be displayed
    QDateTime m_applicationStartTime;

    // Time interval configuration
    TimeInterval timeInterval;

    // Data source reference
    GraphEngine *m_engine; // Reference to engine (not owned)
    WaterfallData *dataSource; // Points to m_engine->dataMutable() when attached

    // Multi-series support
    std::map<QString, QColor> seriesColors;
    std::map<QString, bool> seriesVisibility;
    
    // Cached point pixmaps by (color, size) for efficient rendering
    // Key format: "color_r_g_b_a_size" (e.g., "255_0_255_255_3.0")
    QMap<QString, QPixmap> pointPixmapCache;
    QString getPointPixmapKey(const QColor &color, qreal size) const;
    QPixmap getPointPixmap(const QColor &color, qreal size);

    // Incremental rendering support
    RenderState m_renderState;
    bool m_rangeUpdateNeeded;
    bool m_redrawPending;
    QDateTime m_renderedTimeMin;
    QDateTime m_renderedTimeMax;
    std::set<QString> m_dirtySeries;
    bool m_fastTrackSwitchMode;  // Flag for visible-window-first rendering on track change (auto-reset after render)
    std::map<QString, QGraphicsPathItem*> m_seriesPathItems;
    std::map<QString, std::vector<QGraphicsEllipseItem*>> m_seriesPointItems;
    // Track scatterplot pixmap items per series for incremental updates
    std::map<QString, std::vector<QGraphicsPixmapItem*>> m_seriesScatterplotItems;
    
    // Direct rendering support (replaces QGraphicsScene for data rendering)
    QPixmap m_waterfallBuffer;  // Scroll buffer for waterfall rendering
    int m_waterfallBufferHeight;  // Current buffer height (matches widget height)
    QDateTime m_lastWaterfallRowTime;  // Timestamp of last row drawn (for incremental updates)
    QMap<QString, CircularBuffer<QPointF>> m_scatterPoints;  // Batched scatter points per series (circular buffer)
    QMap<QString, QColor> m_scatterColors;  // Colors per series for scatter points
    QMap<QString, QPainterPath> m_dataLinePaths;  // Store paths for data lines (ADOPTED, etc.) - single path for small datasets
    QMap<QString, CircularBuffer<QPainterPath>> m_batchedLinePaths;  // Batched paths for large datasets (>1000 points) (circular buffer)
    QMap<QString, QColor> m_dataLineColors;  // Colors per series for data lines
    bool m_needsWaterfallRedraw;  // Flag for full waterfall redraw
    bool m_useLineDrawing;  // Flag to use line drawing instead of scatterplot for all series
    
    // Waterfall buffer management methods
    void initializeWaterfallBuffer(const QSize &size);
    void scrollWaterfallBuffer(int pixels);
    void updateWaterfallBufferRow();  // Draw new row at bottom after scrolling

    // Visible data cache for incremental filtering (Plan 2: Incremental Rendering)
    // Avoids O(n) filtering on every draw by caching already-filtered data
    // Uses epoch milliseconds to avoid QDateTime timezone conversion in hot path
    // Uses float instead of qreal to eliminate float-to-double conversion overhead
    std::map<QString, CircularBuffer<std::pair<float, qint64>>> m_cachedVisibleData; // epoch ms, float Y values (circular buffer)
    std::map<QString, std::pair<QDateTime, QDateTime>> m_cachedTimeRange;
    std::map<QString, std::pair<qint64, qint64>> m_cachedTimeRangeEpoch; // epoch ms range used to build visible cache
    std::map<QString, size_t> m_lastProcessedIndex;
    std::map<QString, size_t> m_cachedDataSize;
    
    // Phase 1: Version-based cache validation (mutable for const method caching)
    mutable std::map<QString, uint64_t> m_cachedDataVersion;  // Track data version per series
    mutable std::map<QString, bool> m_cacheValidResult;      // Cache validation result per series

    // Coordinate mapping caches (Issue #1: Performance optimization)
    mutable qint64 m_cachedTimeIntervalMs;  // Cached time interval in milliseconds
    mutable qreal m_cachedYRange;           // Cached (yMax - yMin)
    mutable qreal m_cachedYRangeReciprocal; // Cached 1.0 / (yMax - yMin)
    mutable qreal m_cachedTimeIntervalMsReciprocal; // Cached 1.0 / timeIntervalMs
    mutable qint64 m_cachedTimeMaxEpoch;     // Cached timeMax.toMSecsSinceEpoch() to avoid msecsTo() overhead
    mutable qint64 m_cachedTimeMinEpoch;     // Cached timeMin.toMSecsSinceEpoch() to avoid repeated conversions
    mutable bool m_cachesValid;             // Flag to track cache validity

    // mapScreenToTime() result cache (Performance optimization)
    mutable qreal m_mapScreenToTimeCachedYPos;        // Last cached Y position for mapScreenToTime
    mutable QDateTime m_mapScreenToTimeCachedTime;     // Last cached time result for mapScreenToTime
    mutable bool m_mapScreenToTimeCacheValid; // Flag to track cache validity

    // mapDataToScreen() result cache (Performance optimization - Fix #1)
    // Cache mapping (yValue, timestamp) -> QPointF to avoid recalculating same points
    struct MapDataToScreenCacheKey {
        qreal yValue;
        qint64 timestampEpoch;  // Use epoch for faster comparison
        
        bool operator<(const MapDataToScreenCacheKey& other) const {
            if (timestampEpoch != other.timestampEpoch)
                return timestampEpoch < other.timestampEpoch;
            return yValue < other.yValue;
        }
    };
    mutable std::map<MapDataToScreenCacheKey, QPointF> m_mapDataToScreenCache;
    mutable qint64 m_mapDataToScreenCacheVersion;  // Increment when cache should be invalidated
    static constexpr size_t MAX_MAP_DATA_TO_SCREEN_CACHE_SIZE = 10000;  // Limit cache size
    static constexpr size_t BATCH_THRESHOLD = 1000;  // Threshold for batching paths (points after LOD)
    static constexpr size_t BATCH_SIZE = 100;  // Number of points per batched path
    
    // getSeriesColor() result cache (Performance optimization - Fix #3)
    mutable std::map<QString, QColor> m_seriesColorCache;  // Cache computed default colors

    // Mouse tracking
    bool isDragging;
    QPointF lastMousePos;
    
    // Drawing guard to prevent concurrent draws
    bool isDrawing;

    // Crosshair functionality
    void setupCrosshair();
    void updateCrosshair(const QPointF &mousePos);
    void showCrosshair();
    void hideCrosshair();
    QGraphicsLineItem *crosshairHorizontal;
    QGraphicsLineItem *crosshairVertical;
    bool crosshairEnabled;

    // Cursor callback helpers
    void notifyCursorTimeChanged(const QDateTime &time, qreal yPosition = -1.0);
    std::function<void(const QDateTime &, qreal)> cursorTimeChangedCallback;
    QDateTime lastNotifiedCursorTime;
    qreal lastNotifiedYPosition;
    
    // Crosshair position callback
    std::function<void(qreal xPosition)> crosshairPositionChangedCallback;
    void notifyCrosshairPositionChanged(qreal xPosition);
    qreal lastNotifiedCrosshairXPosition;

    // Time axis cursor functionality
    QGraphicsLineItem *timeAxisCursor;
    qreal mapTimeToY(const QDateTime &time) const;
    
    // Coordinate mapping cache update (Issue #1)
    void updateCoordinateMappingCaches() const;
    
    // Level of Detail (LOD) helper for high intervals (Issue #3)
    size_t calculateLODStep(size_t dataSize) const;
    size_t calculateSymbolLODStep(size_t symbolCount) const;  // Different LOD for symbols (less aggressive)
    
    // Helper methods to reduce code duplication
    void ensureVisibleDataCacheValid(const QString &seriesLabel);  // Ensures cache is valid, updates if needed
    /** Phase 4: read visible cache directly (no copy to std::vector). */
    const CircularBuffer<std::pair<float, qint64>>& cachedVisibleBuffer(const QString &seriesLabel) const;
    bool isValidScreenPoint(const QPointF& point) const;  // Validates screen point (not null and finite)
    
    // Reusable vectors/arrays to avoid repeated allocations
    // Note: These are mutable so they can be modified in const member functions (for caching/optimization)
    mutable std::vector<QString> m_reusableSeriesLabels;  // Reusable vector for series labels
    QList<QGraphicsItem*> m_reusableItemList;  // Reusable list for graphics items
    QList<QGraphicsItem*> m_reusableItemsToRemove;  // Reusable list for items to remove
    std::vector<BTWSymbolData> m_reusableVisibleSymbols;  // Reusable vector for visible BTW symbols
    QVector<QPainterPath> m_reusableBatchedPaths;  // Reusable vector for batched paths (temporary work vector)
    std::vector<std::pair<float, qint64>> m_reusableVisibleData;  // Reusable vector for visible data pairs (epoch ms, float Y values)
    std::vector<QPointF> m_reusablePointFVector;  // Reusable vector for QPointF conversions
    std::vector<QPainterPath> m_reusablePainterPathVector;  // Reusable vector for QPainterPath conversions
    mutable std::vector<qreal> m_reusableYData;  // Reusable vector for Y data series (avoids toVector() allocations)
    mutable std::vector<float> m_reusableYDataFloat;  // Reusable vector for Y data series (float, no conversion overhead)
    mutable std::vector<QDateTime> m_reusableTimestamps;  // Reusable vector for QDateTime timestamps (avoids toVector() allocations)
    mutable std::vector<qint64> m_reusableTimestampsEpoch;  // Reusable vector for epoch timestamps (avoids toVector() allocations)
    
    // Crosshair update caches (Issue #2)
    QRectF m_cachedCursorSceneRect;        // Cached cursor scene rectangle
    QRectF m_cachedOverlaySceneRect;       // Cached overlay scene rectangle
    QDateTime m_lastCachedTime;            // Last cached time for mapTimeToY
    qreal m_lastCachedYPos;                // Last cached Y position
    bool m_cursorSceneRectValid;           // Flag for cursor scene rect cache
    bool m_overlaySceneRectValid;           // Flag for overlay scene rect cache
    
    // Crosshair cache update functions (Issue #2)
    void updateCursorSceneRectCache();
    void updateOverlaySceneRectCache();

    // Mouse selection functionality
    bool mouseSelectionEnabled;
    QPointF selectionStartPos;
    QPointF selectionEndPos;
    QGraphicsRectItem *selectionRect;

    // Selection methods
    void startSelection(const QPointF &scenePos);
    void updateSelection(const QPointF &scenePos);
    void endSelection();
    void clearSelection();
    QDateTime mapScreenToTime(qreal yPos) const;

private slots:
    // Cursor layer update method
    void updateCursorLayer();

public:
    // Coordinate mapping methods (public for overlay sync)
    qreal mapScreenXToRange(qreal xPos) const; // Convert screen X position to range value
    QPointF mapDataToScreen(qreal yValue, const QDateTime &timestamp) const; // Convert data to screen coordinates
    QPointF mapDataToScreen(qreal yValue, qint64 timestampEpochMs) const; // Overload with epoch milliseconds (no timezone conversion!)
    
    // Mouse selection control
    void setMouseSelectionEnabled(bool enabled);
    bool isMouseSelectionEnabled() const;

    // Test method to manually create a selection rectangle
    void testSelectionRectangle();
    
    // Crosshair control
    void setCrosshairEnabled(bool enabled);
    bool isCrosshairEnabled() const;
    
    // Time axis cursor control
    void setTimeAxisCursor(const QDateTime &time);
    void clearTimeAxisCursor();
    void setCursorTimeChangedCallback(const std::function<void(const QDateTime &, qreal)> &callback);
    
    // Crosshair position callback
    void setCrosshairPositionChangedCallback(const std::function<void(qreal xPosition)> &callback);
    
    // Cursor layer control
    void setCursorSyncState(GraphContainerSyncState *syncState);
    void setCursorLayerEnabled(bool enabled);
    bool isCursorLayerEnabled() const;
    
    // Public access to overlay scene for interactive elements
    QGraphicsScene* getOverlayScene() const { return overlayScene; }

    // Range limiting methods
    void setRangeLimitingEnabled(bool enabled);
    bool isRangeLimitingEnabled() const;
    void setCustomYRange(const qreal yMin, const qreal yMax);
    std::pair<qreal,qreal> getCustomYRange() const;
    void unsetCustomYRange();

    // Time range update method
    void updateTimeRange();

    // Time range management methods
    void setTimeRange(const QDateTime &timeMin, const QDateTime &timeMax);
    void setTimeMax(const QDateTime &timeMax);
    void setTimeMin(const QDateTime &timeMin);
    QDateTime getTimeMax() const;
    QDateTime getTimeMin() const;
    std::pair<QDateTime, QDateTime> getTimeRange() const;
    void setTimeRangeFromData();
    void setTimeRangeFromDataWithInterval(qint64 intervalMs);
    
    // Helper to check if time range is valid and reasonable for drawing
    bool isTimeRangeValidForDrawing() const;
    void unsetCustomTimeRange();

    // Public draw method for external redraw triggers
    virtual void draw();
    
    // Force a full redraw (clears and recreates all graphics items)
    // Use this when data changes significantly or after initial setup
    void forceFullRedraw();

    // Drawing methods for custom elements
    void drawPoint(const QPointF &position, const QColor &color = Qt::white, qreal size = 2.0);
    void drawAxisLine(const QPointF &startPos, const QPointF &endPos, const QColor &color = QColor(255, 255, 255, 128));
    void drawCharacterLabel(const QString &text, const QPointF &position, const QColor &color = Qt::white, int fontSize = 12);
    void drawTriangleMarker(const QPointF &position, const QColor &fillColor = Qt::red, const QColor &outlineColor = Qt::black, qreal size = 8.0);
    void drawScatterplot(const QString &seriesLabel, const QColor &pointColor = Qt::white, qreal pointSize = 4.0, const QColor &outlineColor = Qt::black);

    // Multi-series support methods
    void setSeriesColor(const QString &seriesLabel, const QColor &color);
    QColor getSeriesColor(const QString &seriesLabel) const;
    void setSeriesVisible(const QString &seriesLabel, bool visible);
    bool isSeriesVisible(const QString &seriesLabel) const;
    std::vector<QString> getVisibleSeries() const;

    // Capacity management methods - reserve space for rendering caches to reduce reallocations
    /**
     * @brief Reserve capacity for scatter points vector for a specific series
     * @param seriesLabel The series label to reserve capacity for
     * @param capacity Number of elements to reserve
     */
    void reserveScatterPointsCapacity(const QString &seriesLabel, size_t capacity);
    
    /**
     * @brief Reserve capacity for all scatter points vectors
     * @param capacity Number of elements to reserve for each series
     */
    void reserveAllScatterPointsCapacity(size_t capacity);
    
    /**
     * @brief Reserve capacity for batched line paths vector for a specific series
     * @param seriesLabel The series label to reserve capacity for
     * @param capacity Number of elements to reserve
     */
    void reserveBatchedLinePathsCapacity(const QString &seriesLabel, size_t capacity);
    
    /**
     * @brief Reserve capacity for cached visible data vector for a specific series
     * @param seriesLabel The series label to reserve capacity for
     * @param capacity Number of elements to reserve
     */
    void reserveCachedVisibleDataCapacity(const QString &seriesLabel, size_t capacity);
    
    /**
     * @brief Reserve capacity for all cached visible data vectors
     * @param capacity Number of elements to reserve for each series
     */
    void reserveAllCachedVisibleDataCapacity(size_t capacity);
    
    /**
     * @brief Reserve capacity for all rendering caches (scatter points, line paths, cached data)
     * @param scatterCapacity Capacity for scatter points
     * @param linePathsCapacity Capacity for batched line paths
     * @param cachedDataCapacity Capacity for cached visible data
     */
    void reserveAllRenderingCachesCapacity(size_t scatterCapacity, size_t linePathsCapacity, size_t cachedDataCapacity);

signals:
    void SelectionCreated(const TimeSelectionSpan &selection);
    /**
     * @brief Emitted when a marker timestamp and value change (new marker placed or marker clicked)
     * @param timestamp The timestamp of the marker
     * @param value The value (range) of the marker
     */
    void markerTimestampValueChanged(const QDateTime &timestamp, qreal value);
};

#endif // WATERFALLGRAPH_H
