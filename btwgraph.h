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
#include <QGraphicsPolygonItem>
#include <vector>

// Forward declarations to avoid circular dependency
class BTWInteractiveOverlay;
class InteractiveGraphicsItem;
class ZoomPanel;
class GraphContainer;

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
     * @brief Get all timestamps from automatic markers
     * @return Vector of timestamps from all automatic markers that were created
     */
    std::vector<QDateTime> getAutomaticMarkerTimestamps() const;
    
    /**
     * @brief Add a BTW symbol to the graph
     * @param symbolName Name of the symbol (e.g., "MagentaCircle")
     * @param timestamp Timestamp when the symbol should be displayed
     * @param range Range value (Y-axis position) where the symbol should be displayed
     */
    void addBTWSymbol(const QString &symbolName, const QDateTime &timestamp, qreal range);
    
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

public slots:
    void deleteInteractiveMarkers();

protected:
    // Override the draw method to create scatterplots by default
    void draw() override;

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
    void drawBTWSymbols();
    BTWSymbolDrawing::SymbolType symbolNameToType(const QString &symbolName) const;
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
        
        ShadedRegionItem() : polygonItem(nullptr) {}
        ShadedRegionItem(const ShadedRegionData &d) : data(d), polygonItem(nullptr) {}
    };
    QMap<int, ShadedRegionItem> m_shadedRegions;
    int m_nextRegionId;
    
    // Helper method to get zoom panel from parent GraphContainer
    ZoomPanel* getZoomPanel() const;
    
    // Method to draw shaded regions
    void drawShadedRegions();

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
};

#endif // BTWGRAPH_H
