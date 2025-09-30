#ifndef FTWGRAPH_H
#define FTWGRAPH_H

#include "waterfallgraph.h"

/**
 * @brief FTW Graph component that inherits from waterfallgraph
 *
 * This component creates scatterplots by default and can be extended
 * for specific FTW (Frequency Time Waterfall) functionality.
 */
class FTWGraph : public WaterfallGraph
{
    Q_OBJECT

public:
    explicit FTWGraph(QWidget *parent = nullptr, bool enableGrid = false, int gridDivisions = 10, TimeInterval timeInterval = TimeInterval::FifteenMinutes);
    ~FTWGraph();

protected:
    // Override the draw method to create scatterplots by default
    void draw() override;

    // Override mouse event handlers if needed
    void onMouseClick(const QPointF &scenePos) override;
    void onMouseDrag(const QPointF &scenePos) override;

private:
    // FTW-specific properties and methods can be added here
    void drawFTWScatterplot();
};

#endif // FTWGRAPH_H
