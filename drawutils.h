#ifndef DRAWUTILS_H
#define DRAWUTILS_H

#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QPointF>
#include <QRectF>
#include <QTransform>
#include <QtMath>
#include <algorithm>
#include <cmath>

using namespace std;

class DrawUtils
{

public:
    // QGraphicsItem* drawCourse(QPointF startPoint, QPointF endPoint);
    // QGraphicsPolygonItem* drawPolygon(const std::vector<QPointF>& points);
    // QGraphicsLineItem* drawBearingLine(QPointF origin, double magnitude);
    static double bearingtoRadians(double bearing);
    static QPointF calculateEndpoint(QPointF startPoint, double magnitude, double bearing);
    static void drawCourseVector(QGraphicsScene *scene, QPointF startPoint, double magnitude, double bearing, const QColor &color);
    static QPointF bearingToCartesian(qreal magnitude, qreal bearing, QRectF window);
    static void addTestPattern(QGraphicsScene *scene, QRectF rectToDraw);
    static void drawDefaultTestPattern(QGraphicsScene *scene);
    static QTransform computeTransformationMatrix(const QRectF &sourceRect, const QRectF &targetRect);
    static QPair<QTransform, QRectF> computeTransformationWithResult(const QRectF &sourceRect, const QRectF &targetRect);
    static void transformAllSceneItems(QGraphicsScene *scene, const QTransform &transform);
    static QGraphicsLineItem *createLineFromPointAndAngle(const QPointF &startPoint,
                                                          qreal angleInDegrees,
                                                          qreal length);
    static qreal computeCartesianDistance(QPointF source, QPointF target);
    static void addTestLine(QGraphicsScene *scene, QLineF lineToDraw);
    static qreal flipBearing(qreal bearing);
    static qreal calculatePerpendicularDistance(const QPointF &point, const QPointF &linePoint1, const QPointF &linePoint2);
    static QVector<QPointF> getLineRectIntersections(const QLineF &line, const QRectF &rect);
    static bool splitRectWithLine(const QLineF &line, const QRectF &rect,
                                  QVector<QPointF> &poly1, QVector<QPointF> &poly2);
    static void drawShadedPolygon(QGraphicsScene *scene, QVector<QPointF> &poly, const QPen &pen, const QBrush &brush);

    static qreal capPolarAngle(qreal angle);
};

#endif // DRAWUTILS_H
