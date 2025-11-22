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

private:
    // BRW-specific properties and methods can be added here
    void drawBRWScatterplot();
    void drawZeroAxis();
};

#endif // BRWGRAPH_H
