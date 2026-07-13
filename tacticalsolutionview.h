#ifndef TACTICALSOLUTIONVIEW_H
#define TACTICALSOLUTIONVIEW_H

#include "drawutils.h"
#include <QDebug>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPainter>

class TacticalSolutionView : public QGraphicsView
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

    void setOwnShipData(
        const qreal &ownShipSpeed,
        const qreal &ownShipBearing,
        const qreal &sensorBearing);

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
        const qreal &selectedTrackCourse,
        bool showSelectedTrack = true,
        bool showAdoptedTrack = true);

protected:
    // void resizeEvent(QResizeEvent *event) override;
    // void mouseMoveEvent(QMouseEvent *event) override;

private:
    // Drawing functions
    void draw();
    void drawCustomBackground();
    void drawTestPattern();

    void drawVectors();
    void drawOwnShipVector(qreal ownShipSpeed, qreal ownShipBearing);
    void drawSelectedTrackVector(qreal sensorBearing, qreal selectedTrackRange, qreal selectedTrackBearing, qreal selectedTrackSpeed, qreal selectedTrackCourse);
    void drawAdoptedTrackVector(qreal sensorBearing, qreal adoptedTrackRange, qreal adoptedTrackBearing, qreal adoptedTrackSpeed, qreal adoptedTrackCourse);
    void drawVectorsFromPointStore(const VectorPointPairs &pointStore);
    void drawCourseVectorFromEndpoints(const QPointF &startPoint, const QPointF &endPoint, const QColor &color, bool hollowTriangleMarker = false);
    void drawFixedOwnShipTriangle(const QPointF &anchor);
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
    void applyDataAndDraw(
        qreal ownShipSpeed,
        qreal adoptedTrackSpeed,
        qreal selectedTrackSpeed,
        qreal adoptedTrackRange,
        qreal selectedTrackRange);

private:
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
    bool showSelectedTrack;
    bool showAdoptedTrack;
    bool hasData;
};

#endif // TACTICALSOLUTIONVIEW_H
