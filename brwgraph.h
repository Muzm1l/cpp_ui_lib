#ifndef BRWGRAPH_H
#define BRWGRAPH_H

#include "waterfallgraph.h"

/**
 * @brief BRW Graph component that inherits from waterfallgraph
 *
 * This component creates scatterplots by default and can be extended
 * for specific BRW (Bit Rate Waterfall) functionality.
 */
class BRWGraph : public WaterfallGraph
{
    Q_OBJECT

public:
    explicit BRWGraph(QWidget *parent = nullptr, bool enableGrid = false, int gridDivisions = 10, TimeInterval timeInterval = TimeInterval::FifteenMinutes);
    ~BRWGraph();

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

    // BRW always shows a dashed middle line; magenta sync circles snap to it.
    bool magentaCircleOnMiddleLine() const override { return true; }

private:
    // BRW-specific properties and methods can be added here
    void drawBRWScatterplot();
    void drawZeroAxis();
    QGraphicsLineItem *m_zeroAxisLineItem;  // Store reference to zero axis line for updates
};

#endif // BRWGRAPH_H
