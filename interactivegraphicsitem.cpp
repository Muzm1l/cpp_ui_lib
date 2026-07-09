#include "interactivegraphicsitem.h"
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QCursor>
#include <QApplication>
#include <cmath>
#include <QtMath>



InteractiveGraphicsItem::InteractiveGraphicsItem(QGraphicsItem *parent)
    : QObject(), QGraphicsItem(parent)
    , m_size(50.0, 50.0)
    , m_dragEnabled(true)
    , m_rotateEnabled(true)
    , m_showDragRegion(true)
    , m_showRotateRegion(true)
    , m_isDragging(false)
    , m_isRotating(false)
    , m_initialRotation(0.0)
    , m_rotateRegionSize(10.0, 10.0)
    // Initialize customization properties (manual marker colour: light pink)
    , m_markerColor(QColor(255, 182, 193))
    , m_lineColor(QColor(255, 182, 193))
    , m_opacity(1.0)
    , m_lineStyle(Qt::SolidLine)
    , m_lineWidth(2.0)
    , m_locked(false)
    , m_constrainX(false)
    , m_constrainY(false)
    , m_bypassConstraints(false)
    // Initialize rotate regions cache as invalid
    , m_rotateRegionsCacheValid(false)
{
    // Set default pens and brushes
    m_dragRegionPen = QPen(Qt::blue, 2, Qt::DashLine);
    m_dragRegionBrush = QBrush(Qt::transparent);
    m_rotateRegionPen = QPen(Qt::red, 2, Qt::SolidLine);
    m_rotateRegionBrush = QBrush(Qt::transparent);

    // Enable hover events for cursor changes
    setAcceptHoverEvents(true);
    // Enable item interaction flags for proper mouse event handling
    setFlag(QGraphicsItem::ItemIsMovable, false); // We handle movement ourselves with constraints
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsFocusable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
    
    // Accept mouse buttons for direct event handling
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);

    // Update interaction regions
    updateInteractionRegions();
}

InteractiveGraphicsItem::~InteractiveGraphicsItem()
{
}

QRectF InteractiveGraphicsItem::boundingRect() const
{
    // Calculate the full extent needed for the drawing including the line and rotation regions
    qreal markerRadius = qMin(m_size.width() / 2, m_size.height() / 2);
    
    // Calculate line endpoints (same as in getRotateRegions and custom drawing)
    qreal angleDegrees = 0.0; // Default angle for interactive marker
    qreal angleRadians = qDegreesToRadians(angleDegrees);
    qreal lineLength = 5 * markerRadius;
    
    qreal deltaX = lineLength * qSin(angleRadians);
    qreal deltaY = -lineLength * qCos(angleRadians);
    
    QPointF startPoint = QPointF(-deltaX, -deltaY);
    QPointF endPoint = QPointF(deltaX, deltaY);
    
    // Create rotation regions at both ends of the line
    QPointF region1TopLeft = startPoint - QPointF(m_rotateRegionSize.width()/2, m_rotateRegionSize.height()/2);
    QPointF region2TopLeft = endPoint - QPointF(m_rotateRegionSize.width()/2, m_rotateRegionSize.height()/2);
    
    QRectF region1(region1TopLeft, m_rotateRegionSize);
    QRectF region2(region2TopLeft, m_rotateRegionSize);
    
    // Find the bounding rectangle that includes the original circle, line, and both rotation regions
    QRectF baseRect(-m_size.width()/2, -m_size.height()/2, m_size.width(), m_size.height());
    QRectF fullRect = baseRect.united(region1).united(region2);
    
    // Add some padding to ensure we capture all mouse events
    qreal padding = 2.0;
    fullRect.adjust(-padding, -padding, padding, padding);
    
    return fullRect;
}

void InteractiveGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->save();

    // Draw custom content if function is provided
    if (m_customDrawFunction) {
        m_customDrawFunction(painter, boundingRect());
    } else {
        // Default drawing: gradient circle with center dot
        QRectF rect = boundingRect();
        
        // Create gradient
        QRadialGradient gradient(rect.center(), rect.width()/2);
        gradient.setColorAt(0, QColor(255, 100, 100, 200));
        gradient.setColorAt(1, QColor(200, 50, 50, 150));
        
        // Draw circle
        painter->setBrush(QBrush(gradient));
        painter->setPen(QPen(Qt::darkRed, 2));
        painter->drawEllipse(rect);
        
        // Draw center dot
        painter->setBrush(QBrush(Qt::white));
        painter->setPen(QPen(Qt::black, 1));
        painter->drawEllipse(rect.center(), 3, 3);
    }

    // Draw interaction regions if enabled
    if (m_showDragRegion && m_dragEnabled) {
        painter->setPen(m_dragRegionPen);
        painter->setBrush(m_dragRegionBrush);
        painter->drawRect(m_dragRegion);
    }

    if (m_showRotateRegion && m_rotateEnabled) {
        painter->setPen(m_rotateRegionPen);
        painter->setBrush(m_rotateRegionBrush);
        
        // Draw rotation regions at both ends of the line
        QList<QRectF> rotateRegions = getRotateRegions();
        for (const QRectF &rotateRect : rotateRegions) {
            painter->drawRect(rotateRect);
            
            // Draw rotation arrow
            painter->setPen(QPen(Qt::red, 1));
            QPointF center = rotateRect.center();
            QPointF arrowTip = center + QPointF(rotateRect.width()/3, -rotateRect.height()/3);
            painter->drawLine(center, arrowTip);
            
            // Draw arrow head
            QPointF arrowHead1 = arrowTip + QPointF(-3, 3);
            QPointF arrowHead2 = arrowTip + QPointF(-3, -3);
            painter->drawLine(arrowTip, arrowHead1);
            painter->drawLine(arrowTip, arrowHead2);
        }
    }

    painter->restore();
}

void InteractiveGraphicsItem::setSize(const QSizeF &size)
{
    if (m_size != size) {
        prepareGeometryChange();
        m_size = size;
        invalidateRotateRegionsCache();
        updateInteractionRegions();
        update();
    }
}

void InteractiveGraphicsItem::setCustomDrawFunction(std::function<void(QPainter*, const QRectF&)> drawFunction)
{
    m_customDrawFunction = drawFunction;
    update();
}

InteractiveGraphicsItem::InteractionRegion InteractiveGraphicsItem::getInteractionRegion(const QPointF &pos) const
{
    // Convert scene position to local coordinates
    QPointF localPos = mapFromScene(pos);
    
    // Check if the point is within the item's bounding rect
    if (!boundingRect().contains(localPos)) {
        return None;
    }
    
    // Check rotate regions first (they're smaller and more specific)
    if (m_rotateEnabled) {
        QList<QRectF> rotateRegions = getRotateRegions();
        for (int i = 0; i < rotateRegions.size(); ++i) {
            const QRectF &region = rotateRegions[i];
            if (region.contains(localPos)) {
                return RotateRegion;
            }
        }
    }
    
    // If not in rotate region but within the item, it's in the drag region
    if (m_dragEnabled) {
        return DragRegion;
    }
    
    return None;
}

void InteractiveGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // IMPORTANT: Accept the event to prevent propagation to parent items/views
        event->accept();
        
        m_lastMousePos = event->scenePos();
        InteractionRegion region = getInteractionRegion(event->scenePos());
        
        // Check if marker is locked
        if (m_locked) {
            emit regionClicked(region, event->scenePos());
            return;
        }
        
        if (region == DragRegion && m_dragEnabled) {
            m_isDragging = true;
            setCursor(Qt::ClosedHandCursor);
            // Grab mouse to receive all subsequent events
            grabMouse();
        } else if (region == RotateRegion && m_rotateEnabled) {
            m_isRotating = true;
            m_initialRotation = rotation();
            setCursor(Qt::SizeAllCursor);
            // Grab mouse to receive all subsequent events
            grabMouse();
        } else {
            // Emit click signal for any region
            emit regionClicked(region, event->scenePos());
        }
        return; // Don't call base class - we're handling the event completely
    }
    
    QGraphicsItem::mousePressEvent(event);
}

void InteractiveGraphicsItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    // Accept all move events when dragging or rotating
    if (m_isDragging || m_isRotating) {
        event->accept();
    }
    
    if (m_isDragging && m_dragEnabled && !m_locked) {
        QPointF delta = event->scenePos() - m_lastMousePos;
        
        // Apply movement constraints
        QPointF newPos = pos() + delta;
        if (m_constrainX) {
            newPos.setX(pos().x());  // Keep current X
        }
        if (m_constrainY) {
            newPos.setY(pos().y());  // Keep current Y
        }
        
        // Apply movement bounds if set
        if (!m_movementBounds.isNull() && m_movementBounds.isValid()) {
            newPos.setX(qBound(m_movementBounds.left(), newPos.x(), m_movementBounds.right()));
            newPos.setY(qBound(m_movementBounds.top(), newPos.y(), m_movementBounds.bottom()));
        }
        
        setPos(newPos);
        m_lastMousePos = event->scenePos();
        
        // Force update to prevent drawing artifacts
        update();
        
        // Also update the scene to ensure proper rendering
        if (scene()) {
            scene()->update();
        }
        
        emit itemMoved(pos());
        return; // Don't call base class
    } else if (m_isRotating && m_rotateEnabled && !m_locked) {
        QPointF center = sceneBoundingRect().center();
        QPointF mousePos = event->scenePos();
        
        // Calculate angle from center to mouse position
        QPointF delta = mousePos - center;
        qreal angle = qAtan2(delta.y(), delta.x()) * 180.0 / M_PI;
        
        // Set rotation (convert from atan2 angle to rotation angle)
        setRotation(angle + 90.0); // +90 to make 0 degrees point up
        
        // Force update to prevent drawing artifacts
        update();
        
        // Also update the scene to ensure proper rendering
        if (scene()) {
            scene()->update();
        }
        
        emit itemRotated(rotation());
        return; // Don't call base class
    }
    
    QGraphicsItem::mouseMoveEvent(event);
}

void InteractiveGraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        event->accept();
        
        // Release mouse grab
        ungrabMouse();
        
        if (m_isDragging) {
            m_isDragging = false;
            setCursor(Qt::OpenHandCursor);
            emit itemMoved(pos());
        } else if (m_isRotating) {
            m_isRotating = false;
            setCursor(Qt::ArrowCursor);
            emit itemRotated(rotation());
        }
        return; // Don't call base class
    }
    
    QGraphicsItem::mouseReleaseEvent(event);
}

void InteractiveGraphicsItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);
    InteractionRegion region = getInteractionRegion(event->scenePos());
    updateCursor(region);
    QGraphicsItem::hoverEnterEvent(event);
}

void InteractiveGraphicsItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);
    setCursor(Qt::ArrowCursor);
    QGraphicsItem::hoverLeaveEvent(event);
}

void InteractiveGraphicsItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    InteractionRegion region = getInteractionRegion(event->scenePos());
    updateCursor(region);
    QGraphicsItem::hoverMoveEvent(event);
}

void InteractiveGraphicsItem::updateInteractionRegions()
{
    // Drag region covers the entire item
    m_dragRegion = boundingRect();
    
    // Rotate region is in the bottom-right corner
    m_rotateRegion = getRotateRegionRect();
}

void InteractiveGraphicsItem::updateCursor(InteractionRegion region)
{
    switch (region) {
        case DragRegion:
            setCursor(Qt::OpenHandCursor);
            break;
        case RotateRegion:
            setCursor(Qt::OpenHandCursor); // Grab icon for hovering over rotation regions
            break;
        case None:
        default:
            setCursor(Qt::ArrowCursor);
            break;
    }
}

QRectF InteractiveGraphicsItem::getRotateRegionRect() const
{
    // Return the first rotation region (for backward compatibility)
    QList<QRectF> regions = getRotateRegions();
    return regions.isEmpty() ? QRectF() : regions.first();
}

QList<QRectF> InteractiveGraphicsItem::getRotateRegions() const
{
    // Performance optimization: use cached rotate regions if valid
    if (m_rotateRegionsCacheValid) {
        return m_cachedRotateRegions;
    }
    
    QList<QRectF> regions;
    qreal markerRadius = qMin(m_size.width() / 2, m_size.height() / 2);
    
    // Calculate line endpoints (same as in the custom drawing function and boundingRect)
    qreal angleDegrees = 0.0; // Default angle for interactive marker
    qreal angleRadians = qDegreesToRadians(angleDegrees);
    qreal lineLength = 5 * markerRadius;
    
    qreal deltaX = lineLength * qSin(angleRadians);
    qreal deltaY = -lineLength * qCos(angleRadians);
    
    // Use the center of the item (0,0) as the reference point
    QPointF startPoint = QPointF(-deltaX, -deltaY);
    QPointF endPoint = QPointF(deltaX, deltaY);
    
    // Create rotation regions at both ends of the line (region1 = tail, region2 = head)
    QPointF region1TopLeft = startPoint - QPointF(m_rotateRegionSize.width()/2, m_rotateRegionSize.height()/2);
    QPointF region2TopLeft = endPoint - QPointF(m_rotateRegionSize.width()/2, m_rotateRegionSize.height()/2);
    
    if (m_rotateEnd == HeadOnly) {
        // Only the head end (second region) can be used to rotate; tail is non-interactive
        regions.append(QRectF(region2TopLeft, m_rotateRegionSize));
    } else {
        regions.append(QRectF(region1TopLeft, m_rotateRegionSize));
        regions.append(QRectF(region2TopLeft, m_rotateRegionSize));
    }
    
    // Cache the result
    m_cachedRotateRegions = regions;
    m_rotateRegionsCacheValid = true;
    
    return regions;
}

void InteractiveGraphicsItem::invalidateRotateRegionsCache()
{
    m_rotateRegionsCacheValid = false;
    m_cachedRotateRegions.clear();
}

// ========== Customization API Implementation ==========

void InteractiveGraphicsItem::setMarkerColor(const QColor &color)
{
    if (m_markerColor != color) {
        m_markerColor = color;
        update();  // Trigger repaint
        emit colorChanged(color);
    }
}

void InteractiveGraphicsItem::setLineColor(const QColor &color)
{
    if (m_lineColor != color) {
        m_lineColor = color;
        update();  // Trigger repaint
        emit colorChanged(color);
    }
}

void InteractiveGraphicsItem::setMarkerOpacity(qreal opacity)
{
    opacity = qBound(0.0, opacity, 1.0);
    if (!qFuzzyCompare(m_opacity, opacity)) {
        m_opacity = opacity;
        setOpacity(opacity);  // Use Qt's built-in opacity
        emit opacityChanged(opacity);
    }
}

void InteractiveGraphicsItem::setLineWidth(qreal width)
{
    if (!qFuzzyCompare(m_lineWidth, width)) {
        m_lineWidth = width;
        update();  // Trigger repaint
    }
}

void InteractiveGraphicsItem::setLineStyle(Qt::PenStyle style)
{
    if (m_lineStyle != style) {
        m_lineStyle = style;
        update();  // Trigger repaint
    }
}

void InteractiveGraphicsItem::setLocked(bool locked)
{
    if (m_locked != locked) {
        m_locked = locked;
        emit lockedChanged(locked);
    }
}

void InteractiveGraphicsItem::setMovementConstraints(bool constrainX, bool constrainY)
{
    m_constrainX = constrainX;
    m_constrainY = constrainY;
}

void InteractiveGraphicsItem::setMovementBounds(const QRectF &bounds)
{
    m_movementBounds = bounds;
}

QVariant InteractiveGraphicsItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionChange && scene()) {
        QPointF newPos = value.toPointF();
        
        // Skip constraints if bypass flag is set (for programmatic updates like timeline sync)
        if (!m_bypassConstraints) {
            // Apply constraints (for user-initiated movements)
            if (m_constrainX) {
                newPos.setX(pos().x());
            }
            if (m_constrainY) {
                newPos.setY(pos().y());
            }
            
            // Apply bounds
            if (!m_movementBounds.isNull() && m_movementBounds.isValid()) {
                newPos.setX(qBound(m_movementBounds.left(), newPos.x(), m_movementBounds.right()));
                newPos.setY(qBound(m_movementBounds.top(), newPos.y(), m_movementBounds.bottom()));
            }
        }
        
        return newPos;
    }
    
    if (change == ItemSelectedHasChanged) {
        if (value.toBool()) {
            emit markerSelected();
        } else {
            emit markerDeselected();
        }
    }
    
    return QGraphicsItem::itemChange(change, value);
}

void InteractiveGraphicsItem::setPosWithoutConstraints(const QPointF &pos)
{
    // Temporarily bypass constraints for this position update
    m_bypassConstraints = true;
    setPos(pos);
    m_bypassConstraints = false;
}
