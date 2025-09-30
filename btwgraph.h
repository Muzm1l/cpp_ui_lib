#ifndef BTWGRAPH_H
#define BTWGRAPH_H

#include "waterfallgraph.h"

/**
 * @brief BTW Graph component that inherits from waterfallgraph
 *
 * This component creates scatterplots by default and can be extended
 * for specific BTW (Bit Time Waterfall) functionality.
 */
class BTWGraph : public WaterfallGraph
{
    Q_OBJECT

public:
    explicit BTWGraph(QWidget *parent = nullptr, bool enableGrid = false, int gridDivisions = 10, TimeInterval timeInterval = TimeInterval::FifteenMinutes);
    ~BTWGraph();

protected:
    // Override the draw method to create scatterplots by default
    void draw() override;

    // Override mouse event handlers if needed
    void onMouseClick(const QPointF &scenePos) override;
    void onMouseDrag(const QPointF &scenePos) override;

private:
    // BTW-specific properties and methods can be added here
    void drawBTWScatterplot();
};

#endif // BTWGRAPH_H
