#ifndef GRAPHCONTAINER_H
#define GRAPHCONTAINER_H

#include "bdwgraph.h"
#include "brwgraph.h"
#include "btwgraph.h"
#include "fdwgraph.h"
#include "ftwgraph.h"
#include "graphtype.h"
#include "ltwgraph.h"
#include "rtwgraph.h"
#include "timelineutils.h"
#include "timelineview.h"
#include "timeselectionvisualizer.h"
#include "waterfalldata.h"
#include "waterfallgraph.h"
#include "zoompanel.h"
#include <QComboBox>
#include <QDateTime>
#include <QHBoxLayout>
#include <QString>
#include <QTimer>
#include <QUuid>
#include <QVBoxLayout>
#include <QWidget>
#include <functional>
#include <map>
#include <vector>
#include "sharedsyncstate.h"
#include "scopecoalescer.h"

class SharedCacheStore;

class GraphContainer : public QWidget
{
    Q_OBJECT
public:
    explicit GraphContainer(QWidget *parent = nullptr, 
        bool showTimelineView = true, 
        std::map<QString, QColor> seriesColorsMap = std::map<QString, QColor>(), 
        QTimer *timer = nullptr, 
        int containerWidth = 0, 
        int containerHeight = 0, 
        GraphContainerSyncState *syncState = nullptr
    );
    ~GraphContainer();
    void setShowTimelineView(bool showTimelineView);
    bool getShowTimelineView();
    TimelineView *getTimelineView() const;

    /** Apply m_syncState application start time to timeline and waterfall graphs (after GraphLayout updates sync). */
    void applySharedSystemStartTimeFromSync();

    /** Internal: shared visible-data projection cache (owned by GraphLayout). */
    void attachSharedCacheStore(SharedCacheStore *store);
    void setShowTimeSelectionVisualizer(bool show);

    // Sizing methods
    int getTimelineWidth() const;
    
    // Container geometry methods
    void setContainerHeight(int height);
    void setContainerWidth(int width);
    void setContainerSize(int width, int height);
    int getContainerHeight() const;
    int getContainerWidth() const;
    QSize getContainerSize() const;

    // Graph view sizing methods
    void setGraphViewSize(int width, int height);
    QSize getGraphViewSize() const;
    QSize getTotalContainerSize() const;
    
    // Get the combined height of combo box and zoom panel
    int getComboBoxAndZoomPanelHeight() const;


    // Data access methods
    WaterfallData getData() const;
    std::vector<std::pair<qreal, QDateTime>> getDataWithinYExtents(qreal yMin, qreal yMax) const;
    std::vector<std::pair<qreal, QDateTime>> getDataWithinTimeRange(const QDateTime &startTime, const QDateTime &endTime) const;

    // Min/max data methods
    qreal getMinY() const;
    qreal getMaxY() const;
    std::pair<qreal, qreal> getYRange() const;

    // Graph redraw method
    void redrawWaterfallGraph();
    void redrawWaterfallGraph(GraphType graphType); // Redraw a specific graph type
    
    // Get the current waterfall graph
    WaterfallGraph* getCurrentWaterfallGraph() const;
    
    // Get a specific waterfall graph by type
    WaterfallGraph* getWaterfallGraph(GraphType graphType) const;

    // Data options management
    void addDataOption(const GraphType graphType, WaterfallData &dataSource);
    void removeDataOption(const GraphType graphType);
    void clearDataOptions();
    void setCurrentDataOption(const GraphType graphType);
    GraphType getCurrentDataOption() const;
    std::vector<GraphType> getAvailableDataOptions() const;
    WaterfallData *getDataOption(const GraphType graphType);
    bool hasDataOption(const GraphType graphType) const;

    // Signal subscription method for external components
    void subscribeToIntervalChange(QObject *subscriber, const char *slot);

    // Mouse selection control
    void setMouseSelectionEnabled(bool enabled);
    bool isMouseSelectionEnabled() const;

    // Set the current time
    void setCurrentTime(const QTime &time);
    
    // Cursor synchronization
    void setCursorTimeChangedCallback(const std::function<void(GraphContainer *, const QDateTime &)> &callback);
    void applySharedTimeAxisCursor(const QDateTime &time);

    // Selection management methods
    void addTimeSelection(const TimeSelectionSpan &selection);
    void setTimeSelection(int index, const TimeSelectionSpan &selection);  // replace at index (for sync from GraphLayout)
    void clearTimeSelections();
    void clearTimeSelectionsSilent(); // Clears without emitting signal

    // Test method
    void testSelectionRectangle();
    void deleteInteractiveMarkers();

    // Public method for external components to update zoom panel limits
    void initializeZoomPanelLimits();

    // Public method for external components to update time interval
    void updateTimeInterval(TimeInterval interval);

    // API to set time interval without emitting signals (for centralized sync)
    void setTimeInterval(TimeInterval interval);
    
    // API to set time scope without emitting signals (for centralized sync from GraphLayout hub)
    void setTimeScope(const TimeSelectionSpan &selection);


    // Chevron label control methods
    void setChevronLabel1(const QString &label);
    void setChevronLabel2(const QString &label);
    void setChevronLabel3(const QString &label);
    QString getChevronLabel1() const;
    QString getChevronLabel2() const;
    QString getChevronLabel3() const;

    // Manoeuvre methods
    void setManoeuvres(const std::vector<Manoeuvre> *manoeuvres);
    void setInProgressManoeuvre(const QDateTime &startTime);
    void clearInProgressManoeuvre();

    // Range limits management methods
    void setGraphRangeLimits(const GraphType graphType, qreal yMin, qreal yMax);
    void removeGraphRangeLimits(const GraphType graphType);
    void clearAllGraphRangeLimits();
    bool hasGraphRangeLimits(const GraphType graphType) const;
    std::pair<qreal, qreal> getGraphRangeLimits(const GraphType graphType) const;

    // Computed property getters for visualization state
    QDateTime getCurrentDisplayTimeMin() const;
    QDateTime getCurrentDisplayTimeMax() const;
    std::pair<QDateTime, QDateTime> getCurrentDisplayTimeRange() const;
    QDateTime getAvailableDataTimeMin() const;
    QDateTime getAvailableDataTimeMax() const;
    std::pair<QDateTime, QDateTime> getAvailableDataTimeRange() const;
    qreal getAvailableDataYMin() const;
    qreal getAvailableDataYMax() const;
    std::pair<qreal, qreal> getAvailableDataYRange() const;
    WaterfallData *getCurrentWaterfallData() const;

public slots:
    void onTimeIntervalChanged(TimeInterval interval);
    void onSelectionCreated(const TimeSelectionSpan &selection);
    void onZoomValueChanged(ZoomBounds bounds);
    void onTimeSelectionMade(const TimeSelectionSpan &selection);
    void onTimeScopeChanged(const TimeSelectionSpan &selection, bool fromFrozenUserDrag);
    /** Overload for container-to-container sync (GraphContainer::TimeScopeChanged has one arg). */
    void onTimeScopeChanged(const TimeSelectionSpan &selection);
    
    // Marker timestamp slots
    void onRTWRMarkerTimestampCaptured(const QDateTime &timestamp, const QPointF &position);
    void onRTWSymbolTimestampCaptured(const QDateTime &timestamp, const QPointF &position, const QString &symbolName);
    void onBTWManualMarkerPlaced(const QDateTime &timestamp, const QPointF &position);
    void onBTWManualMarkerClicked(const QDateTime &timestamp, const QPointF &position);
    void onGraphContainerInFollowModeChanged(bool isInFollowMode);
    
    // BTW Horizontal line slots
    void onBTWHorizontalLinePlaced(const QUuid &lineId, const QDateTime &timestamp);
    void onBTWHorizontalLineRemoved(const QUuid &lineId, const QDateTime &timestamp);
    
    // BTW Marker sync slots (called when syncing markers from other containers)
    void onBTWMarkerSyncDataChanged(const BTWSyncMarkerData &markerData);
    void onBTWMarkerSyncDeleted(const QUuid &markerId);
    
    // Shaded region sync slots (called when syncing regions from other containers)
    void onShadedRegionSyncAdded(const ShadedRegionSyncData &regionData);
    void onShadedRegionSyncRemoved(const QUuid &syncId);
    void onShadedRegionsSyncCleared();
    // Unified data change notification handler
    void onDataChanged(GraphType graphType);
    
    // Fast incremental update for interactive drag (no range recalculation, no full redraw)
    void onDataChangedInteractive(GraphType graphType, const QString &seriesLabel);

private:
    void updateTotalContainerSize();
    void updateComboBoxOptions();
    void onDataOptionChanged(QString title);
    void setupEventConnections();
    void setupEventConnectionsForWaterfallGraph();
    WaterfallGraph *createWaterfallGraph(GraphType graphType);
    void createAllWaterfallGraphs();
    void setupWaterfallGraphProperties(WaterfallGraph *graph, GraphType graphType);
    void initializeWaterfallGraph(GraphType graphType);
    void handleCursorTimeChanged(const QDateTime &time);
    void applyCursorTimeToGraph(WaterfallGraph *graph);
    void setupTimer();
    void onTimerTick();
    void onClearTimeSelectionsButtonClicked();
    void onHistoryFullSelectionRequested();

signals:
    void TimeSelectionCreated(const TimeSelectionSpan &selection);
    void TimeSelectionModified(int index, const TimeSelectionSpan &newSpan);
    void DeltaTimeSelectionChanged(qreal deltaTime);
    void TimeSelectionsCleared();
    void IntervalChanged(TimeInterval interval);
    void TimeScopeChanged(const TimeSelectionSpan &selection);
    void DeleteInteractiveMarkers();
    
    // Marker timestamp signals
    void RTWRMarkerTimestampCaptured(const QDateTime &timestamp, const QPointF &position);
    void RTWSymbolTimestampCaptured(const QDateTime &timestamp, const QPointF &position, const QString &symbolName);
    void BTWManualMarkerPlaced(const QDateTime &timestamp, const QPointF &position);
    void BTWManualMarkerClicked(const QDateTime &timestamp, const QPointF &position);
    
    // BTW Horizontal line signals
    void BTWHorizontalLinePlaced(const QUuid &lineId, const QDateTime &timestamp);
    void BTWHorizontalLineRemoved(const QUuid &lineId, const QDateTime &timestamp);
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
    
    // ========== BTW Marker Sync Signals ==========
    
    /**
     * @brief Emitted when a BTW marker's data changes and needs to be synced
     * @param markerData The current state of the marker
     */
    void BTWMarkerSyncDataChanged(const BTWSyncMarkerData &markerData);
    
    /**
     * @brief Emitted when a BTW marker is deleted and needs to be synced
     * @param markerId The unique ID of the deleted marker
     */
    void BTWMarkerSyncDeleted(const QUuid &markerId);
    
    // ========== Shaded Region Sync Signals ==========
    
    /**
     * @brief Emitted when a shaded region is added and needs to be synced
     * @param regionData The shaded region data to sync
     */
    void ShadedRegionSyncAdded(const ShadedRegionSyncData &regionData);
    
    /**
     * @brief Emitted when a shaded region is removed and needs to be synced
     * @param syncId The global sync ID of the removed region
     */
    void ShadedRegionSyncRemoved(const QUuid &syncId);
    
    /**
     * @brief Emitted when all shaded regions are cleared
     */
    void ShadedRegionsSyncCleared();

private:
    QHBoxLayout *m_mainLayout;
    QVBoxLayout *m_waterfallLayout;
    QComboBox *m_comboBox;
    ZoomPanel *m_zoomPanel;
    WaterfallGraph *m_currentWaterfallGraph;
    TimeSelectionVisualizer *m_timelineSelectionView;
    TimelineView *m_timelineView;
    bool m_showTimelineView;

    // Waterfallgraph management
    std::map<GraphType, WaterfallGraph *> m_waterfallGraphs;

    // Timer management
    QTimer *m_timer;
    bool m_ownsTimer;

    // Sizing properties
    int m_timelineWidth;
    QSize m_graphViewSize;

    // Series colors map
    std::map<QString, QColor> m_seriesColorsMap;

    // Data source management
    WaterfallData waterfallData;

    // Data options management
    std::map<GraphType, WaterfallData *> dataOptions;
    GraphType currentDataOption;

    // Range limits management
    std::map<GraphType, std::pair<qreal, qreal>> graphRangeLimits;

    // Flag to prevent TimeScopeChanged from interfering with interval updates
    bool m_updatingTimeInterval;

    // Cursor sync state
    std::function<void(GraphContainer *, const QDateTime &)> m_cursorTimeChangedCallback;
    QDateTime m_sharedCursorTime;
    bool m_hasSharedCursorTime;

    // Graph container in follow mode
    bool m_isInFollowMode = true;

    // Shared synchronization state pointer
    GraphContainerSyncState *m_syncState;
    
    // Track last known time scope from sync state to detect changes
    TimeSelectionSpan m_lastSyncedTimeScope;
    bool m_hasLastSyncedTimeScope;

    ScopeCoalescer m_scopeCoalescer;
};

#endif // GRAPHCONTAINER_H
