#ifndef GRAPHLAYOUT_H
#define GRAPHLAYOUT_H

#include "graphcontainer.h"
#include "graphtype.h"
#include "waterfalldata.h"
#include "graphengine.h"
#include <QDateTime>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QColor>
#include <QUuid>
#include <QElapsedTimer>
#include <map>
#include <vector>
#include "sharedsyncstate.h"
#include "sharedcachestore.h"
#include "scopebus.h"

// Forward declaration
class BTWGraph;

enum class LayoutType
{
    GPW1W = 0,  // 1 window only
    GPW4W = 1,  // 4 windows in 2x2 grid
    GPW2WV = 2, // 2 windows in vertical line
    GPW2WH = 3, // 2 windows in horizontal line
    GPW4WH = 4, // 4 windows in horizontal line
    NOGPW2WH = 5, // 2 windows in horizontal line, but take up whole screen
    HIDDEN = 6  // Hidden
};

class GraphLayout : public QWidget
{
    Q_OBJECT
public:
    explicit GraphLayout(QWidget *parent, LayoutType layoutType, QTimer *timer = nullptr, std::map<GraphType, std::vector<QPair<QString, QColor>>> seriesLabelsMap = std::map<GraphType, std::vector<QPair<QString, QColor>>>(), const QDateTime &systemStartTimeAtInit = QDateTime());
    ~GraphLayout();

    void setLayoutType(LayoutType layoutType);
    LayoutType getLayoutType() const;

    // Sizing methods
    void setGraphViewSize(int width, int height);
    void updateLayoutSizing();

    // Data options management - operate on specific container by label
    void addDataOption(const QString &containerLabel, const GraphType &graphType, WaterfallData &dataSource);
    void removeDataOption(const QString &containerLabel, const GraphType &graphType);
    void clearDataOptions(const QString &containerLabel);
    void setCurrentDataOption(const QString &containerLabel, const GraphType &graphType);
    GraphType getCurrentDataOption(const QString &containerLabel) const;
    std::vector<GraphType> getAvailableDataOptions(const QString &containerLabel) const;
    WaterfallData *getDataOption(const QString &containerLabel, const GraphType &graphType);
    bool hasDataOption(const QString &containerLabel, const GraphType &graphType) const;

    // Data options management - operate on specific container by index (0-3)
    // Container indices for GPW4W (2x2) layout:
    // 0 = top-left, 1 = top-right, 2 = bottom-left, 3 = bottom-right
    void setContainerGraphType(int containerIndex, const GraphType &graphType);
    GraphType getContainerGraphType(int containerIndex) const;

    // Data options management - operate on all visible containers
    void addDataOption(const GraphType &graphType, WaterfallData &dataSource);
    void removeDataOption(const GraphType &graphType);
    void clearDataOptions();
    void setCurrentDataOption(const GraphType &graphType);

    // Data point methods for specific data sources
    void addDataPointToDataSource(const GraphType &graphType, const QString &seriesLabel, float yValue, const QDateTime &timestamp);
    void addDataPointsToDataSource(const GraphType &graphType, const QString &seriesLabel, const std::vector<float> &yValues, const std::vector<QDateTime> &timestamps);
    void setDataToDataSource(const GraphType &graphType, const QString &seriesLabel, const std::vector<float> &yData, const std::vector<QDateTime> &timestamps);
    void setDataToDataSource(const GraphType &graphType, const QString &seriesLabel, const WaterfallData &data);
    void clearDataSource(const GraphType &graphType, const QString &seriesLabel);
    
    // Interactive drag API for real-time updates during ruler dragging
    // Use this API when drag is active for fast incremental updates (no range recalculation)
    void setDataToDataSourceInteractive(const GraphType &graphType, const QString &seriesLabel, 
                                        const std::vector<float> &yData, const std::vector<QDateTime> &timestamps);
    
    // Call this when drag ends to trigger full redraw with range recalculation
    void endInteractiveDrag(const GraphType &graphType);
    
    // Flush any pending interactive updates (call when drag ends)
    void flushPendingInteractiveUpdates(const GraphType &graphType);

    // Data source management
    WaterfallData *getDataSource(const GraphType &graphType);
    bool hasDataSource(const GraphType &graphType) const;
    std::vector<GraphType> getDataSourceLabels() const;
    
    // Engine management (for views to attach/detach)
    GraphEngine* getEngine(const GraphType &graphType);
    
    // Series-specific data source management
    bool hasSeriesInDataSource(const GraphType &graphType, const QString &seriesLabel) const;
    std::vector<QString> getSeriesLabelsInDataSource(const GraphType &graphType) const;
    void addSeriesToDataSource(const GraphType &graphType, const QString &seriesLabel);
    void removeSeriesFromDataSource(const GraphType &graphType, const QString &seriesLabel);

    // Container management
    std::vector<QString> getContainerLabels() const;
    bool hasContainer(const GraphType &graphType) const;

    /**
     * @brief Get the names of the graphs currently shown on screen.
     *
     * Returns the graph-type name (e.g. "BTW", "BDW") of each container that is
     * currently visible in the active layout, in container order. Hidden
     * containers (for layouts that show fewer than 4 graphs) are excluded.
     *
     * @return Names of the currently displayed graphs.
     */
    std::vector<QString> getVisibleGraphNames() const;

    /**
     * @brief Highlight a time region in the history selection bar on all containers.
     *
     * Pass two timestamps to draw a highlighted band between them in each
     * container's history selection visualizer (the narrow bar beside the timeline).
     * Timestamps may be passed in any order; they are normalized internally.
     *
     * Uses the existing addTimeSelection() path: overlapping selections are merged,
     * spans are clamped to the valid data range, and each container keeps at most
     * MAX_TIME_SELECTIONS (5). If a container already has 5 selections, the new
     * region is silently ignored for that container.
     *
     * @param startTime One boundary of the region.
     * @param endTime   The other boundary of the region.
     * @return true if at least one container accepted the selection; false if both
     *         timestamps are invalid or every container was already at the limit.
     */
    bool highlightHistorySelectionRegion(const QDateTime &startTime, const QDateTime &endTime);

    // Set the current time
    void setCurrentTime(const QTime &time);
    void deleteInteractiveMarkers();

    // Selection linking methods
    void linkHorizontalContainers();
    
    // Timeline view syncing methods
    void syncAllTimelineViews();
    
    // Sync an external timeline view with all timeline views in this layout
    void syncExternalTimelineView(TimelineView *externalTimelineView);
    
    // Get sync state pointer for external synchronization
    GraphContainerSyncState* getSyncState() { return &m_syncState; }

    /**
     * @brief Single source of truth for time-scope (visible window) propagation.
     *
     * Containers, timelines, and external integrators all publish/subscribe
     * through this bus. The layout itself installs the only writer of
     * GraphContainerSyncState::currentTimeScope.
     */
    TimeScopeBus* getScopeBus() { return &m_scopeBus; }

    /**
     * @brief Session / system start time for timeline slider mapping (range from this time to effective timeline end).
     * @see GraphLayout(QWidget*, LayoutType, QTimer*, seriesLabelsMap, systemStartTimeAtInit) to set at construction.
     */
    void setSystemStartTime(const QDateTime &t);
    QDateTime systemStartTime() const;
    /** Clears shared system start; mapping falls back to wall-clock now as in-range anchor. */
    void clearSystemStartTime();

    /** Timeline "now" edge; use for replay / paused playback. Pass invalid to clear. */
    void setTimelineEndOverride(const QDateTime &t);
    void clearTimelineEndOverride();

    // Chevron label control methods - operate on all visible containers
    void setChevronLabel1(const QString &label);
    void setChevronLabel2(const QString &label);
    void setChevronLabel3(const QString &label);
    QString getChevronLabel1() const;
    QString getChevronLabel2() const;
    QString getChevronLabel3() const;

    // Chevron label control methods - operate on specific container by label
    void setChevronLabel1(const QString &containerLabel, const QString &label);
    void setChevronLabel2(const QString &containerLabel, const QString &label);
    void setChevronLabel3(const QString &containerLabel, const QString &label);
    QString getChevronLabel1(const QString &containerLabel) const;
    QString getChevronLabel2(const QString &containerLabel) const;
    QString getChevronLabel3(const QString &containerLabel) const;

    // Manoeuvre management methods
    void addManoeuvre(const Manoeuvre &manoeuvre);
    void setManoeuvres(const std::vector<Manoeuvre> &manoeuvres);
    void clearManoeuvres();
    std::vector<Manoeuvre> getManoeuvres() const;

    /**
     * @brief Starts drawing a manoeuvre
     *
     * Begins a new manoeuvre drawing session with the specified start time and parameters.
     * The manoeuvre will be completed when endManoeuvreDrawing() is called.
     *
     * @param startTime The start time of the manoeuvre
     * @param bearing The bearing in degrees (0-359)
     * @param speed The speed value
     * @param depth The depth value
     */
    void startManoeuvreDrawing(const QDateTime &startTime, int bearing, int speed, int depth);

    /**
     * @brief Ends drawing a manoeuvre
     *
     * Completes the current manoeuvre drawing session with the specified end time.
     * The manoeuvre will be added to the graph layout.
     *
     * @param endTime The end time of the manoeuvre
     */
    void endManoeuvreDrawing(const QDateTime &endTime);

    // Set range limits methods
    void setHardRangeLimits(const GraphType graphType, qreal yMin, qreal yMax);
    void removeHardRangeLimits(const GraphType graphType);
    void clearAllHardRangeLimits();
    bool hasHardRangeLimits(const GraphType graphType) const;
    std::pair<qreal, qreal> getHardRangeLimits(const GraphType graphType) const;

    // Clear all graphs - clears all data, markers, and symbols from all graphs
    void clearAllGraphs();
    
    // Clear a specific graph type - forces full clear and redraw (useful when empty data is passed)
    // This ensures graphs are properly cleared when data becomes empty
    void clearGraph(const GraphType &graphType);

    // Marker and symbol management methods - operate on specific graph type
    void addRTWSymbol(const GraphType &graphType, const QString &symbolName, const QDateTime &timestamp, float range);
    void addBTWSymbol(const GraphType &graphType, const QString &symbolName, const QDateTime &timestamp, float range);
    void addBTWSymbol(const GraphType &graphType, BTWSymbolDrawing::SymbolType symbolType, const QDateTime &timestamp, float range);
    void addBTWMarker(const GraphType &graphType, const QDateTime &timestamp, float range, float delta);
    void addRTWRMarker(const GraphType &graphType, const QDateTime &timestamp, float range);
    
    // Remove individual markers and symbols
    bool removeBTWSymbol(const GraphType &graphType, const QString &symbolName, const QDateTime &timestamp, float range, float toleranceMs = 1000, float rangeTolerance = 0.1f);
    bool removeBTWSymbol(const GraphType &graphType, BTWSymbolDrawing::SymbolType symbolType, const QDateTime &timestamp, float range, float toleranceMs = 1000, float rangeTolerance = 0.1f);
    bool removeRTWSymbol(const GraphType &graphType, const QString &symbolName, const QDateTime &timestamp, float range, float toleranceMs = 1000, float rangeTolerance = 0.1f);
    bool removeBTWMarker(const GraphType &graphType, const QDateTime &timestamp, float range, float toleranceMs = 1000, float rangeTolerance = 0.1f);
    bool removeRTWRMarker(const GraphType &graphType, const QDateTime &timestamp, float range, float toleranceMs = 1000, float rangeTolerance = 0.1f);
    
    // ========== RTW Ruler indicator API ==========
    // The main system owns up to 4 rulers (index 0..3). These forward to the
    // RTW graph view(s); state lives in RTWGraph. At most one ruler is selected.
    void setRtwRulerActive(int index, const QDateTime &timestamp, qreal range);
    void clearRtwRuler(int index);
    void clearAllRtwRulers();
    void setSelectedRtwRuler(int index);
    int selectedRtwRuler() const;

    // ========== BTW Ruler indicator API ==========
    void setBtwRulerActive(int index, const QDateTime &timestamp, qreal range);
    void clearBtwRuler(int index);
    void clearAllBtwRulers();
    void setSelectedBtwRuler(int index);
    int selectedBtwRuler() const;

    // Clear markers and symbols for specific graph type
    void clearRTWSymbols(const GraphType &graphType);
    void clearBTWSymbols(const GraphType &graphType);
    void clearBTWMarkers(const GraphType &graphType);
    void clearRTWRMarkers(const GraphType &graphType);
    
    // Clear BTW manual markers (interactive overlay markers)
    void clearBTWManualMarkers();
    
    /**
     * @brief Add a BTW manual marker programmatically to the current BTW graph
     * @param timestamp The timestamp for the marker
     * @param rangeValue The range value for the marker
     * @param bearingRate Optional bearing rate (defaults to 0.0)
     * @return true if marker was created successfully
     */
    bool addBTWManualMarker(const QDateTime &timestamp, float rangeValue, float bearingRate = 0.0f);
    
    // ========== Horizontal time line management (all graph types) ==========

    void setHorizontalLineMode(HorizontalLineMode mode);
    HorizontalLineMode horizontalLineMode() const;
    QUuid addHorizontalLine(const QDateTime &timestamp, const QColor &color = Qt::white, qreal width = 2.0);
    QDateTime getHorizontalLineTimestamp(const QUuid &syncId) const;
    bool removeHorizontalLine(const QUuid &syncId);
    void clearHorizontalLines();
    std::vector<HorizontalLineSyncData> getActiveHorizontalLines() const;

    /** @deprecated Use setHorizontalLineMode() — graphType is ignored. */
    void setBTWHorizontalLineMode(const GraphType &graphType, HorizontalLineMode mode);
    void setBTWHorizontalLineMode(const GraphType &graphType, bool enabled);
    QUuid addBTWHorizontalLine(const GraphType &graphType, const QDateTime &timestamp, const QColor &color = Qt::white, qreal width = 2.0);
    QDateTime getBTWHorizontalLineTimestamp(const GraphType &graphType, const QUuid &lineId) const;
    bool removeBTWHorizontalLine(const GraphType &graphType, const QUuid &lineId);
    void clearBTWHorizontalLines(const GraphType &graphType);
    
    // ========== Shaded Region API ==========
    
    /**
     * @brief Add a shaded region to all BTW graphs
     * 
     * The shaded region will be drawn as a cross-hatched vertical band
     * spanning from top to bottom (all timestamps), with horizontal 
     * boundaries defined by the X range values.
     * 
     * @param startX Starting X value (left range boundary)
     * @param endX Ending X value (right range boundary)
     * @return The sync ID of the created region (can be used for removal)
     */
    QUuid addShadedRegionToAllBTW(qreal startX, qreal endX);
    
    /**
     * @brief Remove a shaded region from all BTW graphs by sync ID
     * @param syncId The global sync ID of the region to remove
     * @return true if region was found and removed
     */
    bool removeShadedRegionFromAllBTW(const QUuid &syncId);
    
    /**
     * @brief Clear all shaded regions from all BTW graphs
     */
    void clearAllShadedRegions();
    
    /**
     * @brief Get all active shaded regions
     * @return Vector of shaded region sync data
     */
    std::vector<ShadedRegionSyncData> getAllShadedRegions() const;
    
    // Redraw specific graph
    void redrawGraph(const GraphType &graphType);
    
    // Redraw all graphs
    void redrawAllGraphs();
    
    // Fast track switching API - marks track change for visible-window-first rendering
    /**
     * @brief Mark that a track change has occurred for all graphs.
     * 
     * This method enables fast track switching by:
     * - Clearing all caches in all graphs to prevent stale data
     * - Triggering visible-window-first rendering (only builds geometry for current visible window)
     * - Ensuring immediate visual feedback without blocking on full historical rebuild
     * 
     * The track change mode is automatically reset after rendering completes,
     * and normal operation resumes.
     * 
     * Call this when switching between tracks in a multi-track system (e.g., 64-track system).
     * This will mark all graphs in all containers for fast track switching.
     */
    void markTrackChanged();

    // Capacity management API - set sizes for all arrays used to store graph data
    /**
     * @brief Set capacity for all data series arrays in all data sources
     * 
     * Reserves capacity for Y data, timestamps, and epoch timestamps vectors
     * in all data sources to reduce reallocations during data addition.
     * 
     * @param capacity Number of elements to reserve for each data series
     */
    void setDataSeriesCapacity(size_t capacity);
    
    /**
     * @brief Set capacity for all symbol arrays in all data sources
     * 
     * Reserves capacity for RTW and BTW symbols vectors in all data sources.
     * 
     * @param capacity Number of elements to reserve for symbols
     */
    void setSymbolsCapacity(size_t capacity);
    
    /**
     * @brief Set capacity for all marker arrays in all data sources
     * 
     * Reserves capacity for BTW markers and RTW R markers vectors in all data sources.
     * 
     * @param capacity Number of elements to reserve for markers
     */
    void setMarkersCapacity(size_t capacity);
    
    /**
     * @brief Set capacity for all rendering cache arrays in all graphs
     * 
     * Reserves capacity for scatter points, batched line paths, and cached visible data
     * vectors in all graphs to reduce reallocations during rendering.
     * 
     * @param scatterCapacity Capacity for scatter points
     * @param linePathsCapacity Capacity for batched line paths
     * @param cachedDataCapacity Capacity for cached visible data
     */
    void setRenderingCachesCapacity(size_t scatterCapacity, size_t linePathsCapacity, size_t cachedDataCapacity);
    
    /**
     * @brief Set capacity for all arrays in the system (data sources and graphs)
     * 
     * Comprehensive method that sets capacity for all arrays used to store graph data.
     * This is the recommended method to use for initial setup.
     * 
     * @param dataSeriesCapacity Capacity for data series vectors (Y data, timestamps)
     * @param symbolsCapacity Capacity for symbol vectors (RTW, BTW)
     * @param markersCapacity Capacity for marker vectors (BTW markers, RTW R markers)
     * @param scatterCapacity Capacity for scatter points in graphs
     * @param linePathsCapacity Capacity for batched line paths in graphs
     * @param cachedDataCapacity Capacity for cached visible data in graphs
     */
    void setAllArraysCapacity(size_t dataSeriesCapacity, size_t symbolsCapacity, size_t markersCapacity,
                              size_t scatterCapacity, size_t linePathsCapacity, size_t cachedDataCapacity);

protected:
    void resizeEvent(QResizeEvent *event) override;

public slots:
    void onTimerTick();
    void onTimeSelectionCreated(const TimeSelectionSpan &selection);
    void onTimeSelectionModified(int index, const TimeSelectionSpan &newSpan);
    void onTimeSelectionsCleared();
    void onBTWManualMarkerPlaced(const QDateTime &timestamp, const QPointF &position);
    
    void onHorizontalLineSyncAdded(const HorizontalLineSyncData &lineData);
    void onHorizontalLineSyncUpdated(const HorizontalLineSyncData &lineData);
    void onHorizontalLineSyncRemoved(const QUuid &syncId);
    void onHorizontalLineSyncDragStarted(const QUuid &syncId);
    void onHorizontalLineSyncDragEnded();
    void onHorizontalLinesSyncCleared();
    
    // BTW Marker sync slots - propagate markers to all containers
    void onBTWMarkerSyncDataChanged(const BTWSyncMarkerData &markerData);
    void onBTWMarkerSyncDeleted(const QUuid &markerId);
    
    // Shaded region sync slots - propagate regions to all containers
    void onShadedRegionSyncAdded(const ShadedRegionSyncData &regionData);
    void onShadedRegionSyncRemoved(const QUuid &syncId);
    void onShadedRegionsSyncCleared();

public slots:
    void onContainerIntervalChanged(TimeInterval interval);

private:
    void applyHorizontalLineModeToAllGraphs();
    void applyHorizontalLineSyncToAllContainers(const HorizontalLineSyncData &lineData);
    void refreshAllHorizontalLineVisuals();
    QElapsedTimer m_horizontalLineDragThrottle;
    QUuid m_pendingHorizontalLineDragSyncId;
    HorizontalLineSyncData m_pendingHorizontalLineDragData;
    bool m_hasPendingHorizontalLineDrag = false;

    LayoutType m_layoutType;
    QTimer *m_timer;
    std::vector<GraphContainer *> m_graphContainers;
    std::vector<QString> m_containerLabels;

    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_graphContainersRow1Layout;
    QHBoxLayout *m_graphContainersRow2Layout;

    // Engine storage (owns WaterfallData internally)
    std::map<GraphType, GraphEngine*> m_engines;

    // Series colors map
    std::map<QString, QColor> m_seriesColorsMap;
    
    // Interactive update throttling
    std::map<GraphType, QElapsedTimer> m_interactiveUpdateTimers;
    std::map<GraphType, bool> m_pendingInteractiveUpdate;
    static constexpr qint64 INTERACTIVE_UPDATE_THROTTLE_MS = 16; // ~60 FPS max

    void attachContainerDataSources();
    void initializeContainers();
    void initializeDataSources(std::map<GraphType, std::vector<QPair<QString, QColor>>> seriesLabelsMap);
    int getContainerIndex(const QString &containerLabel) const;
    void disconnectAllContainerConnections();
    void notifyGraphDataChanged(GraphType graphType, bool forceFullRedraw = false);
    void registerCursorSyncCallbacks();
    void onContainerCursorTimeChanged(GraphContainer *source, const QDateTime &time);
    
    // Helper to add BTW symbol (magenta circle) to all graphs at a timestamp
    void addBTWSymbolToAllGraphs(const QDateTime &timestamp, float range);
    
    // Batch method to add magenta circles for all existing BTW markers (more efficient)
    void addBTWSymbolsForExistingBTWMarkers();
    
    // Helper to add BTW symbol to a single graph without redraw (for batch processing)
    bool addBTWSymbolToGraph(WaterfallData *dataSource, const QDateTime &timestamp, bool skipIfExists = true);

    // Container synchronization state
    GraphContainerSyncState m_syncState;

    SharedCacheStore m_sharedRenderCache;

    // Centralized time-scope propagation (replaces ad-hoc connections + ScopeCoalescer).
    TimeScopeBus m_scopeBus;
    int          m_scopeBusWriterToken = -1;

    QDateTime m_systemStartTimeAtInit;

    void propagateSystemStartTimeToContainers();

    // Manoeuvre drawing state
    bool m_manoeuvreDrawingInProgress; ///< Flag indicating if a manoeuvre is currently being drawn
    QDateTime m_currentManoeuvreStartTime; ///< Start time of the manoeuvre being drawn
    int m_currentManoeuvreBearing; ///< Bearing of the manoeuvre being drawn
    int m_currentManoeuvreSpeed; ///< Speed of the manoeuvre being drawn
    int m_currentManoeuvreDepth; ///< Depth of the manoeuvre being drawn

signals:
    /**
     * @brief Emitted whenever the set of graphs shown on screen may have changed
     * (a container's current graph changed, or the layout changed).
     * Listeners can call getVisibleGraphNames() to read the new state.
     */
    void VisibleGraphsChanged();

    void TimeSelectionCreated(const TimeSelectionSpan &selection);
    void TimeSelectionModified(int index, const TimeSelectionSpan &newSpan);
    void TimeSelectionsCleared();
    
    // Marker timestamp signals for external integration
    /**
     * @brief Emitted when an RTW R marker is clicked
     * @param timestamp The timestamp of the clicked R marker
     * @param position The scene position where the marker was clicked
     */
    void RTWRMarkerTimestampCaptured(const QDateTime &timestamp, const QPointF &position);
    
    /**
     * @brief Emitted when an RTW symbol is clicked
     * @param timestamp The timestamp of the clicked RTW symbol
     * @param position The scene position where the symbol was clicked
     * @param symbolName The name of the clicked symbol
     */
    void RTWSymbolTimestampCaptured(const QDateTime &timestamp, const QPointF &position, const QString &symbolName);

    /**
     * @brief Emitted when an RTW ruler indicator is clicked (and thereby selected).
     * @param index The 0-based ruler index (0..3)
     * @param timestamp The ruler's time-axis position
     * @param range The ruler's range-axis position
     */
    void RtwRulerSelected(int index, const QDateTime &timestamp, qreal range);

    /**
     * @brief Emitted when a BTW ruler indicator is clicked (and thereby selected).
     */
    void BtwRulerSelected(int index, const QDateTime &timestamp, qreal range);
    
    /**
     * @brief Emitted when a BTW manual marker is placed
     * @param timestamp The timestamp of the placed marker
     * @param position The scene position where the marker was placed
     */
    void BTWManualMarkerPlaced(const QDateTime &timestamp, const QPointF &position);
    
    /**
     * @brief Emitted when a BTW symbol (magenta circle) is added to all graphs
     * @param timestamp The timestamp where the magenta circle was added
     */
    void BTWSymbolAddedToAllGraphs(const QDateTime &timestamp);
    
    /**
     * @brief Emitted when a BTW manual marker is clicked
     * @param timestamp The timestamp of the clicked marker
     * @param position The scene position where the marker was clicked
     */
    void BTWManualMarkerClicked(const QDateTime &timestamp, const QPointF &position);
    
    /**
     * @brief Emitted when a marker timestamp and value change (new marker placed or marker clicked)
     * @param timestamp The timestamp of the marker
     * @param value The value (range) of the marker
     */
    void markerTimestampValueChanged(const QDateTime &timestamp, qreal value);
    
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
};

#endif // GRAPHLAYOUT_H
