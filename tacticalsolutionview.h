#ifndef TACTICALSOLUTIONVIEW_H
#define TACTICALSOLUTIONVIEW_H

#include <QWidget>
#include <QPainter>
#include <QDebug>
#include <QGraphicsScene>
#include "drawutils.h"

namespace Ui
{
    class TacticalSolutionView;
}

class TacticalSolutionView : public QWidget
{
    Q_OBJECT

public:
    explicit TacticalSolutionView(QWidget *parent = nullptr);
    ~TacticalSolutionView();

    struct VectorPointPairs 
    {
        QPair<QPointF, QPointF> ownShipPoints;
        QPair<QPointF, QPointF> adoptedTrackPoints;
        QPair<QPointF, QPointF> selectedTrackPoints;
    };

protected:
    void paintEvent(QPaintEvent *event) override;
    // void resizeEvent(QResizeEvent *event) override;
    // void mouseMoveEvent(QMouseEvent *event) override;

private:
    // Drawing functions
    void draw();
    void drawBackground();
    void drawTestPattern();

    void drawVectors();
    void drawOwnShipVector(qreal ownShipSpeed, qreal ownShipBearing);
    void drawSelectedTrackVector(qreal sensorBearing, qreal selectedTrackRange, qreal selectedTrackBearing, qreal selectedTrackSpeed);
    void drawAdoptedTrackVector(qreal sensorBearing, qreal adoptedTrackRange,  qreal adoptedTrackBearing, qreal adoptedTrackSpeed);
    double getFarthestDistance(VectorPointPairs* pointStore, const QPointF& linePoint1, const QPointF& linePoint2);
    QPair<QLineF, QLineF> getOutlineLines(const QLineF& line, const qreal distance);

    QRectF getGuideBox(
        qreal ownShipSpeed,
        qreal ownShipBearing,
        qreal sensorBearing,
        qreal adoptedTrackRange,
        qreal adoptedTrackSpeed,
        qreal adoptedTrackBearing,
        qreal selectedTrackRange,
        qreal selectedTrackSpeed,
        qreal selectedTrackBearing,
        VectorPointPairs* pointStore
    );

    QRectF getZoomBoxFromGuideBox(const QRectF guidebox);

private:
    Ui::TacticalSolutionView *ui;
    QGraphicsScene *scene;
};

#endif // TACTICALSOLUTIONVIEW_H
