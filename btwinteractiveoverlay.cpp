#include "btwinteractiveoverlay.h"
#include "btwgraph.h"
#include "interactivegraphicsitem.h"
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QDebug>

BTWInteractiveOverlay::BTWInteractiveOverlay(BTWGraph *btwGraph, QObject *parent)
    : QObject(parent)
    , m_btwGraph(btwGraph)
    , m_overlayScene(nullptr)
{
    // Get the overlay scene from the BTW graph
    if (m_btwGraph) {
        m_overlayScene = m_btwGraph->getOverlayScene();
        if (m_overlayScene) {
            qDebug() << "BTWInteractiveOverlay: Connected to overlay scene";
        } else {
            qDebug() << "BTWInteractiveOverlay: Warning - overlay scene is null";
        }
    } else {
        qDebug() << "BTWInteractiveOverlay: Warning - BTW graph is null";
    }

    // Setup default styles
    setupDefaultStyles();

    qDebug() << "BTWInteractiveOverlay created";
}

BTWInteractiveOverlay::~BTWInteractiveOverlay()
{
    clearAllMarkers();
    qDebug() << "BTWInteractiveOverlay destroyed";
}

InteractiveGraphicsItem* BTWInteractiveOverlay::addDataPointMarker(const QPointF &position, const QDateTime &timestamp, qreal value, const QString &seriesLabel)
{
    if (!m_overlayScene) {
        qDebug() << "BTWInteractiveOverlay: Cannot add marker - no overlay scene";
        return nullptr;
    }

    InteractiveGraphicsItem *marker = new InteractiveGraphicsItem();
    marker->setPos(position);
    marker->setSize(QSizeF(20, 20));

    // Set custom drawing function for data point - green version without interaction regions
    marker->setCustomDrawFunction([timestamp, value, seriesLabel](QPainter *painter, const QRectF &rect) {
        // Calculate marker radius based on the original size (20x20)
        qreal markerRadius = 10.0; // Half of the 20x20 size
        
        // Draw green circle outline (transparent fill) at the center of the item
        painter->setPen(QPen(Qt::green, 2));
        painter->setBrush(QBrush(Qt::transparent));
        QRectF circleRect(-markerRadius, -markerRadius, 2*markerRadius, 2*markerRadius);
        painter->drawEllipse(circleRect);
        
        // Draw angled line through center
        // Use a default angle for the interactive marker (0 degrees)
        qreal angleDegrees = 0.0; // Default angle for interactive marker
        qreal angleRadians = qDegreesToRadians(angleDegrees);
        qreal lineLength = 5 * markerRadius; // Same as BTW custom markers
        
        // Calculate line endpoints based on angle
        // For true north (0°), line points up/down (vertical)
        qreal deltaX = lineLength * qSin(angleRadians);
        qreal deltaY = -lineLength * qCos(angleRadians); // Negative because Y increases downward
        
        // Use the center of the item (0,0) as the reference point
        QPointF startPoint = QPointF(-deltaX, -deltaY);
        QPointF endPoint = QPointF(deltaX, deltaY);
        
        // Draw the angled line in green
        painter->setPen(QPen(Qt::green, 2));
        painter->drawLine(startPoint, endPoint);
    });

    // Apply styling and configure interaction regions
    marker->setDragRegionPen(m_dataPointPen);
    marker->setDragRegionBrush(m_dataPointBrush);
    marker->setShowDragRegion(false);  // Hide the drag region square
    marker->setShowRotateRegion(false); // Hide the rotate regions at line ends
    marker->setRotateRegionSize(QSizeF(12, 12)); // Set rotation regions to 12x12 pixels

    // Add to scene
    m_overlayScene->addItem(marker);
    m_markers.append(marker);
    m_markerTypes.append(DataPoint);

    // Connect signals
    connectMarkerSignals(marker);

    // Force scene update to ensure proper rendering
    if (m_overlayScene) {
        m_overlayScene->update();
    }

    qDebug() << "BTWInteractiveOverlay: Added data point marker at" << position << "for series" << seriesLabel;
    qDebug() << "BTWInteractiveOverlay: Marker bounding rect:" << marker->boundingRect();
    qDebug() << "BTWInteractiveOverlay: Marker scene pos:" << marker->pos();
    qDebug() << "BTWInteractiveOverlay: Marker scene bounding rect:" << marker->sceneBoundingRect();
    emit markerAdded(marker, DataPoint);

    return marker;
}

InteractiveGraphicsItem* BTWInteractiveOverlay::addReferenceLineMarker(const QPointF &startPos, const QPointF &endPos, const QString &label)
{
    if (!m_overlayScene) {
        qDebug() << "BTWInteractiveOverlay: Cannot add marker - no overlay scene";
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

    qDebug() << "BTWInteractiveOverlay: Added reference line marker from" << startPos << "to" << endPos;
    emit markerAdded(marker, ReferenceLine);

    return marker;
}

InteractiveGraphicsItem* BTWInteractiveOverlay::addAnnotationMarker(const QPointF &position, const QString &text, const QColor &color)
{
    if (!m_overlayScene) {
        qDebug() << "BTWInteractiveOverlay: Cannot add marker - no overlay scene";
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

    qDebug() << "BTWInteractiveOverlay: Added annotation marker at" << position << "with text:" << text;
    emit markerAdded(marker, Annotation);

    return marker;
}

InteractiveGraphicsItem* BTWInteractiveOverlay::addCustomMarker(const QPointF &position, const QSizeF &size)
{
    if (!m_overlayScene) {
        qDebug() << "BTWInteractiveOverlay: Cannot add marker - no overlay scene";
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

    qDebug() << "BTWInteractiveOverlay: Added custom marker at" << position << "with size" << size;
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
        
        // Disconnect signals
        disconnectMarkerSignals(marker);
        
        // Remove from scene
        if (m_overlayScene) {
            m_overlayScene->removeItem(marker);
        }
        
        // Remove from lists
        m_markers.removeAt(index);
        m_markerTypes.removeAt(index);
        
        // Delete marker
        delete marker;
        
        qDebug() << "BTWInteractiveOverlay: Removed marker of type" << type;
        emit markerRemoved(marker, type);
    }
}

void BTWInteractiveOverlay::clearAllMarkers()
{
    qDebug() << "BTWInteractiveOverlay: Clearing all markers";
    
    // Disconnect all signals and remove all markers
    for (InteractiveGraphicsItem *marker : m_markers) {
        disconnectMarkerSignals(marker);
        if (m_overlayScene) {
            m_overlayScene->removeItem(marker);
        }
        delete marker;
    }
    
    m_markers.clear();
    m_markerTypes.clear();
    
    // Force scene update to remove any drawing artifacts
    if (m_overlayScene) {
        m_overlayScene->update();
    }
    
    qDebug() << "BTWInteractiveOverlay: All markers cleared";
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
        emit markerMoved(senderMarker, senderMarker->pos());
    }
}

void BTWInteractiveOverlay::onMarkerRotated(qreal angle)
{
    Q_UNUSED(angle);
    // Find which marker was rotated and emit signal
    InteractiveGraphicsItem *senderMarker = qobject_cast<InteractiveGraphicsItem*>(sender());
    if (senderMarker) {
        emit markerRotated(senderMarker, senderMarker->rotation());
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