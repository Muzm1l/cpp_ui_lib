#ifndef WATERFALLGRAPH_EXAMPLE_H
#define WATERFALLGRAPH_EXAMPLE_H

#include "waterfallgraph.h"
#include <QPointF>

class WaterfallGraphExample : public waterfallgraph
{
    Q_OBJECT

public:
    explicit WaterfallGraphExample(QWidget* parent = nullptr);

protected:
    // Override the virtual mouse event handlers
    void onMouseClick(const QPointF& scenePos) override;
    void onMouseDrag(const QPointF& scenePos) override;
};

#endif // WATERFALLGRAPH_EXAMPLE_H
