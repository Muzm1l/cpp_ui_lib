#include "waterfallgraph.h"
#include "ui_waterfallgraph.h"

waterfallgraph::waterfallgraph(QWidget *parent, bool enableGrid, int gridDivisions)
    : QWidget(parent)
    , ui(new Ui::waterfallgraph)
    , graphicsView(nullptr)
    , graphicsScene(nullptr)
    , gridEnabled(enableGrid)
    , gridDivisions(gridDivisions)
    , isDragging(false)
{
    ui->setupUi(this);
    
    // Create graphics scene with initial dimensions
    graphicsScene = new QGraphicsScene(this);
    
    // Create graphics view
    graphicsView = new QGraphicsView(graphicsScene, this);
    graphicsView->setRenderHint(QPainter::Antialiasing);
    graphicsView->setDragMode(QGraphicsView::NoDrag); // We'll handle our own mouse events
    
    // Set black background for both scene and view
    graphicsScene->setBackgroundBrush(QBrush(Qt::black));
    graphicsView->setBackgroundBrush(QBrush(Qt::black));
    
    // Disable scrollbars and configure for no scrolling
    graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    graphicsView->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    graphicsView->setResizeAnchor(QGraphicsView::AnchorViewCenter);
    graphicsView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    
    // Set initial scene rect (will be updated later)
    graphicsScene->setSceneRect(0, 0, 100, 100); // Small initial size
    
    // Set up layout to make the graphics view fill the widget exactly
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0); // No margins for perfect fit
    layout->setSpacing(0); // No spacing
    layout->addWidget(graphicsView);
    setLayout(layout);
    
    // Debug: Check initial sizes
    qDebug() << "Constructor - Widget size:" << this->size();
    qDebug() << "Constructor - Graphics view size:" << graphicsView->size();
    qDebug() << "Constructor - Graphics scene rect:" << graphicsScene->sceneRect();
    
    // Note: updateGraphicsDimensions() will be called in showEvent()
    // when the widget is actually visible and has proper dimensions
    
}

waterfallgraph::~waterfallgraph()
{
    delete ui;
    // Note: graphicsView and graphicsScene are child widgets/scenes, 
    // so they will be automatically deleted by Qt's parent-child mechanism
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

void waterfallgraph::setupDrawingArea()
{
    // Set up the drawing area to cover the entire scene
    drawingArea = graphicsScene->sceneRect();
    qDebug() << "Drawing area set to:" << drawingArea;
}

void waterfallgraph::updateGraphicsDimensions()
{
    if (!graphicsView || !graphicsScene) return;
    
    // Get the current size of the graphics view
    QSize viewSize = graphicsView->size();
    QSize widgetSize = this->size();
    
    qDebug() << "updateGraphicsDimensions - Widget size:" << widgetSize;
    qDebug() << "updateGraphicsDimensions - Graphics view size:" << viewSize;
    
    // Only update if we have valid dimensions
    if (viewSize.width() > 0 && viewSize.height() > 0) {
        // Set the scene rect to match the view size exactly
        QRectF newSceneRect(0, 0, viewSize.width(), viewSize.height());
        graphicsScene->setSceneRect(newSceneRect);
        
        // Ensure the graphics view fits the scene exactly (no scrollbars)
        graphicsView->setSceneRect(newSceneRect);
        graphicsView->resetTransform(); // Reset any scaling
        graphicsView->setTransform(QTransform()); // Ensure 1:1 mapping
        
        // Additional measures to prevent scrolling
        graphicsView->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        graphicsView->setResizeAnchor(QGraphicsView::AnchorViewCenter);
        
        // Force the view to update
        graphicsView->update();
        
        // Ensure background is black
        forceBackgroundUpdate();
        
        // Update the drawing area
        setupDrawingArea();
        
        // Clear existing grid and redraw
        if (gridEnabled) {
            drawGrid();
        }
        
        // Draw test geometry using drawutils (only if not already drawn)
        static bool testPatternDrawn = false;
        if (!testPatternDrawn) {
            DrawUtils::drawDefaultTestPattern(graphicsScene);
            testPatternDrawn = true;
        }
        
        qDebug() << "Graphics dimensions updated successfully to:" << viewSize;
        qDebug() << "Scene rect is now:" << graphicsScene->sceneRect();
        qDebug() << "View rect is now:" << graphicsView->sceneRect();
        qDebug() << "Size difference:" << (viewSize.width() - graphicsScene->sceneRect().width()) << "x" << (viewSize.height() - graphicsScene->sceneRect().height());
        qDebug() << "Background brush:" << graphicsScene->backgroundBrush().color();
    } else {
        qDebug() << "Graphics view size is invalid, skipping update";
    }
}

void waterfallgraph::clearGrid()
{
    // Remove all grid items from the scene
    for (QGraphicsItem* item : gridItems) {
        if (item) {
            graphicsScene->removeItem(item);
            delete item;
        }
    }
    gridItems.clear();
}

void waterfallgraph::drawGrid()
{
    if (!graphicsScene || !gridEnabled) return;
    
    // Clear existing grid first
    clearGrid();
    
    QPen gridPen(Qt::white, 1, Qt::DashLine); // White grid lines for black background
    
    // Draw vertical grid lines (for x-axis - variable)
    double stepX = drawingArea.width() / gridDivisions;
    for (int i = 0; i <= gridDivisions; ++i) {
        double x = drawingArea.left() + i * stepX;
        QGraphicsLineItem* line = graphicsScene->addLine(x, drawingArea.top(), x, drawingArea.bottom(), gridPen);
        gridItems.append(line);
    }
    
    // Draw horizontal grid lines (for y-axis - time)
    double stepY = drawingArea.height() / gridDivisions;
    for (int i = 0; i <= gridDivisions; ++i) {
        double y = drawingArea.top() + i * stepY;
        QGraphicsLineItem* line = graphicsScene->addLine(drawingArea.left(), y, drawingArea.right(), y, gridPen);
        gridItems.append(line);
    }
    
    // Draw border
    QPen borderPen(Qt::white, 2); // White border for black background
    QGraphicsRectItem* border = graphicsScene->addRect(drawingArea, borderPen);
    gridItems.append(border);
}

void waterfallgraph::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // Convert widget coordinates to scene coordinates
        QPointF scenePos = graphicsView->mapToScene(event->pos());
        
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
        QPointF scenePos = graphicsView->mapToScene(event->pos());
        
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
    
    // Update graphics dimensions when the widget is resized
    updateGraphicsDimensions();
}

void waterfallgraph::forceBackgroundUpdate()
{
    if (!graphicsScene || !graphicsView) return;
    
    // Force black background
    graphicsScene->setBackgroundBrush(QBrush(Qt::black));
    graphicsView->setBackgroundBrush(QBrush(Qt::black));
    
    // Force update
    graphicsScene->update();
    graphicsView->update();
    
    qDebug() << "Background forced to black";
    qDebug() << "Scene background:" << graphicsScene->backgroundBrush().color();
    qDebug() << "View background:" << graphicsView->backgroundBrush().color();
}

void waterfallgraph::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    
    // This is called when the widget becomes visible
    // At this point, the layout should be processed and we should have proper dimensions
    qDebug() << "showEvent - Widget size:" << this->size();
    qDebug() << "showEvent - Graphics view size:" << graphicsView->size();
    
    // Force background update
    forceBackgroundUpdate();
    
    // Update graphics dimensions now that we're visible
    updateGraphicsDimensions();
}
