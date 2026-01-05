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
#include <map>
#include <vector>
#include "sharedsyncstate.h"

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
    explicit GraphLayout(QWidget *parent, LayoutType layoutType, QTimer *timer = nullptr, std::map<GraphType, std::vector<QPair<QString, QColor>>> seriesLabelsMap = std::map<GraphType, std::vector<QPair<QString, QColor>>>());
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

    // Data options management - operate on all visible containers
    void addDataOption(const GraphType &graphType, WaterfallData &dataSource);
    void removeDataOption(const GraphType &graphType);
    void clearDataOptions();
    void setCurrentDataOption(const GraphType &graphType);

    // Data point methods for specific data sources
    void addDataPointToDataSource(const GraphType &graphType, const QString &seriesLabel, qreal yValue, const QDateTime &timestamp);
    void addDataPointsToDataSource(const GraphType &graphType, const QString &seriesLabel, const std::vector<qreal> &yValues, const std::vector<QDateTime> &timestamps);
    void setDataToDataSource(const GraphType &graphType, const QString &seriesLabel, const std::vector<qreal> &yData, const std::vector<QDateTime> &timestamps);
    void setDataToDataSource(const GraphType &graphType, const QString &seriesLabel, const WaterfallData &data);
    void clearDataSource(const GraphType &graphType, const QString &seriesLabel);

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

    // Marker and symbol management methods - operate on specific graph type
    void addRTWSymbol(const GraphType &graphType, const QString &symbolName, const QDateTime &timestamp, qreal range);
    void addBTWSymbol(const GraphType &graphType, const QString &symbolName, const QDateTime &timestamp, qreal range);
    void addBTWMarker(const GraphType &graphType, const QDateTime &timestamp, qreal range, qreal delta);
    void addRTWRMarker(const GraphType &graphType, const QDateTime &timestamp, qreal range);
    
    // Remove individual markers and symbols
    bool removeRTWSymbol(const GraphType &graphType, const QString &symbolName, const QDateTime &timestamp, qreal range, qreal toleranceMs = 1000, qreal rangeTolerance = 0.1);
    bool removeBTWMarker(const GraphType &graphType, const QDateTime &timestamp, qreal range, qreal toleranceMs = 1000, qreal rangeTolerance = 0.1);
    bool removeRTWRMarker(const GraphType &graphType, const QDateTime &timestamp, qreal range, qreal toleranceMs = 1000, qreal rangeTolerance = 0.1);
    
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
    bool addBTWManualMarker(const QDateTime &timestamp, qreal rangeValue, qreal bearingRate = 0.0);
    
    // ========== BTW Horizontal Line Management ==========
    
    /**
     * @brief Set horizontal line mode for BTW graphs
     * @param graphType The graph type (should be BTW)
     * @param mode The mode to set (Normal, DrawLine, or DeleteLine)
     */
    void setBTWHorizontalLineMode(const GraphType &graphType, BTWGraph::HorizontalLineMode mode);
    
    /**
     * @brief Set horizontal line mode for BTW graphs (legacy boolean interface)
     * @param graphType The graph type (should be BTW)
     * @param enabled True to enable draw line mode, false for normal mode
     */
    void setBTWHorizontalLineMode(const GraphType &graphType, bool enabled);
    
    /**
     * @brief Add a horizontal line to a BTW graph at a specific time
     * @param graphType The graph type (should be BTW)
     * @param timestamp The time when the line should be drawn
     * @param color The color of the line (default: yellow)
     * @param width The width of the line (default: 2.0)
     * @return Unique identifier for the line
     */
    QUuid addBTWHorizontalLine(const GraphType &graphType, const QDateTime &timestamp, const QColor &color = Qt::white, qreal width = 2.0);
    
    /**
     * @brief Get the timestamp of a horizontal line by its ID
     * @param graphType The graph type (should be BTW)
     * @param lineId The unique identifier of the line
     * @return The timestamp of the line, or invalid QDateTime if not found
     */
    QDateTime getBTWHorizontalLineTimestamp(const GraphType &graphType, const QUuid &lineId) const;
    
    /**
     * @brief Remove a horizontal line from a BTW graph by its ID
     * @param graphType The graph type (should be BTW)
     * @param lineId The unique identifier of the line to remove
     * @return True if the line was found and removed
     */
    bool removeBTWHorizontalLine(const GraphType &graphType, const QUuid &lineId);
    
    /**
     * @brief Clear all horizontal lines from a BTW graph
     * @param graphType The graph type (should be BTW)
     */
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
    void onTimeSelectionsCleared();
    void onBTWManualMarkerPlaced(const QDateTime &timestamp, const QPointF &position);
    
    // BTW Horizontal line sync slots - propagate lines to all containers
    void onBTWHorizontalLinePlaced(const QUuid &lineId, const QDateTime &timestamp);
    void onBTWHorizontalLineRemoved(const QUuid &lineId, const QDateTime &timestamp);
    
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

    void attachContainerDataSources();
    void initializeContainers();
    void initializeDataSources(std::map<GraphType, std::vector<QPair<QString, QColor>>> seriesLabelsMap);
    int getContainerIndex(const QString &containerLabel) const;
    void disconnectAllContainerConnections();
    void propagateTimeSelectionToAllContainers(const TimeSelectionSpan &selection);
    void registerCursorSyncCallbacks();
    void onContainerCursorTimeChanged(GraphContainer *source, const QDateTime &time);
    void onContainerTimeScopeChanged(const TimeSelectionSpan &selection);
    
    // Helper to add BTW symbol (magenta circle) to all graphs at a timestamp
    void addBTWSymbolToAllGraphs(const QDateTime &timestamp, qreal range);
    
    // Batch method to add magenta circles for all existing BTW markers (more efficient)
    void addBTWSymbolsForExistingBTWMarkers();
    
    // Helper to add BTW symbol to a single graph without redraw (for batch processing)
    bool addBTWSymbolToGraph(WaterfallData *dataSource, const QDateTime &timestamp, bool skipIfExists = true);

    // Container synchronization state
    GraphContainerSyncState m_syncState;

    // Manoeuvre drawing state
    bool m_manoeuvreDrawingInProgress; ///< Flag indicating if a manoeuvre is currently being drawn
    QDateTime m_currentManoeuvreStartTime; ///< Start time of the manoeuvre being drawn
    int m_currentManoeuvreBearing; ///< Bearing of the manoeuvre being drawn
    int m_currentManoeuvreSpeed; ///< Speed of the manoeuvre being drawn
    int m_currentManoeuvreDepth; ///< Depth of the manoeuvre being drawn

signals:
    void TimeSelectionCreated(const TimeSelectionSpan &selection);
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
