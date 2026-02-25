#ifndef BTWINTERACTIVEOVERLAY_H
#define BTWINTERACTIVEOVERLAY_H

#include <QObject>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsPixmapItem>
#include <QPointF>
#include <QDateTime>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QList>
#include <QMap>
#include <QUuid>
#include <QPixmap>
#include <QDebug>

// Forward declarations
class BTWGraph;
struct BTWSyncMarkerData;

// Include full type so moc has a complete type for signals using InteractiveGraphicsItem*
#include "interactivegraphicsitem.h"

/**
 * @brief Interactive overlay manager for BTW graph
 * 
 * This class manages interactive markers on the BTW graph's overlay scene.
 * It provides functionality to add, remove, and manage different types of
 * interactive markers with drag and rotation capabilities.
 */
class BTWInteractiveOverlay : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Types of markers that can be added
     */
    enum MarkerType {
        DataPoint,      ///< Data point marker
        ReferenceLine,  ///< Reference line marker
        Annotation,     ///< Text annotation marker
        CustomMarker    ///< Custom marker
    };

    /**
     * @brief Constructor
     * @param btwGraph Pointer to the BTW graph
     * @param parent Parent object
     */
    explicit BTWInteractiveOverlay(BTWGraph *btwGraph, QObject *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~BTWInteractiveOverlay();

    /**
     * @brief Add a data point marker
     * @param position Position of the marker
     * @param timestamp Timestamp for the data point
     * @param value Value of the data point
     * @param seriesLabel Label for the series
     * @return Pointer to the created marker
     */
    InteractiveGraphicsItem* addDataPointMarker(const QPointF &position, const QDateTime &timestamp, float value, const QString &seriesLabel);

    /**
     * @brief Add a reference line marker
     * @param startPos Start position of the line
     * @param endPos End position of the line
     * @param label Label for the reference line
     * @return Pointer to the created marker
     */
    InteractiveGraphicsItem* addReferenceLineMarker(const QPointF &startPos, const QPointF &endPos, const QString &label);

    /**
     * @brief Add an annotation marker
     * @param position Position of the annotation
     * @param text Text content of the annotation
     * @param color Color of the annotation
     * @return Pointer to the created marker
     */
    InteractiveGraphicsItem* addAnnotationMarker(const QPointF &position, const QString &text, const QColor &color = Qt::black);

    /**
     * @brief Add a custom marker
     * @param position Position of the marker
     * @param size Size of the marker
     * @return Pointer to the created marker
     */
    InteractiveGraphicsItem* addCustomMarker(const QPointF &position, const QSizeF &size = QSizeF(30, 30));

    /**
     * @brief Remove a specific marker
     * @param marker Pointer to the marker to remove
     */
    void removeMarker(InteractiveGraphicsItem *marker);

    /**
     * @brief Clear all markers
     */
    void clearAllMarkers();
    
    // ========== Marker Sync Methods ==========
    
    /**
     * @brief Create a marker from sync data
     * 
     * Creates a marker using timestamp, range, and bearing rate data.
     * This is used for syncing markers across different containers.
     * 
     * @param markerData The BTW marker data containing timestamp, range, and bearing rate
     * @return Pointer to the created marker, or nullptr if creation failed
     */
    InteractiveGraphicsItem* createMarkerFromData(const BTWSyncMarkerData &markerData);
    
    /**
     * @brief Find a marker by its unique ID
     * @param id The unique identifier of the marker
     * @return Pointer to the marker, or nullptr if not found
     */
    InteractiveGraphicsItem* findMarkerById(const QUuid &id) const;
    
    /**
     * @brief Get the unique ID of a marker
     * @param marker Pointer to the marker
     * @return The UUID of the marker, or null UUID if not found
     */
    QUuid getMarkerId(InteractiveGraphicsItem *marker) const;
    
    /**
     * @brief Get marker data for syncing
     * @param marker Pointer to the marker
     * @return BTWSyncMarkerData structure with marker's current state
     */
    BTWSyncMarkerData getMarkerData(InteractiveGraphicsItem *marker) const;
    
    /**
     * @brief Update a marker from sync data
     * @param markerData The updated marker data
     * @return true if marker was found and updated, false otherwise
     */
    bool updateMarkerFromData(const BTWSyncMarkerData &markerData);
    
    /**
     * @brief Remove a marker by its unique ID
     * @param id The unique identifier of the marker to remove
     * @return true if marker was found and removed, false otherwise
     */
    bool removeMarkerById(const QUuid &id);
    
    /**
     * @brief Check if a marker with the given ID exists
     * @param id The unique identifier to check
     * @return true if marker exists, false otherwise
     */
    bool hasMarker(const QUuid &id) const;

    /**
     * @brief Get all markers of a specific type
     * @param type Type of markers to retrieve
     * @return List of markers of the specified type
     */
    QList<InteractiveGraphicsItem*> getMarkers(MarkerType type) const;

    /**
     * @brief Get all markers
     * @return List of all markers
     */
    QList<InteractiveGraphicsItem*> getAllMarkers() const;

    /**
     * @brief Set styling for data point markers
     * @param pen Pen for data point markers
     * @param brush Brush for data point markers
     */
    void setDataPointStyle(const QPen &pen, const QBrush &brush);

    /**
     * @brief Set styling for reference line markers
     * @param pen Pen for reference line markers
     * @param brush Brush for reference line markers
     */
    void setReferenceLineStyle(const QPen &pen, const QBrush &brush);

    /**
     * @brief Set styling for annotation markers
     * @param pen Pen for annotation markers
     * @param brush Brush for annotation markers
     */
    void setAnnotationStyle(const QPen &pen, const QBrush &brush);

    /**
     * @brief Set styling for custom markers
     * @param pen Pen for custom markers
     * @param brush Brush for custom markers
     */
    void setCustomMarkerStyle(const QPen &pen, const QBrush &brush);

    /**
     * @brief Update the overlay (refresh display)
     */
    void updateOverlay();

    /**
     * @brief Get the overlay scene
     * @return Pointer to the overlay scene
     */
    QGraphicsScene* getOverlayScene() const { return m_overlayScene; }

    // ========== Marker Customization API ==========
    
    /**
     * @brief Set style for a specific marker
     * @param marker Pointer to the marker
     * @param markerColor Color of the marker circle
     * @param lineColor Color of the marker line (defaults to markerColor if not specified)
     * @param lineWidth Width of the line
     */
    void setMarkerStyle(InteractiveGraphicsItem *marker, 
                       const QColor &markerColor,
                       const QColor &lineColor = QColor(),
                       qreal lineWidth = 2.0);
    
    /**
     * @brief Set all markers to specified style
     * @param markerColor Color of all marker circles
     * @param lineColor Color of all marker lines (defaults to markerColor if not specified)
     * @param lineWidth Width of all lines
     */
    void setAllMarkersStyle(const QColor &markerColor,
                           const QColor &lineColor = QColor(),
                           qreal lineWidth = 2.0);
    
    /**
     * @brief Lock/unlock a specific marker
     * @param marker Pointer to the marker
     * @param locked True to lock the marker
     */
    void setMarkerLocked(InteractiveGraphicsItem *marker, bool locked);
    
    /**
     * @brief Lock/unlock all markers
     * @param locked True to lock all markers
     */
    void setAllMarkersLocked(bool locked);
    
    /**
     * @brief Set marker opacity
     * @param marker Pointer to the marker
     * @param opacity Opacity value (0.0 - 1.0)
     */
    void setMarkerOpacity(InteractiveGraphicsItem *marker, qreal opacity);
    
    /**
     * @brief Set all markers opacity
     * @param opacity Opacity value (0.0 - 1.0)
     */
    void setAllMarkersOpacity(qreal opacity);
    
    /**
     * @brief Get marker at position
     * @param position Scene position to check
     * @return Pointer to marker at position, or nullptr if none
     */
    InteractiveGraphicsItem* getMarkerAt(const QPointF &position) const;
    
    /**
     * @brief Select multiple markers
     * @param markers List of markers to select
     */
    void selectMarkers(const QList<InteractiveGraphicsItem*> &markers);
    
    /**
     * @brief Get list of selected markers
     * @return List of selected markers
     */
    QList<InteractiveGraphicsItem*> getSelectedMarkers() const;
    
    /**
     * @brief Clear marker selection
     */
    void clearSelection();
    
    /**
     * @brief Move selected markers by offset
     * @param offset Offset to move markers
     */
    void moveSelectedMarkers(const QPointF &offset);
    
    /**
     * @brief Delete selected markers
     */
    void deleteSelectedMarkers();
    
    /**
     * @brief Set movement constraints for a marker
     * @param marker Pointer to the marker
     * @param constrainX True to constrain X movement
     * @param constrainY True to constrain Y movement
     */
    void setMarkerConstraints(InteractiveGraphicsItem *marker, bool constrainX, bool constrainY);
    
    /**
     * @brief Set movement bounds for a marker
     * @param marker Pointer to the marker
     * @param bounds Rectangle defining valid movement area
     */
    void setMarkerBounds(InteractiveGraphicsItem *marker, const QRectF &bounds);
    
    /**
     * @brief Sync marker positions with timeline movement
     * 
     * Updates the Y position of all markers based on their stored timestamps
     * and the current time range of the graph. This should be called after
     * the graph redraws or time range changes to keep markers in sync.
     */
    void syncMarkersWithTimeline();
    
    /**
     * @brief Set horizontal-only movement for a marker
     * 
     * Constrains the marker to only move horizontally (along the same timestamp).
     * Vertical movement is blocked.
     * 
     * @param marker Pointer to the marker
     * @param horizontalOnly True to enable horizontal-only movement
     */
    void setMarkerHorizontalOnly(InteractiveGraphicsItem *marker, bool horizontalOnly);
    
    /**
     * @brief Set horizontal-only movement for all markers
     * @param horizontalOnly True to enable horizontal-only movement for all markers
     */
    void setAllMarkersHorizontalOnly(bool horizontalOnly);

signals:
    /**
     * @brief Emitted when a marker is added
     * @param marker Pointer to the added marker
     * @param type Type of the marker
     */
    void markerAdded(InteractiveGraphicsItem *marker, MarkerType type);

    /**
     * @brief Emitted when a marker is removed
     * @param marker Pointer to the removed marker
     * @param type Type of the marker
     */
    void markerRemoved(InteractiveGraphicsItem *marker, MarkerType type);

    /**
     * @brief Emitted when a marker is moved
     * @param marker Pointer to the moved marker
     * @param newPosition New position of the marker
     */
    void markerMoved(InteractiveGraphicsItem *marker, const QPointF &newPosition);

    /**
     * @brief Emitted when a marker is rotated
     * @param marker Pointer to the rotated marker
     * @param angle New rotation angle
     */
    void markerRotated(InteractiveGraphicsItem *marker, qreal angle);
    
    // ========== Sync Signals ==========
    
    /**
     * @brief Emitted when a marker's data changes and needs to be synced
     * 
     * This signal is emitted when a marker is created, moved, or its
     * bearing rate changes. Other containers can listen to this signal
     * to keep their markers in sync.
     * 
     * @param markerData The current state of the marker
     */
    void markerDataChanged(const BTWSyncMarkerData &markerData);
    
    /**
     * @brief Emitted when a marker is deleted and needs to be synced
     * @param markerId The unique ID of the deleted marker
     */
    void markerDeleted(const QUuid &markerId);

    /**
     * @brief Emitted when a marker is clicked
     * @param marker Pointer to the clicked marker
     * @param position Position of the click
     */
    void markerClicked(InteractiveGraphicsItem *marker, const QPointF &position);

private slots:
    /**
     * @brief Handle marker movement
     * @param newPosition New position of the marker
     */
    void onMarkerMoved(const QPointF &newPosition);

    /**
     * @brief Handle marker rotation
     * @param angle New rotation angle
     */
    void onMarkerRotated(qreal angle);

    /**
     * @brief Handle marker region click
     * @param region Interaction region that was clicked
     * @param position Position of the click
     */
    void onMarkerRegionClicked(int region, const QPointF &position);

private:
    // BTW graph reference
    BTWGraph *m_btwGraph;
    QGraphicsScene *m_overlayScene;

    // Marker storage
    QList<InteractiveGraphicsItem*> m_markers;
    QList<MarkerType> m_markerTypes;
    QMap<InteractiveGraphicsItem*, QList<QGraphicsItem*>> m_bearingRateItems; // Maps each marker to its bearing rate items
    QMap<InteractiveGraphicsItem*, QUuid> m_markerIds; // Maps markers to their unique IDs
    QMap<QUuid, InteractiveGraphicsItem*> m_idToMarker; // Reverse lookup: ID to marker
    QMap<InteractiveGraphicsItem*, qreal> m_previousRotation; // Maps each marker to its previous rotation value for direction detection
    QMap<InteractiveGraphicsItem*, QString> m_previousPrefix; // Maps each marker to its previous prefix (R/L) to maintain when no change

    // Styling
    QPen m_dataPointPen;
    QBrush m_dataPointBrush;
    QPen m_referenceLinePen;
    QBrush m_referenceLineBrush;
    QPen m_annotationPen;
    QBrush m_annotationBrush;
    QPen m_customMarkerPen;
    QBrush m_customMarkerBrush;

    // Helper methods
    void setupDefaultStyles();
    void connectMarkerSignals(InteractiveGraphicsItem *marker);
    void disconnectMarkerSignals(InteractiveGraphicsItem *marker);
    void updateBearingRateBox(InteractiveGraphicsItem *marker);
    void removeBearingRateBox(InteractiveGraphicsItem *marker);
    
    // Caching for bearing rate text rendering
    static QMap<QString, QPixmap> s_textPixmapCache;  // Static cache for all possible text values
    static QFont s_cachedFont;  // Font used for cached text
    static QPixmap getCachedTextPixmap(const QString &text);  // Get or create cached pixmap for text
};

#endif // BTWINTERACTIVEOVERLAY_H