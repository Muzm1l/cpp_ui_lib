#ifndef FDWGRAPH_H
#define FDWGRAPH_H

#include "waterfallgraph.h"

/**
 * @brief FDW Graph component that inherits from waterfallgraph
 *
 * This component creates scatterplots by default and can be extended
 * for specific FDW (Frequency Domain Waterfall) functionality.
 */
class FDWGraph : public WaterfallGraph
{
    Q_OBJECT

public:
    explicit FDWGraph(QWidget *parent = nullptr, bool enableGrid = false, int gridDivisions = 10, TimeInterval timeInterval = TimeInterval::FifteenMinutes);
    ~FDWGraph();

protected:
    // Override the draw method to create scatterplots by default
    void draw() override;

    // Override mouse event handlers if needed
    void onMouseClick(const QPointF &scenePos) override;
    void onMouseDrag(const QPointF &scenePos) override;

    // Overlay hooks invoked from WaterfallGraph::drawIncremental(). The base
    // versions are no-ops; we use them to keep the dashed white zero-axis line
    // in sync with the current Y range during live zoom / time-range updates,
    // since those paths go through drawIncremental(), not draw().
    void refreshOverlaysAfterVisibleTimeRangeChange() override;
    void augmentOverlayPassAfterSymbols() override;

    // FDW always shows a dashed middle line; magenta sync circles snap to it.
    bool magentaCircleOnMiddleLine() const override { return true; }

private:
    // FDW-specific properties and methods can be added here
    void drawFDWScatterplot();
    void drawZeroAxis();
    QGraphicsLineItem *m_zeroAxisLineItem;  // Store reference to zero axis line for updates
};

#endif // FDWGRAPH_H
