#ifndef BDWGRAPH_H
#define BDWGRAPH_H

#include "waterfallgraph.h"

/**
 * @brief BDW Graph component that inherits from waterfallgraph
 * 
 * This component creates scatterplots by default and can be extended
 * for specific BDW (Bandwidth Domain Waterfall) functionality.
 */
class BDWGraph : public WaterfallGraph
{
    Q_OBJECT

public:
    explicit BDWGraph(QWidget *parent = nullptr, bool enableGrid = false, int gridDivisions = 10, TimeInterval timeInterval = TimeInterval::FifteenMinutes);
    ~BDWGraph();

protected:
    // Override the draw method to create scatterplots by default
    void draw() override;
    
    // Override mouse event handlers if needed
    void onMouseClick(const QPointF& scenePos) override;
    void onMouseDrag(const QPointF& scenePos) override;

private:
    // BDW-specific properties and methods can be added here
    void drawBDWScatterplot();
};

#endif // BDWGRAPH_H
