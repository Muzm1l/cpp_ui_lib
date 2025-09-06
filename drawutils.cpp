#include "drawutils.h"

/**
 * @brief converts to bearing to radians
 *
 * @param bearing
 * @return double
 */
double DrawUtils::bearingtoRadians(double bearing)
{

    auto degree = (90.0f - bearing);

    return qDegreesToRadians(degree);
}

/**
 * @brief calculates the endpoint from a start point and the polar arguments
 *
 * @param startPoint
 * @param magnitude
 * @param bearing
 * @return QPointF
 */
QPointF DrawUtils::calculateEndpoint(QPointF startPoint, double magnitude, double bearing)
{
    // Convert bearing to radians
    double bearingRad = DrawUtils::bearingtoRadians(bearing);

    // Calculate endpoint
    double endX = startPoint.x() + magnitude * qCos(bearingRad);
    double endY = startPoint.y() - magnitude * qSin(bearingRad);

    return QPointF(endX, endY);
}

/// @brief Draw individual course vectors
/// @param scene
/// @param startPoint
/// @param magnitude
/// @param bearing
/// @param color
void DrawUtils::drawCourseVector(QGraphicsScene *scene, QPointF startPoint, double magnitude, double bearing, const QColor &color)
{

    qreal headLen = 5;
    qreal headAngleDeg = 30;

    QPen pen(color);
    QBrush brush(color);
    int radius = 5;

    scene->addEllipse(startPoint.x() - radius, startPoint.y() - radius, radius * 2, radius * 2, pen, brush); // Calulate endpoint

    auto endpoint = DrawUtils::calculateEndpoint(startPoint, magnitude, bearing);

    pen.setWidth(2);
    scene->addLine(QLineF(startPoint, endpoint), pen);

    // Calculate arrow head points
    qreal angle = qAtan2(endpoint.y() - startPoint.y(), endpoint.x() - startPoint.x());
    qreal a1 = angle + qDegreesToRadians(180.0 - headAngleDeg);
    qreal a2 = angle - qDegreesToRadians(180.0 - headAngleDeg);

    QPointF h1(endpoint.x() + headLen * qCos(a1), endpoint.y() + headLen * qSin(a1));
    QPointF h2(endpoint.x() + headLen * qCos(a2), endpoint.y() + headLen * qSin(a2));

    // Draw arrow head as filled polygon
    QPolygonF head;

    head << endpoint << h1 << h2;

    scene->addPolygon(head, pen, brush);
}

/**
 * @brief Converts bearing to cartesian points
 *
 * @param magnitude
 * @param bearing
 * @param window
 * @return QPointF
 */
QPointF DrawUtils::bearingToCartesian(qreal magnitude, qreal bearing, QRectF window)
{
    // Center coorindate
    QPointF center = QPointF(
        window.x() + window.width() / 2,
        window.y() + window.height() / 2);

    // Convert the bearing to a normal degree angle
    auto degree = 90 - bearing;

    // Convert to radians
    auto radians = qDegreesToRadians(degree);

    // Calculate the endpoint
    qreal x = center.x() + magnitude * qCos(radians);
    qreal y = center.y() - magnitude * qSin(radians);

    return QPointF(x, y);
}

/**
 * @brief Added the test pattern
 *
 * @param scene
 * @param rectToDraw
 */
void DrawUtils::addTestPattern(QGraphicsScene *scene, QRectF rectToDraw)
{
    if (!scene)
    {
        qDebug() << "No scene found, existing DrawUtils::addTestPattern";
        return;
    }

    QPen testPen(Qt::white, 1);
    testPen.setStyle(Qt::DashLine);

    // Draw test rectangle showing full bounds
    scene->addRect(rectToDraw, testPen);
}

/**
 * @brief Adds a test line to the scene
 * 
 * @param scene 
 * @param lineToDraw 
 */
void DrawUtils::addTestLine(QGraphicsScene *scene, QLineF lineToDraw)
{
    if (!scene)
    {
        qDebug() << "No scene found, existing DrawUtils::addTestPattern";
        return;
    }

    QPen testPen(Qt::white, 1);
    testPen.setStyle(Qt::DashLine);

    // Draw test rectangle showing full bounds
    scene->addLine(lineToDraw, testPen);
}

/**
 * @brief Draws the default test pattern
 * 
 * @param scene 
 */
void DrawUtils::drawDefaultTestPattern(QGraphicsScene *scene)
{
    if (!scene)
        return;

    QPen testPen(Qt::white, 1);
    testPen.setStyle(Qt::DashLine);

    // Draw test rectangle showing full bounds
    scene->addRect(scene->sceneRect(), testPen);

    // Draw diagonal line to show extent
    scene->addLine(0, 0, scene->sceneRect().width(), scene->sceneRect().height(), testPen);
}

/**
 * @brief Computes a transformation matrix to transform sourceRect to fit within targetRect.
 * The transformation will:
 * 1. Scale sourceRect so its largest dimension equals targetRect's smallest dimension
 * 2. Center the scaled sourceRect within targetRect
 *
 * @param sourceRect The rectangle to be transformed
 * @param targetRect The target rectangle to fit within
 * @return QTransform matrix that applies the required transformation
 */
QTransform DrawUtils::computeTransformationMatrix(const QRectF &sourceRect, const QRectF &targetRect)
{
// Handle degenerate cases
    if (sourceRect.isEmpty() || targetRect.isEmpty())
    {
        return QTransform(); // Return identity transform
    }

    // Get dimensions of both rectangles
    qreal sourceWidth = sourceRect.width();
    qreal sourceHeight = sourceRect.height();
    qreal targetWidth = targetRect.width();
    qreal targetHeight = targetRect.height();

    // Largest dimension of source vs smallest of target
    qreal sourceLargestDimension = std::max(sourceWidth, sourceHeight);
    qreal targetSmallestDimension = std::min(targetWidth, targetHeight);

    // Uniform scale factor
    qreal scaleFactor = targetSmallestDimension / sourceLargestDimension;
    qDebug() << "Scale Factor: " << scaleFactor;

    QPointF sourceCenter = sourceRect.center();
    QPointF targetCenter = targetRect.center();

    QTransform transform;
    // Step 1: move source center to origin
    transform.translate(-sourceCenter.x(), -sourceCenter.y());
    // Step 2: scale around origin
    // transform.scale(scaleFactor, scaleFactor);
    // Step 3: move to target center
    transform.translate(targetCenter.x(), targetCenter.y());

    return transform;}

/**
 * @brief Alternative version that returns the transformed rectangle for verification
 *
 * @param sourceRect
 * @param targetRect
 * @return QPair<QTransform, QRectF>
 */
QPair<QTransform, QRectF> DrawUtils::computeTransformationWithResult(const QRectF &sourceRect, const QRectF &targetRect)
{
    QTransform transform = computeTransformationMatrix(sourceRect, targetRect);
    QRectF transformedRect = transform.mapRect(sourceRect);

    return qMakePair(transform, transformedRect);
}

/**
 * @brief Transforms all the items in the scene
 *
 * @param scene
 * @param transform
 */
void DrawUtils::transformAllSceneItems(QGraphicsScene *scene, const QTransform &transform)
{
    if (!scene)
        return;

    // Get all items in the scene
    QList<QGraphicsItem *> items = scene->items();

    // Apply transformation to each item
    for (QGraphicsItem *item : items)
    {
        if (item)
        {
            // Option A: Set the item's transform directly
            item->setTransform(transform, true); // true = combine with existing transform

            // Option B: Or replace the existing transform completely
            // item->setTransform(transform, false);
        }
    }
}

/**
 * @brief Creates a line from a point and an angle
 * 
 * @param startPoint 
 * @param angleInDegrees 
 * @param length 
 * @return QGraphicsLineItem* 
 */
QGraphicsLineItem *DrawUtils::createLineFromPointAndAngle(const QPointF &startPoint,
                                                          qreal angleInDegrees,
                                                          qreal length)
{
    // Create line using QLineF's fromPolar method
    QLineF line = QLineF::fromPolar(length, angleInDegrees);

    // Translate to start point
    line.translate(startPoint);

    return new QGraphicsLineItem(line);
}

/**
 * @brief Computes the cartesian distance between two points
 * 
 * @param source 
 * @param target 
 * @return qreal 
 */
qreal DrawUtils::computeCartesianDistance(QPointF source, QPointF target)
{
    return sqrt(pow(target.x() - source.x(), 2) + pow(target.y() - source.y(), 2));
}

/**
 * @brief Flips the bearing
 * 
 * @param bearing 
 * @return qreal 
 */
qreal DrawUtils::flipBearing(qreal bearing)
{
    if (bearing < 180)
    {
        return bearing + 180;
    }
    else
    {
        return bearing - 180;
    }
}

/**
 * @brief Calculates the perpendicular distance from a point to a line
 * 
 * @param point 
 * @param linePoint1 
 * @param linePoint2 
 * @return qreal 
 */
qreal DrawUtils::calculatePerpendicularDistance(const QPointF &point, const QPointF &linePoint1, const QPointF &linePoint2)
{
    // Vector from linePoint1 to linePoint2
    double dx = linePoint2.x() - linePoint1.x();
    double dy = linePoint2.y() - linePoint1.y();

    // If the line points are the same, return distance to that point
    if (dx == 0 && dy == 0)
    {
        return qSqrt(qPow(point.x() - linePoint1.x(), 2) + qPow(point.y() - linePoint1.y(), 2));
    }

    // Using the formula: |ax + by + c| / sqrt(a² + b²)
    // Where the line equation is ax + by + c = 0
    // For a line through two points, we can derive:
    // a = (y2 - y1), b = (x1 - x2), c = (x2*y1 - x1*y2)

    double a = dy;
    double b = -dx;
    double c = linePoint2.x() * linePoint1.y() - linePoint1.x() * linePoint2.y();

    // Calculate perpendicular distance
    double distance = qAbs(a * point.x() + b * point.y() + c) / qSqrt(a * a + b * b);

    return distance;
}

/**
 * @brief Get all intersection points between a line and a rectangle
 *
 * @param line The line to check
 * @param rect The rectangle
 * @return QVector<QPointF> List of intersection points (0, 1, or 2 points)
 */
QVector<QPointF> DrawUtils::getLineRectIntersections(const QLineF &line, const QRectF &rect)
{
    QVector<QPointF> intersections;

    // Rectangle edges
    QLineF top(rect.topLeft(), rect.topRight());
    QLineF bottom(rect.bottomLeft(), rect.bottomRight());
    QLineF left(rect.topLeft(), rect.bottomLeft());
    QLineF right(rect.topRight(), rect.bottomRight());

    QPointF ip;

    auto checkIntersect = [&](const QLineF &edge)
    {
        QLineF::IntersectType type = line.intersects(edge, &ip);
        if (type == QLineF::BoundedIntersection)
        {
            intersections.append(ip);
        }
    };

    checkIntersect(top);
    checkIntersect(bottom);
    checkIntersect(left);
    checkIntersect(right);

    return intersections;
}

/**
 * @brief Split a rectangle into two polygons using a bisecting line
 *
 * @param line The line that cuts the rectangle
 * @param rect The rectangle to be split
 * @param poly1 Output: first polygon (as QVector<QPointF>)
 * @param poly2 Output: second polygon (as QVector<QPointF>)
 * @return true if successful (line intersects in 2 points), false otherwise
 */
bool DrawUtils::splitRectWithLine(const QLineF &line, const QRectF &rect,
                                  QVector<QPointF> &poly1, QVector<QPointF> &poly2)
{
    poly1.clear();
    poly2.clear();

    // Rectangle corners in CCW order
    QVector<QPointF> rectPts = {
        rect.topLeft(),
        rect.topRight(),
        rect.bottomRight(),
        rect.bottomLeft()};

    // Edges
    QVector<QLineF> edges = {
        QLineF(rect.topLeft(), rect.topRight()),
        QLineF(rect.topRight(), rect.bottomRight()),
        QLineF(rect.bottomRight(), rect.bottomLeft()),
        QLineF(rect.bottomLeft(), rect.topLeft())};

    QVector<QPointF> intersections;
    QPointF ip;

    // Collect intersections
    for (auto &edge : edges)
    {
        if (line.intersects(edge, &ip) == QLineF::BoundedIntersection)
        {
            // Avoid duplicates
            if (std::none_of(intersections.begin(), intersections.end(),
                             [&](const QPointF &p)
                             { return QLineF(p, ip).length() < 1e-6; }))
            {
                intersections.append(ip);
            }
        }
    }

    if (intersections.size() != 2)
    {
        return false; // Can't bisect if not exactly 2 intersections
    }

    // Insert intersection points into rectangle polygon
    QVector<QPointF> augmented;
    for (int i = 0; i < rectPts.size(); ++i)
    {
        QPointF a = rectPts[i];
        QPointF b = rectPts[(i + 1) % rectPts.size()];
        augmented.append(a);

        QLineF edge(a, b);
        for (auto &ip : intersections)
        {
            if (QLineF(a, ip).length() + QLineF(ip, b).length() - edge.length() < 1e-6)
            {
                augmented.append(ip); // Insert intersection into sequence
            }
        }
    }

    // Now walk augmented polygon to split into two
    int idx1 = augmented.indexOf(intersections[0]);
    int idx2 = augmented.indexOf(intersections[1]);

    if (idx1 == -1 || idx2 == -1)
        return false;

    // Polygon 1: walk from idx1 -> idx2
    for (int i = idx1; i != idx2; i = (i + 1) % augmented.size())
    {
        poly1.append(augmented[i]);
    }
    poly1.append(augmented[idx2]);

    // Polygon 2: walk from idx2 -> idx1
    for (int i = idx2; i != idx1; i = (i + 1) % augmented.size())
    {
        poly2.append(augmented[i]);
    }
    poly2.append(augmented[idx1]);

    return true;
}

/**
 * @brief Draws a shaded polygon
 * 
 * @param scene 
 * @param poly 
 * @param pen 
 * @param brush 
 */
void DrawUtils::drawShadedPolygon(QGraphicsScene *scene, QVector<QPointF> &poly, const QPen& pen, const QBrush& brush)
{
    if (!scene || poly.size() < 3) // must be at least a triangle
    {
        return;
    }

    QPolygonF polygon(poly);

    // Customize pen/brush as needed
    scene->addPolygon(polygon, pen, brush);
}

/**
 * @brief Cap the polar angle
 * 
 * @param angle 
 * @return qreal 
 */
qreal DrawUtils::capPolarAngle(qreal angle)
{
    if (angle < 0) {
        angle += 360.0;
    }
    while (angle >= 360.0) {
        angle -= 360.0;
    }
    return angle;
}
