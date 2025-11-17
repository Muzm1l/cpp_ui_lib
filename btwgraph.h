#ifndef BTWGRAPH_H
#define BTWGRAPH_H

#include "waterfallgraph.h"
#include <QPushButton>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>

// Forward declarations to avoid circular dependency
class BTWInteractiveOverlay;
class InteractiveGraphicsItem;

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
    void drawCustomCircleMarkers(const QString &seriesLabel);
    
    // Interactive overlay setup
    void setupInteractiveOverlay();
    
    // Interactive overlay
    BTWInteractiveOverlay *m_interactiveOverlay;
};

#endif // BTWGRAPH_H
