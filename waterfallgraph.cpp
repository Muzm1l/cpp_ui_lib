#include "waterfallgraph.h"
#include "ui_waterfallgraph.h"

waterfallgraph::waterfallgraph(QWidget *parent, bool enableGrid, int gridDivisions)
    : QWidget(parent)
    , ui(new Ui::waterfallgraph)
    , graphicsScene(nullptr)
    , gridEnabled(enableGrid)
    , gridDivisions(gridDivisions)
    , isDragging(false)
{
    ui->setupUi(this);
    
    // Set black background (like two-axis graph)
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::black);
    setPalette(pal);
    setAutoFillBackground(true);
    
    // Set minimum size
    setMinimumSize(400, 300);
    
    // Initialize scene
    graphicsScene = new QGraphicsScene(this);
    graphicsScene->setSceneRect(0, 0, width(), height());
    
    // Make sure widget expands
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Enable mouse tracking
    setMouseTracking(true);
    
    // Initial draw will happen in paintEvent
    qDebug() << "Constructor - Widget size:" << this->size();
    qDebug() << "Constructor - Graphics scene rect:" << graphicsScene->sceneRect();
}

waterfallgraph::~waterfallgraph()
{
    delete ui;
    // Note: graphicsScene is a child scene, so it will be automatically deleted by Qt's parent-child mechanism
}

void waterfallgraph::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (graphicsScene)
    {
        // Draw all elements
        draw();

        // Render the scene to the widget
        graphicsScene->render(&painter, rect(), graphicsScene->sceneRect());
    }

    qDebug() << "Paint event - Widget size:" << width() << "x" << height();
}

void waterfallgraph::setData(const std::vector<double>& xData, const std::vector<double>& yData)
{
    // Validate that both vectors have the same size
    if (xData.size() != yData.size()) {
        qDebug() << "Error: xData and yData must have the same size. xData size:" << xData.size() << "yData size:" << yData.size();
        return;
    }
    
    // Store the data
    this->xData = xData;
    this->yData = yData;
    
    qDebug() << "Data set successfully. Size:" << xData.size();
    // TODO: Redraw the graph with the new data
}

void waterfallgraph::onMouseClick(const QPointF& scenePos)
{
    qDebug() << "Mouse clicked at scene position:" << scenePos;
    // This is a virtual function that can be overridden in derived classes
}

void waterfallgraph::onMouseDrag(const QPointF& scenePos)
{
    qDebug() << "Mouse dragged to scene position:" << scenePos;
    // This is a virtual function that can be overridden in derived classes
}

void waterfallgraph::draw()
{
    if (!graphicsScene)
        return;

    // Clear existing items
    graphicsScene->clear();

    // Update scene rect to match widget size
    graphicsScene->setSceneRect(0, 0, width(), height());

    // Update the drawing area
    setupDrawingArea();

    // Draw grid if enabled
    if (gridEnabled) {
        drawGrid();
    }

    // Draw test geometry using drawutils
    DrawUtils::drawDefaultTestPattern(graphicsScene);
}

void waterfallgraph::setupDrawingArea()
{
    // Set up the drawing area to cover the entire scene
    drawingArea = graphicsScene->sceneRect();
    qDebug() << "Drawing area set to:" << drawingArea;
}



void waterfallgraph::drawGrid()
{
    if (!graphicsScene || !gridEnabled || drawingArea.isEmpty() || gridDivisions <= 0) return;
    
    QPen gridPen(Qt::white, 1, Qt::DashLine); // White grid lines for black background
    
    // Draw vertical grid lines (for x-axis - variable)
    double stepX = drawingArea.width() / gridDivisions;
    for (int i = 0; i <= gridDivisions; ++i) {
        double x = drawingArea.left() + i * stepX;
        graphicsScene->addLine(x, drawingArea.top(), x, drawingArea.bottom(), gridPen);
    }
    
    // Draw horizontal grid lines (for y-axis - time)
    double stepY = drawingArea.height() / gridDivisions;
    for (int i = 0; i <= gridDivisions; ++i) {
        double y = drawingArea.top() + i * stepY;
        graphicsScene->addLine(drawingArea.left(), y, drawingArea.right(), y, gridPen);
    }
    
    // Draw border
    QPen borderPen(Qt::white, 2); // White border for black background
    graphicsScene->addRect(drawingArea, borderPen);
}

void waterfallgraph::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // Convert widget coordinates to scene coordinates
        QPointF scenePos = QPointF(event->pos());
        
        // Check if the click is within the drawing area
        if (drawingArea.contains(scenePos)) {
            isDragging = true;
            lastMousePos = scenePos;
            onMouseClick(scenePos);
        }
    }
    
    QWidget::mousePressEvent(event);
}

void waterfallgraph::mouseMoveEvent(QMouseEvent *event)
{
    if (isDragging && (event->buttons() & Qt::LeftButton)) {
        // Convert widget coordinates to scene coordinates
        QPointF scenePos = QPointF(event->pos());
        
        // Check if the move is within the drawing area
        if (drawingArea.contains(scenePos)) {
            onMouseDrag(scenePos);
            lastMousePos = scenePos;
        }
    }
    
    QWidget::mouseMoveEvent(event);
}

void waterfallgraph::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        isDragging = false;
    }
    
    QWidget::mouseReleaseEvent(event);
}

void waterfallgraph::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (graphicsScene)
    {
        // Redraw will happen in paintEvent
        update();

        qDebug() << "Resize event - New size:" << size();
    }
}


void waterfallgraph::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    
    // This is called when the widget becomes visible
    qDebug() << "showEvent - Widget size:" << this->size();
    
    // Force a redraw
    update();
}
