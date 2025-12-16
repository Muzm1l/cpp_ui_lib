#ifndef INTERACTIVEGRAPHICSITEM_H
#define INTERACTIVEGRAPHICSITEM_H

#include <QObject>
#include <QGraphicsItem>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QPointF>
#include <QRectF>
#include <QMouseEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QDebug>
#include <functional>

/**
 * @brief Interactive graphics item that supports dragging and rotation
 * 
 * This class provides a QGraphicsItem that can be dragged around a scene
 * and rotated using specific interaction regions. It supports custom drawing
 * functions and provides visual feedback for different interaction states.
 */
class InteractiveGraphicsItem : public QObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)

public:
    /**
     * @brief Interaction regions for the item
     */
    enum InteractionRegion {
        None,           ///< No interaction region
        DragRegion,     ///< Region for dragging the item
        RotateRegion    ///< Region for rotating the item
    };

    /**
     * @brief Constructor
     * @param parent Parent graphics item
     */
    explicit InteractiveGraphicsItem(QGraphicsItem *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~InteractiveGraphicsItem();

    // QGraphicsItem interface
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    /**
     * @brief Set the size of the item
     * @param size New size
     */
    void setSize(const QSizeF &size);

    /**
     * @brief Get the size of the item
     * @return Current size
     */
    QSizeF getSize() const { return m_size; }

    /**
     * @brief Set custom drawing function
     * @param drawFunction Function to call for custom drawing
     */
    void setCustomDrawFunction(std::function<void(QPainter*, const QRectF&)> drawFunction);

    /**
     * @brief Set drag region pen
     * @param pen Pen for drag region outline
     */
    void setDragRegionPen(const QPen &pen) { m_dragRegionPen = pen; }

    /**
     * @brief Set drag region brush
     * @param brush Brush for drag region fill
     */
    void setDragRegionBrush(const QBrush &brush) { m_dragRegionBrush = brush; }

    /**
     * @brief Set rotate region pen
     * @param pen Pen for rotate region outline
     */
    void setRotateRegionPen(const QPen &pen) { m_rotateRegionPen = pen; }

    /**
     * @brief Set rotate region brush
     * @param brush Brush for rotate region fill
     */
    void setRotateRegionBrush(const QBrush &brush) { m_rotateRegionBrush = brush; }

    /**
     * @brief Set rotate region size
     * @param size Size of the rotate regions
     */
    void setRotateRegionSize(const QSizeF &size) { m_rotateRegionSize = size; invalidateRotateRegionsCache(); updateInteractionRegions(); }

    /**
     * @brief Enable or disable drag functionality
     * @param enabled True to enable dragging
     */
    void setDragEnabled(bool enabled) { m_dragEnabled = enabled; }

    /**
     * @brief Enable or disable rotation functionality
     * @param enabled True to enable rotation
     */
    void setRotateEnabled(bool enabled) { m_rotateEnabled = enabled; }

    /**
     * @brief Show or hide drag region
     * @param show True to show drag region
     */
    void setShowDragRegion(bool show) { m_showDragRegion = show; }

    /**
     * @brief Show or hide rotate region
     * @param show True to show rotate region
     */
    void setShowRotateRegion(bool show) { m_showRotateRegion = show; }

    /**
     * @brief Get current interaction region at position
     * @param pos Position to check
     * @return Interaction region at position
     */
    InteractionRegion getInteractionRegion(const QPointF &pos) const;

    // ========== Marker Customization API ==========
    
    /**
     * @brief Set marker color (circle/shape color)
     * @param color The color to set
     */
    void setMarkerColor(const QColor &color);
    
    /**
     * @brief Get marker color
     * @return Current marker color
     */
    QColor getMarkerColor() const { return m_markerColor; }
    
    /**
     * @brief Set marker line color
     * @param color The color to set
     */
    void setLineColor(const QColor &color);
    
    /**
     * @brief Get marker line color
     * @return Current line color
     */
    QColor getLineColor() const { return m_lineColor; }
    
    /**
     * @brief Set marker opacity (0.0 - 1.0)
     * @param opacity The opacity value
     */
    void setMarkerOpacity(qreal opacity);
    
    /**
     * @brief Get marker opacity
     * @return Current opacity value
     */
    qreal getMarkerOpacity() const { return m_opacity; }
    
    /**
     * @brief Set marker line width
     * @param width The line width in pixels
     */
    void setLineWidth(qreal width);
    
    /**
     * @brief Get marker line width
     * @return Current line width
     */
    qreal getLineWidth() const { return m_lineWidth; }
    
    /**
     * @brief Set marker line style
     * @param style The Qt pen style
     */
    void setLineStyle(Qt::PenStyle style);
    
    /**
     * @brief Get marker line style
     * @return Current line style
     */
    Qt::PenStyle getLineStyle() const { return m_lineStyle; }
    
    /**
     * @brief Lock/unlock marker movement
     * @param locked True to lock the marker
     */
    void setLocked(bool locked);
    
    /**
     * @brief Check if marker is locked
     * @return True if marker is locked
     */
    bool isLocked() const { return m_locked; }
    
    /**
     * @brief Set constraints on movement
     * @param constrainX True to constrain X movement
     * @param constrainY True to constrain Y movement
     */
    void setMovementConstraints(bool constrainX, bool constrainY);
    
    /**
     * @brief Set movement bounds
     * @param bounds Rectangle defining valid movement area
     */
    void setMovementBounds(const QRectF &bounds);
    
    /**
     * @brief Get movement bounds
     * @return Current movement bounds
     */
    QRectF getMovementBounds() const { return m_movementBounds; }
    
    /**
     * @brief Check if marker is currently being dragged
     * @return True if dragging
     */
    bool isDragging() const { return m_isDragging; }
    
    /**
     * @brief Check if marker is currently being rotated
     * @return True if rotating
     */
    bool isRotating() const { return m_isRotating; }
    
    /**
     * @brief Set position while bypassing movement constraints
     * 
     * This is used for programmatic position updates (like timeline sync)
     * that should not be blocked by user movement constraints.
     * 
     * @param pos New position
     */
    void setPosWithoutConstraints(const QPointF &pos);

protected:
    // Override itemChange for movement constraints
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    // Mouse event handlers
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;

signals:
    /**
     * @brief Emitted when item is moved
     * @param newPosition New position of the item
     */
    void itemMoved(const QPointF &newPosition);

    /**
     * @brief Emitted when item is rotated
     * @param angle New rotation angle in degrees
     */
    void itemRotated(qreal angle);

    /**
     * @brief Emitted when a region is clicked
     * @param region The region that was clicked
     * @param position Position of the click
     */
    void regionClicked(InteractionRegion region, const QPointF &position);
    
    /**
     * @brief Emitted when marker color changes
     * @param newColor The new color
     */
    void colorChanged(const QColor &newColor);
    
    /**
     * @brief Emitted when marker opacity changes
     * @param newOpacity The new opacity
     */
    void opacityChanged(qreal newOpacity);
    
    /**
     * @brief Emitted when marker locked state changes
     * @param locked The new locked state
     */
    void lockedChanged(bool locked);
    
    /**
     * @brief Emitted when marker is selected
     */
    void markerSelected();
    
    /**
     * @brief Emitted when marker is deselected
     */
    void markerDeselected();

private:
    // Item properties
    QSizeF m_size;
    std::function<void(QPainter*, const QRectF&)> m_customDrawFunction;

    // Interaction regions
    QRectF m_dragRegion;
    QRectF m_rotateRegion;
    QSizeF m_rotateRegionSize;

    // Visual properties
    QPen m_dragRegionPen;
    QBrush m_dragRegionBrush;
    QPen m_rotateRegionPen;
    QBrush m_rotateRegionBrush;

    // Interaction state
    bool m_dragEnabled;
    bool m_rotateEnabled;
    bool m_showDragRegion;
    bool m_showRotateRegion;
    bool m_isDragging;
    bool m_isRotating;
    QPointF m_lastMousePos;
    qreal m_initialRotation;
    
    // Customization properties
    QColor m_markerColor;
    QColor m_lineColor;
    qreal m_opacity;
    Qt::PenStyle m_lineStyle;
    qreal m_lineWidth;
    bool m_locked;
    bool m_constrainX;
    bool m_constrainY;
    QRectF m_movementBounds;
    bool m_bypassConstraints;  // Flag to temporarily bypass constraints for programmatic updates

    // Helper methods
    void updateInteractionRegions();
    void updateCursor(InteractionRegion region);
    QRectF getRotateRegionRect() const;
    QList<QRectF> getRotateRegions() const;
    void invalidateRotateRegionsCache();
    
    // Rotate regions cache for performance optimization
    mutable QList<QRectF> m_cachedRotateRegions;
    mutable bool m_rotateRegionsCacheValid;
};

Q_DECLARE_METATYPE(InteractiveGraphicsItem*)

#endif // INTERACTIVEGRAPHICSITEM_H