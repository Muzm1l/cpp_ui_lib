#ifndef BTWINTERACTIVEOVERLAY_H
#define BTWINTERACTIVEOVERLAY_H

#include <QObject>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QPointF>
#include <QDateTime>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QList>
#include <QDebug>

// Forward declaration to avoid circular dependency
class BTWGraph;
class InteractiveGraphicsItem;

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
    InteractiveGraphicsItem* addDataPointMarker(const QPointF &position, const QDateTime &timestamp, qreal value, const QString &seriesLabel);

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
};

#endif // BTWINTERACTIVEOVERLAY_H