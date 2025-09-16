#ifndef WATERFALLGRAPH_H
#define WATERFALLGRAPH_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include "drawutils.h"

namespace Ui {
class waterfallgraph;
}

class waterfallgraph : public QWidget
{
    Q_OBJECT

public:
    explicit waterfallgraph(QWidget *parent = nullptr);
    ~waterfallgraph();

private:
    Ui::waterfallgraph *ui;
    QGraphicsView *graphicsView;
    QGraphicsScene *graphicsScene;
};

#endif // WATERFALLGRAPH_H
