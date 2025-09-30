#ifndef RTWGRAPH_H
#define RTWGRAPH_H

#include "waterfallgraph.h"

/**
 * @brief RTW Graph component that inherits from waterfallgraph
 *
 * This component creates scatterplots by default and can be extended
 * for specific RTW (Rate Time Waterfall) functionality.
 */
class RTWGraph : public WaterfallGraph
{
    Q_OBJECT

public:
    explicit RTWGraph(QWidget *parent = nullptr, bool enableGrid = false, int gridDivisions = 10, TimeInterval timeInterval = TimeInterval::FifteenMinutes);
    ~RTWGraph();

protected:
    // Override the draw method to create scatterplots by default
    void draw() override;

    // Override mouse event handlers if needed
    void onMouseClick(const QPointF &scenePos) override;
    void onMouseDrag(const QPointF &scenePos) override;

private:
    // RTW-specific properties and methods can be added here
    void drawRTWScatterplot();
};

#endif // RTWGRAPH_H
