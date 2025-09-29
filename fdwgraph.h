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
    void onMouseClick(const QPointF& scenePos) override;
    void onMouseDrag(const QPointF& scenePos) override;

private:
    // FDW-specific properties and methods can be added here
    void drawFDWScatterplot();
};

#endif // FDWGRAPH_H
