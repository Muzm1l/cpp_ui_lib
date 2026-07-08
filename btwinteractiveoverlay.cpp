#include "btwinteractiveoverlay.h"
#include "btwgraph.h"
#include "interactivegraphicsitem.h"
#include "drawutils.h"
#include "sharedsyncstate.h"
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsTextItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPixmap>
#include <QVariant>

// Static cache initialization
QMap<QString, QPixmap> BTWInteractiveOverlay::s_textPixmapCache;
QFont BTWInteractiveOverlay::s_cachedFont;

BTWInteractiveOverlay::BTWInteractiveOverlay(BTWGraph *btwGraph, QObject *parent)
    : QObject(parent)
    , m_btwGraph(btwGraph)
    , m_overlayScene(nullptr)
{
    // Get the overlay scene from the BTW graph
    if (m_btwGraph) {
        m_overlayScene = m_btwGraph->getOverlayScene();
    }

    // Setup default styles
    setupDefaultStyles();
}

BTWInteractiveOverlay::~BTWInteractiveOverlay()
{
}

InteractiveGraphicsItem* BTWInteractiveOverlay::addDataPointMarker(const QPointF &position, const QDateTime &timestamp, float value, const QString &seriesLabel)
{
    if (!m_overlayScene) {
        return nullptr;
    }

    InteractiveGraphicsItem *marker = new InteractiveGraphicsItem();
    marker->setPos(position);
    marker->setSize(QSizeF(20, 20));

    // Set custom drawing function for data point (head/tail distinction: head = filled circle at one end)
    marker->setCustomDrawFunction([marker](QPainter *painter, const QRectF &rect) {
        Q_UNUSED(rect);
        
        // Get color settings from the marker (allows runtime customization)
        QColor markerColor = marker->getMarkerColor();
        QColor lineColor = marker->getLineColor();
        qreal lineWidth = marker->getLineWidth();
        Qt::PenStyle lineStyle = marker->getLineStyle();
        
        // Calculate marker radius based on the original size (20x20)
        qreal markerRadius = 10.0; // Half of the 20x20 size
        
        // Draw circle outline (transparent fill) at the center of the item
        painter->setPen(QPen(markerColor, lineWidth, lineStyle));
        painter->setBrush(QBrush(Qt::transparent));
        QRectF circleRect(-markerRadius, -markerRadius, 2*markerRadius, 2*markerRadius);
        painter->drawEllipse(circleRect);
        
        // Line from center: tail end (startPoint) and head end (endPoint)
        qreal angleDegrees = 0.0;
        qreal angleRadians = qDegreesToRadians(angleDegrees);
        qreal lineLength = 5 * markerRadius;
        qreal deltaX = lineLength * qSin(angleRadians);
        qreal deltaY = -lineLength * qCos(angleRadians);
        QPointF startPoint = QPointF(-deltaX, -deltaY);
        QPointF endPoint = QPointF(deltaX, deltaY);
        
        // Draw line (tail from center to startPoint, then center to head at endPoint)
        painter->setPen(QPen(lineColor, lineWidth, lineStyle));
        painter->drawLine(QPointF(0, 0), startPoint);  // tail
        painter->drawLine(QPointF(0, 0), endPoint);    // head stem
        
        // Draw head: filled circle at endPoint so head is visually distinct from tail
        qreal headRadius = 3.0;
        painter->setPen(QPen(lineColor, lineWidth));
        painter->setBrush(QBrush(lineColor));
        painter->drawEllipse(endPoint, headRadius, headRadius);
    });

    // Apply styling and configure interaction regions
    marker->setDragRegionPen(m_dataPointPen);
    marker->setDragRegionBrush(m_dataPointBrush);
    marker->setShowDragRegion(false);  // Hide the drag region square
    marker->setShowRotateRegion(false); // Hide the rotate regions at line ends
    marker->setRotateRegionSize(QSizeF(12, 12)); // Set rotation regions to 12x12 pixels
    marker->setRotateEnd(InteractiveGraphicsItem::HeadOnly); // Only head can rotate; tail is non-interactive

    // Store the timestamp in the marker for later retrieval
    // Use QGraphicsItem::setData() with key 0 to store the timestamp
    marker->setData(0, QVariant::fromValue(timestamp));
    
    // Store the initial range value for reference (key 1)
    marker->setData(1, QVariant::fromValue(value));
    // Source series this marker was snapped to (e.g. BTW-2), for diagnostics / future use
    marker->setData(3, seriesLabel);
    
    // Enable horizontal-only movement by default
    // This constrains the marker to move only along the X axis (range), not Y (timestamp)
    marker->setMovementConstraints(false, true);  // constrainX=false, constrainY=true

    // Add to scene
    m_overlayScene->addItem(marker);
    m_markers.append(marker);
    m_markerTypes.append(DataPoint);
    
    // Generate and store unique ID for this marker
    QUuid markerId = QUuid::createUuid();
    m_markerIds[marker] = markerId;
    m_idToMarker[markerId] = marker;
    marker->setData(2, QVariant::fromValue(markerId)); // Store ID in marker data as well (key 2)

    // Connect signals
    connectMarkerSignals(marker);

    // Force scene update to ensure proper rendering
    if (m_overlayScene) {
        m_overlayScene->update();
    }

    // Create bearing rate box for the marker
    updateBearingRateBox(marker);

    emit markerAdded(marker, DataPoint);
    
    // Emit marker data changed signal for sync
    BTWSyncMarkerData markerData;
    markerData.id = markerId;
    markerData.timestamp = timestamp;
    markerData.rangeValue = static_cast<qreal>(value);
    markerData.bearingRate = marker->rotation() / 10.0;
    markerData.isDeleted = false;
    emit markerDataChanged(markerData);

    return marker;
}

InteractiveGraphicsItem* BTWInteractiveOverlay::addReferenceLineMarker(const QPointF &startPos, const QPointF &endPos, const QString &label)
{
    if (!m_overlayScene) {
        return nullptr;
    }

    InteractiveGraphicsItem *marker = new InteractiveGraphicsItem();
    marker->setPos((startPos + endPos) / 2);
    marker->setSize(QSizeF(50, 20));

    // Set custom drawing function for reference line
    marker->setCustomDrawFunction([startPos, endPos, label](QPainter *painter, const QRectF &rect) {
        Q_UNUSED(rect);
        
        // Draw line
        painter->setPen(QPen(Qt::green, 3));
        painter->drawLine(startPos, endPos);
        
        // Draw label
        painter->setPen(QPen(Qt::green, 1));
        QFont font = painter->font();
        font.setPointSizeF(10.0);
        font.setBold(true);
        painter->setFont(font);
        painter->drawText(startPos + QPointF(5, -5), label);
    });

    // Apply styling
    marker->setDragRegionPen(m_referenceLinePen);
    marker->setDragRegionBrush(m_referenceLineBrush);

    // Add to scene
    m_overlayScene->addItem(marker);
    m_markers.append(marker);
    m_markerTypes.append(ReferenceLine);

    // Connect signals
    connectMarkerSignals(marker);

    emit markerAdded(marker, ReferenceLine);

    return marker;
}

InteractiveGraphicsItem* BTWInteractiveOverlay::addAnnotationMarker(const QPointF &position, const QString &text, const QColor &color)
{
    if (!m_overlayScene) {
        return nullptr;
    }

    InteractiveGraphicsItem *marker = new InteractiveGraphicsItem();
    marker->setPos(position);
    marker->setSize(QSizeF(80, 30));

    // Set custom drawing function for annotation
    marker->setCustomDrawFunction([text, color](QPainter *painter, const QRectF &rect) {
        // Draw text background
        painter->setBrush(QBrush(QColor(255, 255, 255, 200)));
        painter->setPen(QPen(color, 2));
        painter->drawRoundedRect(rect, 5, 5);
        
        // Draw text
        painter->setPen(QPen(color, 1));
        QFont font = painter->font();
        font.setPointSizeF(10.0);
        font.setBold(true);
        painter->setFont(font);
        painter->drawText(rect, Qt::AlignCenter, text);
    });

    // Apply styling
    marker->setDragRegionPen(m_annotationPen);
    marker->setDragRegionBrush(m_annotationBrush);

    // Add to scene
    m_overlayScene->addItem(marker);
    m_markers.append(marker);
    m_markerTypes.append(Annotation);

    // Connect signals
    connectMarkerSignals(marker);

    emit markerAdded(marker, Annotation);

    return marker;
}

InteractiveGraphicsItem* BTWInteractiveOverlay::addCustomMarker(const QPointF &position, const QSizeF &size)
{
    if (!m_overlayScene) {
        return nullptr;
    }

    InteractiveGraphicsItem *marker = new InteractiveGraphicsItem();
    marker->setPos(position);
    marker->setSize(size);

    // Apply styling
    marker->setDragRegionPen(m_customMarkerPen);
    marker->setDragRegionBrush(m_customMarkerBrush);

    // Add to scene
    m_overlayScene->addItem(marker);
    m_markers.append(marker);
    m_markerTypes.append(CustomMarker);

    // Connect signals
    connectMarkerSignals(marker);

    emit markerAdded(marker, CustomMarker);

    return marker;
}

void BTWInteractiveOverlay::removeMarker(InteractiveGraphicsItem *marker)
{
    if (!marker) {
        return;
    }

    int index = m_markers.indexOf(marker);
    if (index >= 0) {
        MarkerType type = m_markerTypes[index];
        
        // Get marker ID before deletion for sync signal
        QUuid markerId = getMarkerId(marker);
        
        // Remove bearing rate items
        removeBearingRateBox(marker);
        
        // Disconnect signals
        disconnectMarkerSignals(marker);
        
        // Remove from scene
        if (m_overlayScene) {
            m_overlayScene->removeItem(marker);
        }
        
        // Remove from ID maps
        if (m_markerIds.contains(marker)) {
            m_markerIds.remove(marker);
        }
        if (!markerId.isNull() && m_idToMarker.contains(markerId)) {
            m_idToMarker.remove(markerId);
        }
        
        // Remove previous rotation and prefix tracking
        m_previousRotation.remove(marker);
        m_previousPrefix.remove(marker);
        
        // Remove from lists
        m_markers.removeAt(index);
        m_markerTypes.removeAt(index);
        
        // Delete marker
        delete marker;
        
        emit markerRemoved(marker, type);
        
        // Emit sync signal for deletion
        if (!markerId.isNull()) {
            emit markerDeleted(markerId);
        }
    }
}

void BTWInteractiveOverlay::clearAllMarkers()
{
    // Disconnect all signals and remove all markers
    for (InteractiveGraphicsItem *marker : m_markers) {
        // Remove bearing rate items
        removeBearingRateBox(marker);
        
        disconnectMarkerSignals(marker);
        if (m_overlayScene) {
            m_overlayScene->removeItem(marker);
        }
        delete marker;
    }
    
    m_markers.clear();
    m_markerTypes.clear();
    m_bearingRateItems.clear();
    m_previousRotation.clear();  // Clear previous rotation tracking
    m_previousPrefix.clear();    // Clear previous prefix tracking
    
    // Force scene update to remove any drawing artifacts
    if (m_overlayScene) {
        m_overlayScene->update();
    }
}

QList<InteractiveGraphicsItem*> BTWInteractiveOverlay::getMarkers(MarkerType type) const
{
    QList<InteractiveGraphicsItem*> result;
    for (int i = 0; i < m_markers.size(); ++i) {
        if (m_markerTypes[i] == type) {
            result.append(m_markers[i]);
        }
    }
    return result;
}

QList<InteractiveGraphicsItem*> BTWInteractiveOverlay::getAllMarkers() const
{
    return m_markers;
}

void BTWInteractiveOverlay::setDataPointStyle(const QPen &pen, const QBrush &brush)
{
    m_dataPointPen = pen;
    m_dataPointBrush = brush;
    
    // Update existing data point markers
    for (int i = 0; i < m_markers.size(); ++i) {
        if (m_markerTypes[i] == DataPoint) {
            m_markers[i]->setDragRegionPen(pen);
            m_markers[i]->setDragRegionBrush(brush);
        }
    }
}

void BTWInteractiveOverlay::setReferenceLineStyle(const QPen &pen, const QBrush &brush)
{
    m_referenceLinePen = pen;
    m_referenceLineBrush = brush;
    
    // Update existing reference line markers
    for (int i = 0; i < m_markers.size(); ++i) {
        if (m_markerTypes[i] == ReferenceLine) {
            m_markers[i]->setDragRegionPen(pen);
            m_markers[i]->setDragRegionBrush(brush);
        }
    }
}

void BTWInteractiveOverlay::setAnnotationStyle(const QPen &pen, const QBrush &brush)
{
    m_annotationPen = pen;
    m_annotationBrush = brush;
    
    // Update existing annotation markers
    for (int i = 0; i < m_markers.size(); ++i) {
        if (m_markerTypes[i] == Annotation) {
            m_markers[i]->setDragRegionPen(pen);
            m_markers[i]->setDragRegionBrush(brush);
        }
    }
}

void BTWInteractiveOverlay::setCustomMarkerStyle(const QPen &pen, const QBrush &brush)
{
    m_customMarkerPen = pen;
    m_customMarkerBrush = brush;
    
    // Update existing custom markers
    for (int i = 0; i < m_markers.size(); ++i) {
        if (m_markerTypes[i] == CustomMarker) {
            m_markers[i]->setDragRegionPen(pen);
            m_markers[i]->setDragRegionBrush(brush);
        }
    }
}

void BTWInteractiveOverlay::updateOverlay()
{
    if (m_overlayScene) {
        m_overlayScene->update();
    }
}

void BTWInteractiveOverlay::onMarkerMoved(const QPointF &newPosition)
{
    Q_UNUSED(newPosition);
    // Find which marker was moved and emit signal
    InteractiveGraphicsItem *senderMarker = qobject_cast<InteractiveGraphicsItem*>(sender());
    if (senderMarker) {
        // Update bearing rate box position
        updateBearingRateBox(senderMarker);
        
        // Update the stored range value when marker is moved horizontally
        if (m_btwGraph) {
            qreal newRangeValue = m_btwGraph->mapScreenXToRange(senderMarker->scenePos().x());
            senderMarker->setData(1, QVariant::fromValue(newRangeValue));
        }
        
        emit markerMoved(senderMarker, senderMarker->pos());
        
        // Emit sync signal
        BTWSyncMarkerData markerData = getMarkerData(senderMarker);
        if (!markerData.id.isNull()) {
            emit markerDataChanged(markerData);
        }
    }
}

void BTWInteractiveOverlay::onMarkerRotated(qreal angle)
{
    Q_UNUSED(angle);
    // Find which marker was rotated and emit signal
    InteractiveGraphicsItem *senderMarker = qobject_cast<InteractiveGraphicsItem*>(sender());
    if (senderMarker) {
        // Update bearing rate box position (it should follow marker but not rotate)
        updateBearingRateBox(senderMarker);
        emit markerRotated(senderMarker, senderMarker->rotation());
        
        // Emit sync signal
        BTWSyncMarkerData markerData = getMarkerData(senderMarker);
        if (!markerData.id.isNull()) {
            emit markerDataChanged(markerData);
        }
    }
}

void BTWInteractiveOverlay::onMarkerRegionClicked(int region, const QPointF &position)
{
    Q_UNUSED(region);
    // Find which marker was clicked and emit signal
    InteractiveGraphicsItem *senderMarker = qobject_cast<InteractiveGraphicsItem*>(sender());
    if (senderMarker) {
        emit markerClicked(senderMarker, position);
    }
}

void BTWInteractiveOverlay::setupDefaultStyles()
{
    // Data point style (blue)
    m_dataPointPen = QPen(Qt::blue, 2, Qt::DashLine);
    m_dataPointBrush = QBrush(Qt::transparent);
    
    // Reference line style (green)
    m_referenceLinePen = QPen(Qt::green, 2, Qt::DashLine);
    m_referenceLineBrush = QBrush(Qt::transparent);
    
    // Annotation style (orange)
    m_annotationPen = QPen(Qt::darkYellow, 2, Qt::DashLine);
    m_annotationBrush = QBrush(Qt::transparent);
    
    // Custom marker style (red)
    m_customMarkerPen = QPen(Qt::red, 2, Qt::DashLine);
    m_customMarkerBrush = QBrush(Qt::transparent);
}

void BTWInteractiveOverlay::connectMarkerSignals(InteractiveGraphicsItem *marker)
{
    if (marker) {
        connect(marker, &InteractiveGraphicsItem::itemMoved,
                this, &BTWInteractiveOverlay::onMarkerMoved);
        connect(marker, &InteractiveGraphicsItem::itemRotated,
                this, &BTWInteractiveOverlay::onMarkerRotated);
        connect(marker, &InteractiveGraphicsItem::regionClicked,
                this, &BTWInteractiveOverlay::onMarkerRegionClicked);
    }
}

void BTWInteractiveOverlay::disconnectMarkerSignals(InteractiveGraphicsItem *marker)
{
    if (marker) {
        disconnect(marker, &InteractiveGraphicsItem::itemMoved,
                   this, &BTWInteractiveOverlay::onMarkerMoved);
        disconnect(marker, &InteractiveGraphicsItem::itemRotated,
                   this, &BTWInteractiveOverlay::onMarkerRotated);
        disconnect(marker, &InteractiveGraphicsItem::regionClicked,
                   this, &BTWInteractiveOverlay::onMarkerRegionClicked);
    }
}

void BTWInteractiveOverlay::updateBearingRateBox(InteractiveGraphicsItem *marker)
{
    if (!marker || !m_overlayScene) {
        return;
    }
    
    // Get marker position (use scene position for absolute coordinates)
    QPointF markerPos = marker->scenePos();
    qreal markerRadius = 10.0; // Match the marker radius in addDataPointMarker
    qreal currentRotation = marker->rotation(); // Get current rotation angle
    
    // Normalize rotation to 0-359 degrees (one full rotation = 360 values: 0 to 359)
    qreal normalizedRotation = currentRotation;
    while (normalizedRotation < 0) {
        normalizedRotation += 360.0;
    }
    while (normalizedRotation >= 360.0) {
        normalizedRotation -= 360.0;
    }
    
    // Determine prefix based on rotation direction (increasing = R, decreasing = L)
    QString prefix = "";
    if (normalizedRotation == 0) {
        prefix = "R";  // Default to R at 0 degrees
    } else {
        // Check if we have a previous rotation value to compare
        if (m_previousRotation.contains(marker)) {
            qreal prevRotation = m_previousRotation[marker];
            qreal normalizedPrev = prevRotation;
            while (normalizedPrev < 0) {
                normalizedPrev += 360.0;
            }
            while (normalizedPrev >= 360.0) {
                normalizedPrev -= 360.0;
            }
            
            // Calculate difference, handling wrap-around
            // Use a small epsilon to handle floating point precision issues
            qreal diff = normalizedRotation - normalizedPrev;
            
            // Handle very small differences (floating point precision) as no change
            if (qAbs(diff) < 0.001) {
                diff = 0.0;
            }
            
            // Handle wrap-around cases:
            // - If diff > 180: wrapped from 0→359 (anti-clockwise, decreasing) → L
            // - If diff < -180: wrapped from 359→0 (clockwise, increasing) → R
            // - Otherwise: normal case, use diff sign
            bool isWrapped = false;
            if (diff > 180.0) {
                // Wrapped from 0→359 (anti-clockwise, decreasing)
                diff = diff - 360.0;  // Now negative, indicating decrease
                isWrapped = true;
            } else if (diff < -180.0) {
                // Wrapped from 359→0 (clockwise, increasing)
                diff = diff + 360.0;  // Now positive, indicating increase
                isWrapped = true;
            }
            
            // Determine direction based on difference
            // R for increasing (clockwise: 0→359), L for decreasing (anti-clockwise: 359→0)
            if (diff > 0) {
                prefix = "R";  // Increasing (clockwise: 0→359)
            } else if (diff < 0) {
                prefix = "L";  // Decreasing (anti-clockwise: 359→0)
            } else {
                // No change, keep previous prefix if available
                if (m_previousPrefix.contains(marker)) {
                    prefix = m_previousPrefix[marker];  // Maintain previous prefix
                } else {
                    // First time with no change (rotation 0): default to R
                    prefix = "R";
                }
            }
        } else {
            // First time seeing this marker (rotation 0): default to R
            prefix = "R";
        }
        
        // Store current rotation and prefix for next comparison
        m_previousRotation[marker] = currentRotation;
        m_previousPrefix[marker] = prefix;
    }
    
    // Local display value: scale global rotation (0-360) to this graph's visible bearing range,
    // then add start zoom panel sticker so box shows same scale as stickers (e.g. 0-360 or 330-360).
    qreal visibleMin = 0.0, visibleMax = 360.0;
    if (m_btwGraph) {
        m_btwGraph->getVisibleBearingRange(visibleMin, visibleMax);
    }
    qreal visibleSpan = visibleMax - visibleMin;
    if (visibleSpan <= 0.0 || !qIsFinite(visibleSpan)) {
        visibleSpan = 360.0;
    }
    qreal localDisplayValue = normalizedRotation * (visibleSpan / 360.0);
    if (localDisplayValue < 0.0) {
        localDisplayValue = 0.0;
    } else if (localDisplayValue >= visibleSpan) {
        localDisplayValue = visibleSpan - 0.01;  // Keep below span for display
    }
    // Box value = start sticker + normalized offset (so A shows 0-360, B shows 330-360)
    qreal boxDisplayValue = visibleMin + localDisplayValue;
    
    // Format the display value to 2 decimals (sticker-scale: visibleMin to visibleMax)
    QString displayValue = QString::number(boxDisplayValue, 'f', 2);
    QString bearingRateText = prefix + displayValue;
    
    // Get cached pixmap for this text (L0-L359, R0-R359, or 0)
    QPixmap textPixmap = getCachedTextPixmap(bearingRateText);
    
    // Calculate text dimensions from cached pixmap
    QRectF textRect = textPixmap.rect();
    
    // Position text to the left of the marker (centered vertically)
    qreal textX = markerPos.x() - textRect.width() - markerRadius - 7;
    qreal textY = markerPos.y() - textRect.height() / 2;
    
    // Performance optimization: Reuse existing items instead of deleting and recreating
    QGraphicsPixmapItem *textLabel = nullptr;
    QGraphicsRectItem *textOutline = nullptr;
    
    if (m_bearingRateItems.contains(marker) && m_bearingRateItems[marker].size() >= 2) {
        // Reuse existing items
        textLabel = qgraphicsitem_cast<QGraphicsPixmapItem*>(m_bearingRateItems[marker].at(0));
        textOutline = qgraphicsitem_cast<QGraphicsRectItem*>(m_bearingRateItems[marker].at(1));
    }
    
    if (textLabel) {
        // Update existing pixmap item
        textLabel->setPixmap(textPixmap);
        textLabel->setPos(textX, textY);
    } else {
        // Create new pixmap item
        textLabel = new QGraphicsPixmapItem(textPixmap);
        textLabel->setPos(textX, textY);
        textLabel->setZValue(1002);
        m_overlayScene->addItem(textLabel);
    }
    
    if (textOutline) {
        // Update existing outline
        textOutline->setRect(textX - 2, textY + 1, textRect.width() + 6, textRect.height() + 4);
    } else {
        // Create new outline
        textOutline = new QGraphicsRectItem();
        textOutline->setRect(textX - 2, textY + 1, textRect.width() + 6, textRect.height() + 4);
        textOutline->setPen(QPen(QColor(255, 182, 193), 1));
        textOutline->setBrush(QBrush(Qt::transparent));
        textOutline->setZValue(1001);
        m_overlayScene->addItem(textOutline);
    }
    
    // Store/update the items reference
    if (!m_bearingRateItems.contains(marker)) {
        QList<QGraphicsItem*> items;
        items.append(textLabel);
        items.append(textOutline);
        m_bearingRateItems[marker] = items;
    }
}

QPixmap BTWInteractiveOverlay::getCachedTextPixmap(const QString &text)
{
    // Check if pixmap is already cached
    if (s_textPixmapCache.contains(text)) {
        return s_textPixmapCache[text];
    }
    
    // Initialize font if not already set
    if (s_cachedFont.pointSizeF() == -1) {
        s_cachedFont.setPointSizeF(8.0);
        s_cachedFont.setBold(true);
    }
    
    // Create pixmap for this text
    QFontMetrics fm(s_cachedFont);
    QRect textRect = fm.boundingRect(text);
    
    // Add some padding for better rendering
    QPixmap pixmap(textRect.width() + 4, textRect.height() + 4);
    pixmap.fill(Qt::transparent);
    
    // Draw text on pixmap
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setFont(s_cachedFont);
    painter.setPen(QColor(255, 182, 193));
    painter.drawText(2, textRect.height() + 2, text);
    painter.end();
    
    // Cache the pixmap
    s_textPixmapCache[text] = pixmap;
    
    return pixmap;
}

void BTWInteractiveOverlay::removeBearingRateBox(InteractiveGraphicsItem *marker)
{
    if (!marker || !m_overlayScene) {
        return;
    }
    
    // Remove stored bearing rate items from scene
    if (m_bearingRateItems.contains(marker)) {
        QList<QGraphicsItem*> items = m_bearingRateItems[marker];
        for (QGraphicsItem *item : items) {
            if (item) {
                m_overlayScene->removeItem(item);
                delete item;
            }
        }
        m_bearingRateItems.remove(marker);
    }
}

// ========== Marker Customization API Implementation ==========

void BTWInteractiveOverlay::setMarkerStyle(InteractiveGraphicsItem *marker, 
                                           const QColor &markerColor,
                                           const QColor &lineColor,
                                           qreal lineWidth)
{
    if (!marker) {
        return;
    }
    
    marker->setMarkerColor(markerColor);
    marker->setLineColor(lineColor.isValid() ? lineColor : markerColor);
    marker->setLineWidth(lineWidth);
    
    // Update the bearing rate box to match the new color
    updateBearingRateBox(marker);
}

void BTWInteractiveOverlay::setAllMarkersStyle(const QColor &markerColor,
                                               const QColor &lineColor,
                                               qreal lineWidth)
{
    for (InteractiveGraphicsItem *marker : m_markers) {
        setMarkerStyle(marker, markerColor, lineColor, lineWidth);
    }
}

void BTWInteractiveOverlay::setMarkerLocked(InteractiveGraphicsItem *marker, bool locked)
{
    if (!marker) {
        return;
    }
    
    marker->setLocked(locked);
}

void BTWInteractiveOverlay::setAllMarkersLocked(bool locked)
{
    for (InteractiveGraphicsItem *marker : m_markers) {
        marker->setLocked(locked);
    }
}

void BTWInteractiveOverlay::setMarkerOpacity(InteractiveGraphicsItem *marker, qreal opacity)
{
    if (!marker) {
        return;
    }
    
    marker->setMarkerOpacity(opacity);
}

void BTWInteractiveOverlay::setAllMarkersOpacity(qreal opacity)
{
    for (InteractiveGraphicsItem *marker : m_markers) {
        marker->setMarkerOpacity(opacity);
    }
}

InteractiveGraphicsItem* BTWInteractiveOverlay::getMarkerAt(const QPointF &position) const
{
    if (!m_overlayScene) {
        return nullptr;
    }
    
    // Walk stacking order: skip chrome (e.g. time-selection band) that may sit above markers in z.
    const QList<QGraphicsItem *> stack = m_overlayScene->items(position);
    for (QGraphicsItem *item : stack) {
        auto *marker = dynamic_cast<InteractiveGraphicsItem *>(item);
        if (marker && m_markers.contains(marker)) {
            return marker;
        }
    }
    return nullptr;
}

void BTWInteractiveOverlay::selectMarkers(const QList<InteractiveGraphicsItem*> &markers)
{
    // First clear existing selection
    clearSelection();
    
    // Select the specified markers
    for (InteractiveGraphicsItem *marker : markers) {
        if (marker && m_markers.contains(marker)) {
            marker->setSelected(true);
        }
    }
}

QList<InteractiveGraphicsItem*> BTWInteractiveOverlay::getSelectedMarkers() const
{
    QList<InteractiveGraphicsItem*> selectedMarkers;
    
    for (InteractiveGraphicsItem *marker : m_markers) {
        if (marker && marker->isSelected()) {
            selectedMarkers.append(marker);
        }
    }
    
    return selectedMarkers;
}

void BTWInteractiveOverlay::clearSelection()
{
    for (InteractiveGraphicsItem *marker : m_markers) {
        if (marker) {
            marker->setSelected(false);
        }
    }
}

void BTWInteractiveOverlay::moveSelectedMarkers(const QPointF &offset)
{
    QList<InteractiveGraphicsItem*> selectedMarkers = getSelectedMarkers();
    
    for (InteractiveGraphicsItem *marker : selectedMarkers) {
        if (marker && !marker->isLocked()) {
            marker->setPos(marker->pos() + offset);
            updateBearingRateBox(marker);
            emit markerMoved(marker, marker->pos());
        }
    }
}

void BTWInteractiveOverlay::deleteSelectedMarkers()
{
    QList<InteractiveGraphicsItem*> selectedMarkers = getSelectedMarkers();
    
    for (InteractiveGraphicsItem *marker : selectedMarkers) {
        removeMarker(marker);
    }
}

void BTWInteractiveOverlay::setMarkerConstraints(InteractiveGraphicsItem *marker, bool constrainX, bool constrainY)
{
    if (!marker) {
        return;
    }
    
    marker->setMovementConstraints(constrainX, constrainY);
}

void BTWInteractiveOverlay::setMarkerBounds(InteractiveGraphicsItem *marker, const QRectF &bounds)
{
    if (!marker) {
        return;
    }
    
    marker->setMovementBounds(bounds);
}

void BTWInteractiveOverlay::syncMarkersWithTimeline()
{
    if (!m_btwGraph) {
        return;
    }
    
    int markersUpdated = 0;
    
    for (InteractiveGraphicsItem *marker : m_markers) {
        if (!marker) continue;
        
        // Get the stored timestamp from the marker (key 0)
        QVariant timestampVariant = marker->data(0);
        if (!timestampVariant.isValid() || !timestampVariant.canConvert<QDateTime>()) {
            continue;
        }
        
        QDateTime timestamp = timestampVariant.value<QDateTime>();
        if (!timestamp.isValid()) {
            continue;
        }
        
        // Get the stored range value from the marker (key 1)
        QVariant rangeVariant = marker->data(1);
        qreal rangeValue = 0.0;
        if (rangeVariant.isValid() && rangeVariant.canConvert<qreal>()) {
            rangeValue = rangeVariant.value<qreal>();
        } else {
            // Fallback: calculate range from current X position (less accurate after zoom)
            qreal currentX = marker->pos().x();
            rangeValue = m_btwGraph->mapScreenXToRange(currentX);
        }
        
        // OPTIMIZATION: Use epoch milliseconds to avoid timezone conversion in mapDataToScreen
        // Convert timestamp once here instead of inside mapDataToScreen (avoids /etc/localtime reads)
        qint64 timestampEpoch = timestamp.toMSecsSinceEpoch();
        QPointF newScreenPos = m_btwGraph->mapDataToScreen(rangeValue, timestampEpoch);
        
        // Update both X and Y positions based on stored data values
        if (newScreenPos != marker->pos()) {
            // Use setPosWithoutConstraints to bypass constraints
            marker->setPosWithoutConstraints(newScreenPos);
            
            // Update the bearing rate box position
            updateBearingRateBox(marker);
            
            markersUpdated++;
        }
    }
    
    if (markersUpdated > 0) {
        // Force overlay scene update to ensure visual refresh
        if (m_overlayScene) {
            m_overlayScene->update();
        }
    }
}

void BTWInteractiveOverlay::setMarkerHorizontalOnly(InteractiveGraphicsItem *marker, bool horizontalOnly)
{
    if (!marker) {
        return;
    }
    
    // Constrain Y movement (vertical) while allowing X movement (horizontal)
    marker->setMovementConstraints(false, horizontalOnly);  // constrainX=false, constrainY=horizontalOnly
}

void BTWInteractiveOverlay::setAllMarkersHorizontalOnly(bool horizontalOnly)
{
    for (InteractiveGraphicsItem *marker : m_markers) {
        if (marker) {
            marker->setMovementConstraints(false, horizontalOnly);
        }
    }
}

// ========== Marker Sync Methods Implementation ==========

InteractiveGraphicsItem* BTWInteractiveOverlay::createMarkerFromData(const BTWSyncMarkerData &markerData)
{
    if (!m_overlayScene || !m_btwGraph) {
        return nullptr;
    }
    
    // Check if marker with this ID already exists
    if (m_idToMarker.contains(markerData.id)) {
        updateMarkerFromData(markerData);
        return m_idToMarker[markerData.id];
    }
    
    // Convert data coordinates to screen position
    QPointF screenPos = m_btwGraph->mapDataToScreen(markerData.rangeValue, markerData.timestamp);
    
    // Create the marker using similar logic to addDataPointMarker
    InteractiveGraphicsItem *marker = new InteractiveGraphicsItem();
    marker->setPos(screenPos);
    marker->setSize(QSizeF(20, 20));
    
    // Set custom drawing function (same as addDataPointMarker: head/tail distinction)
    marker->setCustomDrawFunction([marker](QPainter *painter, const QRectF &rect) {
        Q_UNUSED(rect);
        
        QColor markerColor = marker->getMarkerColor();
        QColor lineColor = marker->getLineColor();
        qreal lineWidth = marker->getLineWidth();
        Qt::PenStyle lineStyle = marker->getLineStyle();
        
        qreal markerRadius = 10.0;
        
        painter->setPen(QPen(markerColor, lineWidth, lineStyle));
        painter->setBrush(QBrush(Qt::transparent));
        QRectF circleRect(-markerRadius, -markerRadius, 2*markerRadius, 2*markerRadius);
        painter->drawEllipse(circleRect);
        
        qreal angleDegrees = 0.0;
        qreal angleRadians = qDegreesToRadians(angleDegrees);
        qreal lineLength = 5 * markerRadius;
        qreal deltaX = lineLength * qSin(angleRadians);
        qreal deltaY = -lineLength * qCos(angleRadians);
        QPointF startPoint = QPointF(-deltaX, -deltaY);
        QPointF endPoint = QPointF(deltaX, deltaY);
        
        painter->setPen(QPen(lineColor, lineWidth, lineStyle));
        painter->drawLine(QPointF(0, 0), startPoint);  // tail
        painter->drawLine(QPointF(0, 0), endPoint);    // head stem
        qreal headRadius = 3.0;
        painter->setPen(QPen(lineColor, lineWidth));
        painter->setBrush(QBrush(lineColor));
        painter->drawEllipse(endPoint, headRadius, headRadius);  // head
    });
    
    // Apply styling
    marker->setDragRegionPen(m_dataPointPen);
    marker->setDragRegionBrush(m_dataPointBrush);
    marker->setShowDragRegion(false);
    marker->setShowRotateRegion(false);
    marker->setRotateRegionSize(QSizeF(12, 12));
    marker->setRotateEnd(InteractiveGraphicsItem::HeadOnly); // Only head can rotate; tail non-interactive
    
    // Store data in marker
    marker->setData(0, QVariant::fromValue(markerData.timestamp));
    marker->setData(1, QVariant::fromValue(markerData.rangeValue));
    marker->setData(2, QVariant::fromValue(markerData.id));
    
    // Set bearing rate (rotation)
    marker->setRotation(markerData.bearingRate * 10.0);
    
    // Enable horizontal-only movement
    marker->setMovementConstraints(false, true);
    
    // Add to scene and storage
    m_overlayScene->addItem(marker);
    m_markers.append(marker);
    m_markerTypes.append(DataPoint);
    m_markerIds[marker] = markerData.id;
    m_idToMarker[markerData.id] = marker;
    
    // Connect signals
    connectMarkerSignals(marker);
    
    // Create bearing rate box
    updateBearingRateBox(marker);
    
    // Update scene
    if (m_overlayScene) {
        m_overlayScene->update();
    }
    
    emit markerAdded(marker, DataPoint);
    
    return marker;
}

InteractiveGraphicsItem* BTWInteractiveOverlay::findMarkerById(const QUuid &id) const
{
    if (m_idToMarker.contains(id)) {
        return m_idToMarker[id];
    }
    return nullptr;
}

QUuid BTWInteractiveOverlay::getMarkerId(InteractiveGraphicsItem *marker) const
{
    if (!marker) {
        return QUuid();
    }
    
    if (m_markerIds.contains(marker)) {
        return m_markerIds[marker];
    }
    
    // Try to get from marker data
    QVariant idVariant = marker->data(2);
    if (idVariant.isValid() && idVariant.canConvert<QUuid>()) {
        return idVariant.value<QUuid>();
    }
    
    return QUuid();
}

BTWSyncMarkerData BTWInteractiveOverlay::getMarkerData(InteractiveGraphicsItem *marker) const
{
    BTWSyncMarkerData data;
    
    if (!marker) {
        return data;
    }
    
    // Get ID
    data.id = getMarkerId(marker);
    
    // Get timestamp
    QVariant timestampVariant = marker->data(0);
    if (timestampVariant.isValid() && timestampVariant.canConvert<QDateTime>()) {
        data.timestamp = timestampVariant.value<QDateTime>();
    }
    
    // Prefer stored range (data 1); it matches snapped data coordinates. Fall back to screen X mapping.
    QVariant rangeVariant = marker->data(1);
    if (rangeVariant.isValid() && rangeVariant.canConvert<double>()) {
        data.rangeValue = rangeVariant.toDouble();
    } else if (m_btwGraph) {
        data.rangeValue = m_btwGraph->mapScreenXToRange(marker->scenePos().x());
    }
    
    // Get bearing rate from rotation
    data.bearingRate = marker->rotation() / 10.0;
    
    data.isDeleted = false;
    
    return data;
}

bool BTWInteractiveOverlay::updateMarkerFromData(const BTWSyncMarkerData &markerData)
{
    InteractiveGraphicsItem *marker = findMarkerById(markerData.id);
    if (!marker) {
        return false;
    }
    
    if (!m_btwGraph) {
        return false;
    }
    
    // Convert data coordinates to screen position
    QPointF screenPos = m_btwGraph->mapDataToScreen(markerData.rangeValue, markerData.timestamp);
    
    // Update position without constraints
    marker->setPosWithoutConstraints(screenPos);
    
    // Update rotation (bearing rate)
    marker->setRotation(markerData.bearingRate * 10.0);
    
    // Update stored data
    marker->setData(0, QVariant::fromValue(markerData.timestamp));
    marker->setData(1, QVariant::fromValue(markerData.rangeValue));
    
    // Update bearing rate box
    updateBearingRateBox(marker);
    
    // Update scene
    if (m_overlayScene) {
        m_overlayScene->update();
    }
    
    return true;
}

bool BTWInteractiveOverlay::removeMarkerById(const QUuid &id)
{
    InteractiveGraphicsItem *marker = findMarkerById(id);
    if (!marker) {
        return false;
    }
    
    // Remove from ID maps
    m_markerIds.remove(marker);
    m_idToMarker.remove(id);
    
    // Remove using existing method (handles bearing rate box, scene, etc.)
    removeMarker(marker);
    
    return true;
}

bool BTWInteractiveOverlay::hasMarker(const QUuid &id) const
{
    return m_idToMarker.contains(id) && m_idToMarker[id] != nullptr;
}
