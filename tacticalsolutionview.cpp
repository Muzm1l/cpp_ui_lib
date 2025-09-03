#include "tacticalsolutionview.h"
#include "ui_tacticalsolutionview.h"

const qreal SPEED_NORMALIZATION_FACTOR = 1;
const qreal ZOOMBOX_EXPANSION_FACTOR = 1.2;

/**
 * @brief Construct a new Tactical Solution View:: Tactical Solution View object
 *
 * @param parent
 */
TacticalSolutionView::TacticalSolutionView(QWidget *parent)
    : QWidget(parent), ui(new Ui::TacticalSolutionView)
{
    ui->setupUi(this);

    // Set black background
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::black);
    setPalette(pal);
    setAutoFillBackground(true);

    // Set minimum size
    setMinimumSize(226, 240);

    // Initialize scene
    scene = new QGraphicsScene(this);
    scene->setSceneRect(0, 0, width(), height());

    // Make sure widget expands
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Enable mouse tracking
    // setMouseTracking(true);
}

/**
 * @brief Destroy the Tactical Solution View:: Tactical Solution View object
 *
 */
TacticalSolutionView::~TacticalSolutionView()
{
    delete ui;
}

/**
 * @brief Draw the entire graph.
 *
 */
void TacticalSolutionView::draw()
{
    if (!scene)
        return;

    // Clear existing items
    scene->clear();

    // Update scene rect to match widget size
    scene->setSceneRect(0, 0, width(), height());

    // Draw in layers from back to front
    drawBackground();
    // DrawUtils::drawDefaultTestPattern(scene);

    drawVectors();

    qDebug() << "Draw completed - Scene rect:" << scene->sceneRect();
}

/**
 * @brief Draw the background for the graph.
 *
 */
void TacticalSolutionView::drawBackground()
{
    if (!scene)
        return;

    // Draw the background - currently empty as we use widget background
}

/**
 * @brief Handle paint events for the graph.
 *
 * @param event
 */
void TacticalSolutionView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (scene)
    {
        // Draw all elements
        draw();

        // Render the scene to the widget
        scene->render(&painter, rect(), scene->sceneRect());
    }

    qDebug() << "Paint event - Widget size:" << width() << "x" << height();
}

/// @brief Draws the various vectors
/// @param ownShipPosition
/// @param selectedTrackPosition
/// @param adoptedPosition
void TacticalSolutionView::drawVectors()
{

    if (!scene)
    {
        return;
    }


    // Create store for all the Vector Points
    VectorPointPairs pointStore;

    // Draw the ownship vector
    drawOwnShipVector(ownShipSpeed, ownShipBearing);

    // Draw the selected track vector
    drawSelectedTrackVector(sensorBearing, selectedTrackRange, selectedTrackBearing, selectedTrackSpeed);

    // Adopted track vector
    drawAdoptedTrackVector(sensorBearing, adoptedTrackRange, adoptedTrackBearing, adoptedTrackSpeed);

    // Get guidebox
    QRectF guidebox = getGuideBox(
        ownShipSpeed,
        ownShipBearing,
        sensorBearing,
        adoptedTrackRange,
        adoptedTrackSpeed,
        adoptedTrackBearing,
        selectedTrackRange,
        selectedTrackSpeed,
        selectedTrackBearing,
        &pointStore);

    QRectF zoomBox = getZoomBoxFromGuideBox(guidebox);

    qDebug() << "Guide Box: width: " << guidebox.width() << ", height: " << guidebox.height();
    qDebug() << "Zoom Box: width:" << zoomBox.width() << ", height: " << zoomBox.height();

    // DrawUtils::addTestPattern(scene, guidebox);
    // DrawUtils::addTestPattern(scene, zoomBox);

    // Compute the transformation to adjust the zooming and panning
    QTransform zoomtransform = DrawUtils::computeTransformationMatrix(zoomBox, scene->sceneRect());
    qDebug() << "Scene Box: width:" << scene->sceneRect().width() << ", height: " << scene->sceneRect().height();

    // Draw test line that goes through a point for a given angle
    // Get the largest distance
    auto sceneRect = scene->sceneRect();
    auto bearingLineRange = std::max(sceneRect.width(), sceneRect.height());
    QPointF ownShipPosition = DrawUtils::bearingToCartesian(0, 0, scene->sceneRect());

    // Use this to draw the bearings line and the bisection ref line
    auto p1 = DrawUtils::calculateEndpoint(ownShipPosition, bearingLineRange * 5, sensorBearing);
    auto p2 = DrawUtils::calculateEndpoint(ownShipPosition, bearingLineRange * 5, DrawUtils::flipBearing(sensorBearing));

    // Draw the bearing line
    QPen bearingPen(Qt::green, 2);

    scene->addLine(QLineF(ownShipPosition, p1), bearingPen);

    // Draw a refernce Line
    // DrawUtils::addTestLine(scene, QLineF(p1, p2));
    // Draw the Bearing Line

    // Get the farthest perpedicular point from the line
    auto distance = getFarthestDistance(&pointStore, p1, p2);

    // Generate the perpendicular lines
    auto lines = getOutlineLines(QLineF(p1, p2), distance);

    // DrawUtils::addTestLine(scene, lines.first);

    // DrawUtils::addTestLine(scene, lines.second);

    QPointF ownShipEnd = pointStore.ownShipPoints.second;

    // Decide which outline line is closer to ownship end
    qreal d1 = DrawUtils::calculatePerpendicularDistance(ownShipEnd, lines.first.p1(), lines.first.p2());
    qreal d2 = DrawUtils::calculatePerpendicularDistance(ownShipEnd, lines.second.p1(), lines.second.p2());

    QLineF chosenLine = (d1 < d2) ? lines.first : lines.second;
    QLineF oppositeLine = (d1 < d2) ? lines.second : lines.first;

    qDebug() << "Chosen Line: " << chosenLine;
    qDebug() << "Opposite Line: " << oppositeLine;

    // Apply the transform
    DrawUtils::transformAllSceneItems(scene, zoomtransform);

    // Transform stored points
    pointStore.ownShipPoints.first = zoomtransform.map(pointStore.ownShipPoints.first);
    pointStore.ownShipPoints.second = zoomtransform.map(pointStore.ownShipPoints.second);
    pointStore.selectedTrackPoints.first = zoomtransform.map(pointStore.selectedTrackPoints.first);
    pointStore.selectedTrackPoints.second = zoomtransform.map(pointStore.selectedTrackPoints.second);
    pointStore.adoptedTrackPoints.first = zoomtransform.map(pointStore.adoptedTrackPoints.first);
    pointStore.adoptedTrackPoints.second = zoomtransform.map(pointStore.adoptedTrackPoints.second);

    // Transform the lines
    oppositeLine = zoomtransform.map(oppositeLine);

    std::vector<QPointF> shadedPolygon;

    // We find the intersection between the parallel line that is the
    // opposite of the ownShip course direction, we can calculate
    // this by seeing which of the distances is the closest the endpoint
    // of the ownship vector to the outline lines

    QVector<QPointF> halfA, halfB;

    DrawUtils::splitRectWithLine(oppositeLine, scene->sceneRect(), halfA, halfB);

    QPen pen(Qt::white);
    pen.setWidth(2);

    QBrush brush(Qt::white, Qt::BDiagPattern);
    // check if halfA or halfB is within the polygon
    if (QPolygonF(halfA).containsPoint(ownShipEnd, Qt::OddEvenFill))
    {
        // Shade Half B
        DrawUtils::drawShadedPolygon(scene, halfB, pen, brush);
    }
    else
    {
        // Shade Half A
        DrawUtils::drawShadedPolygon(scene, halfA, pen, brush);
    }
}

/**
 * @brief Returns two parallel lines as offset lines
 *
 * @param line The original line to create parallel lines from
 * @param distance The perpendicular distance from the original line
 * @return QPair<QLineF, QLineF> Two parallel lines, one on each side of the original
 */
QPair<QLineF, QLineF> TacticalSolutionView::getOutlineLines(const QLineF &line, const qreal distance)
{
    // Calculate the direction vector of the line
    QPointF direction = line.p2() - line.p1();

    // Calculate the length of the direction vector
    qreal length = std::sqrt(direction.x() * direction.x() + direction.y() * direction.y());

    // Handle degenerate case where line has zero length
    if (qFuzzyIsNull(length))
    {
        return QPair<QLineF, QLineF>(line, line);
    }

    // Normalize the direction vector
    QPointF unitDirection = QPointF(direction.x() / length, direction.y() / length);

    // Calculate the perpendicular vector (rotate 90 degrees counterclockwise)
    QPointF perpendicular = QPointF(-unitDirection.y(), unitDirection.x());

    // Scale the perpendicular vector by the desired distance
    // Since the caller passes distance/2, this creates lines at the correct offset
    QPointF offset = QPointF(perpendicular.x() * distance, perpendicular.y() * distance);

    // Create the first parallel line (offset in positive perpendicular direction)
    QPointF p1_offset_pos = line.p1() + offset;
    QPointF p2_offset_pos = line.p2() + offset;
    QLineF line1(p1_offset_pos, p2_offset_pos);

    // Create the second parallel line (offset in negative perpendicular direction)
    QPointF p1_offset_neg = line.p1() - offset;
    QPointF p2_offset_neg = line.p2() - offset;
    QLineF line2(p1_offset_neg, p2_offset_neg);

    return QPair<QLineF, QLineF>(line1, line2);
}

double TacticalSolutionView::getFarthestDistance(VectorPointPairs *pointStore, const QPointF &linePoint1, const QPointF &linePoint2)
{
    auto d1 = DrawUtils::calculatePerpendicularDistance(
        pointStore->ownShipPoints.second,
        linePoint1,
        linePoint2);

    auto d2 = DrawUtils::calculatePerpendicularDistance(
        pointStore->adoptedTrackPoints.second,
        linePoint1,
        linePoint2);

    auto d3 = DrawUtils::calculatePerpendicularDistance(
        pointStore->selectedTrackPoints.second,
        linePoint1,
        linePoint2);

    std::vector<qreal> distances = {d1, d2, d3};
    auto maxref = std::max_element(distances.begin(), distances.end());

    qDebug() << "own ship: " << d1;
    qDebug() << "adopted tracl: " << d2;
    qDebug() << "selected tracl: " << d3;

    qreal maxValue = *maxref;

    qDebug() << "max distance: " << maxValue;

    return maxValue;
}

/**
 * @brief Draws the own ship vector
 *
 * @param magnitude
 * @param bearing
 */
void TacticalSolutionView::drawOwnShipVector(qreal ownShipSpeed, qreal ownShipBearing)
{
    QPointF ownShipBearingPosition = QPointF(0, 0);
    QPointF ownShipPosition = DrawUtils::bearingToCartesian(
        0,
        0,
        this->scene->sceneRect());

    // Draw a figure (circle) at each position
    DrawUtils::drawCourseVector(scene, ownShipPosition, ownShipSpeed / SPEED_NORMALIZATION_FACTOR, ownShipBearing, Qt::cyan);
}

/**
 * @brief Draws the selected track vector
 *
 * @param sensorBearing
 * @param selectedTrackSpeed
 * @param selectedTrackBearing
 * @param magnitude
 */
void TacticalSolutionView::drawSelectedTrackVector(qreal sensorBearing, qreal selectedTrackRange, qreal selectedTrackBearing, qreal selectedTrackSpeed)
{
    QPointF selectedTrackPosition = DrawUtils::bearingToCartesian(
        selectedTrackRange,
        sensorBearing,
        this->scene->sceneRect());
    // Draw a figure (circle) at each position
    DrawUtils::drawCourseVector(scene, selectedTrackPosition, selectedTrackSpeed / SPEED_NORMALIZATION_FACTOR, selectedTrackBearing, Qt::yellow);
}

/**
 * @brief Draws the adopted track vector
 *
 * @param sensorBearing
 * @param adoptedTrackSpeed
 * @param adoptedTrackBearing
 * @param magnitude
 */
void TacticalSolutionView::drawAdoptedTrackVector(qreal sensorBearing, qreal adoptedTrackRange, qreal adoptedTrackBearing, qreal adoptedTrackSpeed)
{
    QPointF adoptedTrackPosition = DrawUtils::bearingToCartesian(
        adoptedTrackRange,
        sensorBearing,
        this->scene->sceneRect());
    // Draw a figure (circle) at each position
    DrawUtils::drawCourseVector(scene, adoptedTrackPosition, adoptedTrackSpeed / SPEED_NORMALIZATION_FACTOR, adoptedTrackBearing, Qt::red);
}

/**
 * @brief Calculate the Guide Box
 *
 * @param ownShipSpeed
 * @param ownShipBearing
 * @param sensorBearing
 * @param adoptedTrackRange
 * @param adoptedTrackSpeed
 * @param adoptedTrackBearing
 * @param selectedTrackRange
 * @param selectedTrackSpeed
 * @param selectedTrackBearing
 * @return QRectF
 */
QRectF TacticalSolutionView::getGuideBox(
    qreal ownShipSpeed,
    qreal ownShipBearing,
    qreal sensorBearing,
    qreal adoptedTrackRange,
    qreal adoptedTrackSpeed,
    qreal adoptedTrackBearing,
    qreal selectedTrackRange,
    qreal selectedTrackSpeed,
    qreal selectedTrackBearing,
    VectorPointPairs *pointStore)
{
    std::vector<QPointF> guideBoxPoints;

    // Own Ship Vector
    QPointF ownShipBearingPosition = QPointF(0, 0);

    QPointF ownShipPosition = DrawUtils::bearingToCartesian(
        0,
        0,
        this->scene->sceneRect());

    auto endpoint = DrawUtils::calculateEndpoint(ownShipPosition, ownShipSpeed, ownShipBearing);

    // Add to the guidebox list
    guideBoxPoints.push_back(ownShipPosition);
    guideBoxPoints.push_back(endpoint);

    // Store the points
    pointStore->ownShipPoints = qMakePair(ownShipBearingPosition, endpoint);

    // Selected Track Vector
    QPointF selectedTrackPosition = DrawUtils::bearingToCartesian(
        selectedTrackRange,
        sensorBearing,
        this->scene->sceneRect());

    endpoint = DrawUtils::calculateEndpoint(selectedTrackPosition, selectedTrackSpeed, selectedTrackBearing);

    // Add to the guidebox list
    guideBoxPoints.push_back(selectedTrackPosition);
    guideBoxPoints.push_back(endpoint);

    // Store the points
    pointStore->selectedTrackPoints = qMakePair(selectedTrackPosition, endpoint);

    // Adopted Track Vector
    QPointF adoptedTrackPosition = DrawUtils::bearingToCartesian(
        adoptedTrackRange,
        sensorBearing,
        this->scene->sceneRect());

    endpoint = DrawUtils::calculateEndpoint(adoptedTrackPosition, adoptedTrackSpeed, adoptedTrackBearing);

    // Add to the guidebox list
    guideBoxPoints.push_back(adoptedTrackPosition);
    guideBoxPoints.push_back(endpoint);

    // Store the points
    pointStore->adoptedTrackPoints = qMakePair(adoptedTrackPosition, endpoint);

    // Loop throught the guidebox points and find the min/max x,y co-ordinates amongt
    qreal xmin = 0;
    qreal xmax = 0;
    qreal ymin = 0;
    qreal ymax = 0;

    QPointF firstelement = guideBoxPoints.front();

    xmin = firstelement.x();
    xmax = firstelement.x();

    ymin = firstelement.y();
    ymax = firstelement.y();

    // Loop through all the points to find the extent of the guidebox
    for (const QPointF &element : guideBoxPoints)
    {
        if (element.x() > xmax)
        {
            xmax = element.x();
        }

        if (element.x() < xmin)
        {
            xmin = element.x();
        }

        if (element.y() > ymax)
        {
            ymax = element.y();
        }

        if (element.y() < ymin)
        {
            ymin = element.y();
        }
    }

    // Here's the box needed
    QRectF guidebox = QRectF(
        xmin,
        ymin,
        xmax - xmin,
        ymax - ymin);

    return guidebox;
}

/**
 * @brief Returns the zoom box from the guidebox
 *
 * @param guidebox
 * @return QRectF
 */
QRectF TacticalSolutionView::getZoomBoxFromGuideBox(const QRectF guidebox)
{
    // Look for the biggest side and save it
    qreal largestSide;

    if (guidebox.width() > guidebox.height())
    {
        largestSide = guidebox.width();
    }
    else
    {
        largestSide = guidebox.height();
    }

    // Now expand the dimension
    largestSide = ZOOMBOX_EXPANSION_FACTOR * largestSide;

    // Now create a new rectagle with the same center as the previous

    qreal deltax = (largestSide - guidebox.width()) / 2;
    qreal deltay = (largestSide - guidebox.height()) / 2;

    QRectF zoomBox = QRectF(guidebox);

    zoomBox.adjust(
        -deltax,
        -deltay,
        deltax,
        deltay);

    return zoomBox;
}

void TacticalSolutionView::setData(
    const qreal &ownShipSpeed,
    const qreal &ownShipBearing,
    const qreal &sensorBearing,
    const qreal &adoptedTrackRange,
    const qreal &adoptedTrackSpeed,
    const qreal &adoptedTrackBearing,
    const qreal &selectedTrackRange,
    const qreal &selectedTrackSpeed,
    const qreal &selectedTrackBearing)
{
    // Store bearings as-is (angles don’t scale)
    this->ownShipBearing     = ownShipBearing;
    this->sensorBearing      = sensorBearing;
    this->adoptedTrackBearing  = adoptedTrackBearing;
    this->selectedTrackBearing = selectedTrackBearing;

    if (!scene || scene->sceneRect().isEmpty()) {
        // If scene not ready, store raw values without scaling
        this->ownShipSpeed     = ownShipSpeed;
        this->adoptedTrackSpeed = adoptedTrackSpeed;
        this->selectedTrackSpeed = selectedTrackSpeed;
        this->adoptedTrackRange  = adoptedTrackRange;
        this->selectedTrackRange = selectedTrackRange;
        this->update();
        return;
    }

    // ------------------------------
    // Step 1: Get scene dimensions
    // ------------------------------
    qreal sceneDim = std::min(scene->sceneRect().width(), scene->sceneRect().height());
    qreal usableSceneSize = sceneDim * 0.8;   // leave ~10% margin on each side

    // ------------------------------
    // Step 2: Compute max extents
    // ------------------------------
    qreal maxRange = std::max(adoptedTrackRange, selectedTrackRange);
    qreal maxSpeed = std::max({ownShipSpeed, adoptedTrackSpeed, selectedTrackSpeed});

    // ------------------------------
    // Step 3: Compute normalization factors
    // ------------------------------
    qreal rangeNorm = 1.0;
    qreal speedNorm = 1.0;

    if (maxRange > 0)
        rangeNorm = (usableSceneSize * 0.5) / maxRange;   // ranges fit ~50% radius

    if (maxSpeed > 0)
        speedNorm = (usableSceneSize * 0.2) / maxSpeed;   // speeds fit ~20% radius

    // ------------------------------
    // Step 4: Apply normalization
    // ------------------------------
    this->ownShipSpeed       = ownShipSpeed     * speedNorm;
    this->adoptedTrackSpeed  = adoptedTrackSpeed * speedNorm;
    this->selectedTrackSpeed = selectedTrackSpeed * speedNorm;

    this->adoptedTrackRange  = adoptedTrackRange * rangeNorm;
    this->selectedTrackRange = selectedTrackRange * rangeNorm;

    // ------------------------------
    // Step 5: Trigger redraw
    // ------------------------------
    this->update();
}
