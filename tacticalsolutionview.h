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

    void setData(
        const qreal &ownShipSpeed,
        const qreal &ownShipBearing,
        const qreal &sensorBearing,
        const qreal &adoptedTrackRange,
        const qreal &adoptedTrackSpeed,
        const qreal &adoptedTrackBearing,
        const qreal &selectedTrackRange,
        const qreal &selectedTrackSpeed,
        const qreal &selectedTrackBearing,
        const qreal &adoptedTrackCourse,
        const qreal &selectedTrackCourse
    );

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
    void drawSelectedTrackVector(qreal sensorBearing, qreal selectedTrackRange, qreal selectedTrackBearing, qreal selectedTrackSpeed, qreal selectedTrackCourse);
    void drawAdoptedTrackVector(qreal sensorBearing, qreal adoptedTrackRange, qreal adoptedTrackBearing, qreal adoptedTrackSpeed, qreal adoptedTrackCourse);
    double getFarthestDistance(VectorPointPairs *pointStore, const QPointF &linePoint1, const QPointF &linePoint2);
    QPair<QLineF, QLineF> getOutlineLines(const QLineF &line, const qreal distance);

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
        qreal adoptedTrackCourse,
        qreal selectedTrackCourse,
        VectorPointPairs *pointStore);

    QRectF getZoomBoxFromGuideBox(const QRectF guidebox);
    
    // Utility function to normalize angles to 0-360 degree range
    qreal normalizeAngle(qreal angle);

private:
    Ui::TacticalSolutionView *ui;
    QGraphicsScene *scene;

    // Data stores for all the things rendered
    qreal ownShipSpeed;
    qreal ownShipBearing;
    qreal sensorBearing;
    qreal adoptedTrackRange;
    qreal adoptedTrackSpeed;
    qreal adoptedTrackBearing;
    qreal adoptedTrackCourse;
    qreal selectedTrackRange;
    qreal selectedTrackSpeed;
    qreal selectedTrackBearing;
    qreal selectedTrackCourse;
};

#endif // TACTICALSOLUTIONVIEW_H
