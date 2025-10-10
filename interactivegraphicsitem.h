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
    void setRotateRegionSize(const QSizeF &size) { m_rotateRegionSize = size; updateInteractionRegions(); }

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

protected:
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

    // Helper methods
    void updateInteractionRegions();
    void updateCursor(InteractionRegion region);
    QRectF getRotateRegionRect() const;
    QList<QRectF> getRotateRegions() const;
};

#endif // INTERACTIVEGRAPHICSITEM_H